/*
 * Copyright (c) 2013  Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Backport functionality introduced in Linux 3.13.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <net/genetlink.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
#ifdef CPTCFG_REGULATOR
#include <linux/module.h>
#include <linux/regulator/driver.h>
#include <linux/device.h>

static void devm_rdev_release(struct device *dev, void *res)
{
	regulator_unregister(*(struct regulator_dev **)res);
}

/**
 * devm_regulator_register - Resource managed regulator_register()
 * @regulator_desc: regulator to register
 * @config: runtime configuration for regulator
 *
 * Called by regulator drivers to register a regulator.  Returns a
 * valid pointer to struct regulator_dev on success or an ERR_PTR() on
 * error.  The regulator will automatically be released when the device
 * is unbound.
 */
struct regulator_dev *devm_regulator_register(struct device *dev,
				  const struct regulator_desc *regulator_desc,
				  const struct regulator_config *config)
{
	struct regulator_dev **ptr, *rdev;

	ptr = devres_alloc(devm_rdev_release, sizeof(*ptr),
			   GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	rdev = regulator_register(regulator_desc, config);
	if (!IS_ERR(rdev)) {
		*ptr = rdev;
		devres_add(dev, ptr);
	} else {
		devres_free(ptr);
	}

	return rdev;
}
//EXPORT_SYMBOL_GPL(devm_regulator_register);

static int devm_rdev_match(struct device *dev, void *res, void *data)
{
	struct regulator_dev **r = res;
	if (!r || !*r) {
		WARN_ON(!r || !*r);
		return 0;
	}
	return *r == data;
}

/**
 * devm_regulator_unregister - Resource managed regulator_unregister()
 * @regulator: regulator to free
 *
 * Unregister a regulator registered with devm_regulator_register().
 * Normally this function will not need to be called and the resource
 * management code will ensure that the resource is freed.
 */
void devm_regulator_unregister(struct device *dev, struct regulator_dev *rdev)
{
	int rc;

	rc = devres_release(dev, devm_rdev_release, devm_rdev_match, rdev);
	if (rc != 0)
		WARN_ON(rc);
}
//EXPORT_SYMBOL_GPL(devm_regulator_unregister);
#endif /* CPTCFG_REGULATOR */
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)) */

/************* generic netlink backport *****************/

#undef genl_register_family
#undef genl_unregister_family

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#undef genl_info
static LIST_HEAD(backport_nl_fam);

static struct genl_ops *genl_get_cmd(u8 cmd, struct genl_family *family)
{
	struct genl_ops *ops;

	list_for_each_entry(ops, &family->family.ops_list, ops.ops_list)
		if (ops->cmd == cmd)
			return ops;

	return NULL;
}

static int nl_doit_wrapper(struct sk_buff *skb, struct genl_info *info)
{
	struct backport_genl_info backport_info;
	struct genl_family *family;
	struct genl_ops *ops;
	int err;

	list_for_each_entry(family, &backport_nl_fam, list) {
		if (family->id == info->nlhdr->nlmsg_type)
			goto found;
	}
	return -ENOENT;

found:
	ops = genl_get_cmd(info->genlhdr->cmd, family);
	if (!ops)
		return -ENOENT;

	memset(&backport_info.user_ptr, 0, sizeof(backport_info.user_ptr));
	backport_info.info = info;
#define __copy(_field) backport_info._field = info->_field
	__copy(snd_seq);
	__copy(snd_pid);
	__copy(genlhdr);
	__copy(attrs);
#undef __copy
	if (family->pre_doit) {
		err = family->pre_doit(ops, skb, &backport_info);
		if (err)
			return err;
	}

	err = ops->doit(skb, &backport_info);

	if (family->post_doit)
		family->post_doit(ops, skb, &backport_info);

	return err;
}
#endif /* < 2.6.37 */

int __backport_genl_register_family(struct genl_family *family)
{
	int i, ret;

#define __copy(_field) family->family._field = family->_field
	__copy(id);
	__copy(hdrsize);
	__copy(version);
	__copy(maxattr);
	strncpy(family->family.name, family->name, sizeof(family->family.name));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	__copy(netnsok);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	__copy(pre_doit);
	__copy(post_doit);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	__copy(parallel_ops);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)
	__copy(module);
#endif
#undef __copy

	ret = genl_register_family(&family->family);
	if (ret < 0)
		return ret;

	family->attrbuf = family->family.attrbuf;
	family->id = family->family.id;

	for (i = 0; i < family->n_ops; i++) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#define __copy(_field) family->ops[i].ops._field = family->ops[i]._field
		__copy(cmd);
		__copy(flags);
		__copy(policy);
		__copy(dumpit);
		__copy(done);
#undef __copy
		if (family->ops[i].doit)
			family->ops[i].ops.doit = nl_doit_wrapper;
		ret = genl_register_ops(&family->family, &family->ops[i].ops);
#else
		ret = genl_register_ops(&family->family, &family->ops[i]);
#endif
		if (ret < 0)
			goto error;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	list_add(&family->list, &backport_nl_fam);
#endif

	for (i = 0; i < family->n_mcgrps; i++) {
		ret = genl_register_mc_group(&family->family,
					     &family->mcgrps[i]);
		if (ret)
			goto error;
	}

	return 0;

 error:
	backport_genl_unregister_family(family);
	return ret;
}
//EXPORT_SYMBOL_GPL(__backport_genl_register_family);

int backport_genl_unregister_family(struct genl_family *family)
{
	int err;
	err = genl_unregister_family(&family->family);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	list_del(&family->list);
#endif
	return err;
}
//EXPORT_SYMBOL_GPL(backport_genl_unregister_family);
