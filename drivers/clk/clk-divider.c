/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
 * Copyright (C) 2011 Richard Zhao, Linaro <richard.zhao@linaro.org>
 * Copyright (C) 2011-2012 Mike Turquette, Linaro Ltd <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable divider clock implementation
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/log2.h>

/*
 * DOC: basic adjustable divider clock that cannot gate
 *
 * Traits of this clock:
 * prepare - clk_prepare only ensures that parents are prepared
 * enable - clk_enable only ensures that parents are enabled
 * rate - rate is adjustable.  clk->rate = DIV_ROUND_UP(parent->rate / divisor)
 * parent - fixed parent.  No clk_set_parent support
 */

#define to_clk_divider(_hw) container_of(_hw, struct clk_divider, hw)

#define div_mask(d)	((1 << ((d)->width)) - 1)

static unsigned int _get_table_maxdiv(const struct clk_div_table *table)
{
	unsigned int maxdiv = 0;
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div > maxdiv)
			maxdiv = clkt->div;
	return maxdiv;
}

static unsigned int _get_table_mindiv(const struct clk_div_table *table)
{
	unsigned int mindiv = UINT_MAX;
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div < mindiv)
			mindiv = clkt->div;
	return mindiv;
}

static unsigned int _get_maxdiv(struct clk_divider *divider)
{
	if (divider->flags & CLK_DIVIDER_ONE_BASED)
		return div_mask(divider);
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << div_mask(divider);
	if (divider->table)
		return _get_table_maxdiv(divider->table);
	return div_mask(divider) + 1;
}

static unsigned int _get_table_div(const struct clk_div_table *table,
							unsigned int val)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == val)
			return clkt->div;
	return 0;
}

static unsigned int _get_div(struct clk_divider *divider, unsigned int val)
{
	if (divider->flags & CLK_DIVIDER_ONE_BASED)
		return val;
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << val;
	if (divider->table)
		return _get_table_div(divider->table, val);
	return val + 1;
}

static unsigned int _get_table_val(const struct clk_div_table *table,
							unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return clkt->val;
	return 0;
}

static unsigned int _get_val(struct clk_divider *divider, unsigned int div)
{
	if (divider->flags & CLK_DIVIDER_ONE_BASED)
		return div;
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return __ffs(div);
	if (divider->table)
		return  _get_table_val(divider->table, div);
	return div - 1;
}

static unsigned long clk_divider_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_divider *divider = to_clk_divider(hw);
	unsigned int div, val;

	val = clk_readl(divider->reg) >> divider->shift;
	val &= div_mask(divider);

	div = _get_div(divider, val);
	if (!div) {
		WARN(!(divider->flags & CLK_DIVIDER_ALLOW_ZERO),
			"%s: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
			__clk_get_name(hw->clk));
		return parent_rate;
	}

	return DIV_ROUND_UP(parent_rate, div);
}

static bool _is_valid_table_div(const struct clk_div_table *table,
							 unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return true;
	return false;
}

static bool _is_valid_div(struct clk_divider *divider, unsigned int div)
{
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return is_power_of_2(div);
	if (divider->table)
		return _is_valid_table_div(divider->table, div);
	return true;
}

static int _round_up_table(const struct clk_div_table *table, int div)
{
	const struct clk_div_table *clkt;
	int up = INT_MAX;

	for (clkt = table; clkt->div; clkt++) {
		if (clkt->div == div)
			return clkt->div;
		else if (clkt->div < div)
			continue;

		if ((clkt->div - div) < (up - div))
			up = clkt->div;
	}

	return up;
}

static int _round_down_table(const struct clk_div_table *table, int div)
{
	const struct clk_div_table *clkt;
	int down = _get_table_mindiv(table);

	for (clkt = table; clkt->div; clkt++) {
		if (clkt->div == div)
			return clkt->div;
		else if (clkt->div > div)
			continue;

		if ((div - clkt->div) < (div - down))
			down = clkt->div;
	}

	return down;
}

static int _div_round_up(struct clk_divider *divider,
		unsigned long parent_rate, unsigned long rate)
{
	int div = DIV_ROUND_UP(parent_rate, rate);

	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		div = __roundup_pow_of_two(div);
	if (divider->table)
		div = _round_up_table(divider->table, div);

	return div;
}

static int _div_round_closest(struct clk_divider *divider,
		unsigned long parent_rate, unsigned long rate)
{
	int up, down, div;
	unsigned long up_rate, down_rate;

	up = down = div = DIV_ROUND_CLOSEST(parent_rate, rate);

	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO) {
		up = __roundup_pow_of_two(div);
		down = __rounddown_pow_of_two(div);
	} else if (divider->table) {
		up = _round_up_table(divider->table, div);
		down = _round_down_table(divider->table, div);
	}

	up_rate = DIV_ROUND_UP(parent_rate, up);
	down_rate = DIV_ROUND_UP(parent_rate, down);

	return (rate - up_rate) <= (down_rate - rate) ? up : down;
}

static int _div_round(struct clk_divider *divider, unsigned long parent_rate,
		unsigned long rate)
{
	if (divider->flags & CLK_DIVIDER_ROUND_CLOSEST)
		return _div_round_closest(divider, parent_rate, rate);

	return _div_round_up(divider, parent_rate, rate);
}

static bool _is_best_div(struct clk_divider *divider,
		unsigned long rate, unsigned long now, unsigned long best)
{
	if (divider->flags & CLK_DIVIDER_ROUND_CLOSEST)
		return abs(rate - now) < abs(rate - best);

	return now <= rate && now > best;
}

static int _next_div(struct clk_divider *divider, int div)
{
	div++;

	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return __roundup_pow_of_two(div);
	if (divider->table)
		return _round_up_table(divider->table, div);

	return div;
}

static int clk_divider_bestdiv(struct clk_hw *hw, unsigned long rate,
		unsigned long *best_parent_rate)
{
	struct clk_divider *divider = to_clk_divider(hw);
	int i, bestdiv = 0;
	unsigned long parent_rate, best = 0, now, maxdiv;
	unsigned long parent_rate_saved = *best_parent_rate;

	if (!rate)
		rate = 1;

	/* if read only, just return current value */
	if (divider->flags & CLK_DIVIDER_READ_ONLY) {
		bestdiv = readl(divider->reg) >> divider->shift;
		bestdiv &= div_mask(divider);
		bestdiv = _get_div(divider, bestdiv);
		return bestdiv;
	}

	maxdiv = _get_maxdiv(divider);

	if (!(__clk_get_flags(hw->clk) & CLK_SET_RATE_PARENT)) {
		parent_rate = *best_parent_rate;
		bestdiv = _div_round(divider, parent_rate, rate);
		bestdiv = bestdiv == 0 ? 1 : bestdiv;
		bestdiv = bestdiv > maxdiv ? maxdiv : bestdiv;
		return bestdiv;
	}

	/*
	 * The maximum divider we can use without overflowing
	 * unsigned long in rate * i below
	 */
	maxdiv = min(ULONG_MAX / rate, maxdiv);

	for (i = 1; i <= maxdiv; i = _next_div(divider, i)) {
		if (!_is_valid_div(divider, i))
			continue;
		if (rate * i == parent_rate_saved) {
			/*
			 * It's the most ideal case if the requested rate can be
			 * divided from parent clock without needing to change
			 * parent rate, so return the divider immediately.
			 */
			*best_parent_rate = parent_rate_saved;
			return i;
		}
		parent_rate = __clk_round_rate(__clk_get_parent(hw->clk),
					       rate * i);
		now = DIV_ROUND_UP(parent_rate, i);
		if (_is_best_div(divider, rate, now, best)) {
			bestdiv = i;
			best = now;
			*best_parent_rate = parent_rate;
		}
	}

	if (!bestdiv) {
		bestdiv = _get_maxdiv(divider);
		*best_parent_rate = __clk_round_rate(__clk_get_parent(hw->clk), 1);
	}

	return bestdiv;
}

static long clk_divider_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	int div;
	div = clk_divider_bestdiv(hw, rate, prate);

	return DIV_ROUND_UP(*prate, div);
}

static int clk_divider_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct clk_divider *divider = to_clk_divider(hw);
	unsigned int div, value;
	unsigned long flags = 0;
	u32 val;

	div = DIV_ROUND_UP(parent_rate, rate);

	if (!_is_valid_div(divider, div))
		return -EINVAL;

	value = _get_val(divider, div);

	if (value > div_mask(divider))
		value = div_mask(divider);

	if (divider->lock)
		spin_lock_irqsave(divider->lock, flags);

	if (divider->flags & CLK_DIVIDER_HIWORD_MASK) {
		val = div_mask(divider) << (divider->shift + 16);
	} else {
		val = clk_readl(divider->reg);
		val &= ~(div_mask(divider) << divider->shift);
	}
	val |= value << divider->shift;
	clk_writel(val, divider->reg);

	if (divider->lock)
		spin_unlock_irqrestore(divider->lock, flags);

	return 0;
}

const struct clk_ops clk_divider_ops = {
	.recalc_rate = clk_divider_recalc_rate,
	.round_rate = clk_divider_round_rate,
	.set_rate = clk_divider_set_rate,
};
EXPORT_SYMBOL_GPL(clk_divider_ops);

static struct clk *_register_divider(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_divider_flags, const struct clk_div_table *table,
		spinlock_t *lock)
{
	struct clk_divider *div;
	struct clk *clk;
	struct clk_init_data init;

	if (clk_divider_flags & CLK_DIVIDER_HIWORD_MASK) {
		if (width + shift > 16) {
			pr_warn("divider value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the divider */
	div = kzalloc(sizeof(struct clk_divider), GFP_KERNEL);
	if (!div) {
		pr_err("%s: could not allocate divider clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &clk_divider_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_divider assignments */
	div->reg = reg;
	div->shift = shift;
	div->width = width;
	div->flags = clk_divider_flags;
	div->lock = lock;
	div->hw.init = &init;
	div->table = table;

	/* register the clock */
	clk = clk_register(dev, &div->hw);

	if (IS_ERR(clk))
		kfree(div);

	return clk;
}

/**
 * clk_register_divider - register a divider clock with the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust divider
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @clk_divider_flags: divider-specific flags for this clock
 * @lock: shared register lock for this clock
 */
struct clk *clk_register_divider(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_divider_flags, spinlock_t *lock)
{
	return _register_divider(dev, name, parent_name, flags, reg, shift,
			width, clk_divider_flags, NULL, lock);
}
EXPORT_SYMBOL_GPL(clk_register_divider);

/**
 * clk_register_divider_table - register a table based divider clock with
 * the clock framework
 * @dev: device registering this clock
 * @name: name of this clock
 * @parent_name: name of clock's parent
 * @flags: framework-specific flags
 * @reg: register address to adjust divider
 * @shift: number of bits to shift the bitfield
 * @width: width of the bitfield
 * @clk_divider_flags: divider-specific flags for this clock
 * @table: array of divider/value pairs ending with a div set to 0
 * @lock: shared register lock for this clock
 */
struct clk *clk_register_divider_table(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_divider_flags, const struct clk_div_table *table,
		spinlock_t *lock)
{
	return _register_divider(dev, name, parent_name, flags, reg, shift,
			width, clk_divider_flags, table, lock);
}
EXPORT_SYMBOL_GPL(clk_register_divider_table);
