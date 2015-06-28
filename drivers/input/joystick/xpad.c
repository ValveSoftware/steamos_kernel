/*
 * X-Box gamepad driver
 *
 * Copyright (c) 2002 Marko Friedemann <mfr@bmx-chemnitz.de>
 *               2004 Oliver Schwartz <Oliver.Schwartz@gmx.de>,
 *                    Steven Toth <steve@toth.demon.co.uk>,
 *                    Franz Lehner <franz@caos.at>,
 *                    Ivan Hawkes <blackhawk@ivanhawkes.com>
 *               2005 Dominic Cerquetti <binary1230@yahoo.com>
 *               2006 Adam Buchbinder <adam.buchbinder@gmail.com>
 *               2007 Jan Kratochvil <honza@jikos.cz>
 *               2010 Christoph Fritz <chf.fritz@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *
 * This driver is based on:
 *  - information from     http://euc.jp/periphs/xbox-controller.ja.html
 *  - the iForce driver    drivers/char/joystick/iforce.c
 *  - the skeleton-driver  drivers/usb/usb-skeleton.c
 *  - Xbox 360 information http://www.free60.org/wiki/Gamepad
 *
 * Thanks to:
 *  - ITO Takayuki for providing essential xpad information on his website
 *  - Vojtech Pavlik     - iforce driver / input subsystem
 *  - Greg Kroah-Hartman - usb-skeleton driver
 *  - XBOX Linux project - extra USB id's
 *
 * TODO:
 *  - fine tune axes (especially trigger axes)
 *  - fix "analog" buttons (reported as digital now)
 *  - get rumble working
 *  - need USB IDs for other dance pads
 *
 * History:
 *
 * 2002-06-27 - 0.0.1 : first version, just said "XBOX HID controller"
 *
 * 2002-07-02 - 0.0.2 : basic working version
 *  - all axes and 9 of the 10 buttons work (german InterAct device)
 *  - the black button does not work
 *
 * 2002-07-14 - 0.0.3 : rework by Vojtech Pavlik
 *  - indentation fixes
 *  - usb + input init sequence fixes
 *
 * 2002-07-16 - 0.0.4 : minor changes, merge with Vojtech's v0.0.3
 *  - verified the lack of HID and report descriptors
 *  - verified that ALL buttons WORK
 *  - fixed d-pad to axes mapping
 *
 * 2002-07-17 - 0.0.5 : simplified d-pad handling
 *
 * 2004-10-02 - 0.0.6 : DDR pad support
 *  - borrowed from the XBOX linux kernel
 *  - USB id's for commonly used dance pads are present
 *  - dance pads will map D-PAD to buttons, not axes
 *  - pass the module paramater 'dpad_to_buttons' to force
 *    the D-PAD to map to buttons if your pad is not detected
 *
 * Later changes can be tracked in SCM.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/usb/input.h>

#define DRIVER_AUTHOR "Marko Friedemann <mfr@bmx-chemnitz.de>"
#define DRIVER_DESC "X-Box pad driver"

#define XPAD_PKT_LEN 32

/* xbox d-pads should map to buttons, as is required for DDR pads
   but we map them to axes when possible to simplify things */
#define MAP_DPAD_TO_BUTTONS		(1 << 0)
#define MAP_TRIGGERS_TO_BUTTONS		(1 << 1)
#define MAP_STICKS_TO_NULL		(1 << 2)
#define DANCEPAD_MAP_CONFIG	(MAP_DPAD_TO_BUTTONS |			\
				MAP_TRIGGERS_TO_BUTTONS | MAP_STICKS_TO_NULL)

#define XTYPE_XBOX        0
#define XTYPE_XBOX360     1
#define XTYPE_XBOX360W    2
#define XTYPE_XBOXONE     3
#define XTYPE_UNKNOWN     4

static bool dpad_to_buttons;
module_param(dpad_to_buttons, bool, S_IRUGO);
MODULE_PARM_DESC(dpad_to_buttons, "Map D-PAD to buttons rather than axes for unknown pads");

static bool triggers_to_buttons;
module_param(triggers_to_buttons, bool, S_IRUGO);
MODULE_PARM_DESC(triggers_to_buttons, "Map triggers to buttons rather than axes for unknown pads");

static bool sticks_to_null;
module_param(sticks_to_null, bool, S_IRUGO);
MODULE_PARM_DESC(sticks_to_null, "Do not map sticks at all for unknown pads");

static const struct xpad_device {
	u16 idVendor;
	u16 idProduct;
	char *name;
	u8 mapping;
	u8 xtype;
} xpad_device[] = {
	{ 0x045e, 0x0202, "Microsoft X-Box pad v1 (US)", 0, XTYPE_XBOX },
	{ 0x045e, 0x0285, "Microsoft X-Box pad (Japan)", 0, XTYPE_XBOX },
	{ 0x045e, 0x0287, "Microsoft Xbox Controller S", 0, XTYPE_XBOX },
	{ 0x045e, 0x0289, "Microsoft X-Box pad v2 (US)", 0, XTYPE_XBOX },
	{ 0x045e, 0x028e, "Microsoft X-Box 360 pad", 0, XTYPE_XBOX360 },
	{ 0x045e, 0x02d1, "Microsoft X-Box One pad", 0, XTYPE_XBOXONE },
	{ 0x045e, 0x0291, "Xbox 360 Wireless Receiver (XBOX)", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360W },
	{ 0x045e, 0x0719, "Xbox 360 Wireless Receiver", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360W },
	{ 0x044f, 0x0f07, "Thrustmaster, Inc. Controller", 0, XTYPE_XBOX },
	{ 0x046d, 0xc242, "Logitech Chillstream Controller", 0, XTYPE_XBOX360 },
	{ 0x046d, 0xca84, "Logitech Xbox Cordless Controller", 0, XTYPE_XBOX },
	{ 0x046d, 0xca88, "Logitech Compact Controller for Xbox", 0, XTYPE_XBOX },
	{ 0x05fd, 0x1007, "Mad Catz Controller (unverified)", 0, XTYPE_XBOX },
	{ 0x05fd, 0x107a, "InterAct 'PowerPad Pro' X-Box pad (Germany)", 0, XTYPE_XBOX },
	{ 0x0738, 0x4516, "Mad Catz Control Pad", 0, XTYPE_XBOX },
	{ 0x0738, 0x4522, "Mad Catz LumiCON", 0, XTYPE_XBOX },
	{ 0x0738, 0x4526, "Mad Catz Control Pad Pro", 0, XTYPE_XBOX },
	{ 0x0738, 0x4536, "Mad Catz MicroCON", 0, XTYPE_XBOX },
	{ 0x0738, 0x4540, "Mad Catz Beat Pad", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX },
	{ 0x0738, 0x4556, "Mad Catz Lynx Wireless Controller", 0, XTYPE_XBOX },
	{ 0x0738, 0x4716, "Mad Catz Wired Xbox 360 Controller", 0, XTYPE_XBOX360 },
	{ 0x0738, 0x4728, "Mad Catz Street Fighter IV FightPad", MAP_TRIGGERS_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x0738, 0x4738, "Mad Catz Wired Xbox 360 Controller (SFIV)", MAP_TRIGGERS_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x0738, 0x6040, "Mad Catz Beat Pad Pro", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX },
	{ 0x0738, 0xbeef, "Mad Catz JOYTECH NEO SE Advanced GamePad", XTYPE_XBOX360 },
	{ 0x0c12, 0x8802, "Zeroplus Xbox Controller", 0, XTYPE_XBOX },
	{ 0x0c12, 0x8809, "RedOctane Xbox Dance Pad", DANCEPAD_MAP_CONFIG, XTYPE_XBOX },
	{ 0x0c12, 0x880a, "Pelican Eclipse PL-2023", 0, XTYPE_XBOX },
	{ 0x0c12, 0x8810, "Zeroplus Xbox Controller", 0, XTYPE_XBOX },
	{ 0x0c12, 0x9902, "HAMA VibraX - *FAULTY HARDWARE*", 0, XTYPE_XBOX },
	{ 0x0d2f, 0x0002, "Andamiro Pump It Up pad", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX },
	{ 0x0e4c, 0x1097, "Radica Gamester Controller", 0, XTYPE_XBOX },
	{ 0x0e4c, 0x2390, "Radica Games Jtech Controller", 0, XTYPE_XBOX },
	{ 0x0e6f, 0x0003, "Logic3 Freebird wireless Controller", 0, XTYPE_XBOX },
	{ 0x0e6f, 0x0005, "Eclipse wireless Controller", 0, XTYPE_XBOX },
	{ 0x0e6f, 0x0006, "Edge wireless Controller", 0, XTYPE_XBOX },
	{ 0x0e6f, 0x0105, "HSM3 Xbox360 dancepad", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x0e6f, 0x0201, "Pelican PL-3601 'TSZ' Wired Xbox 360 Controller", 0, XTYPE_XBOX360 },
	{ 0x0e6f, 0x0213, "Afterglow Gamepad for Xbox 360", 0, XTYPE_XBOX360 },
	{ 0x0e8f, 0x0201, "SmartJoy Frag Xpad/PS2 adaptor", 0, XTYPE_XBOX },
	{ 0x0f0d, 0x000d, "Hori Fighting Stick EX2", MAP_TRIGGERS_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x0f0d, 0x0016, "Hori Real Arcade Pro.EX", MAP_TRIGGERS_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x0f30, 0x0202, "Joytech Advanced Controller", 0, XTYPE_XBOX },
	{ 0x0f30, 0x8888, "BigBen XBMiniPad Controller", 0, XTYPE_XBOX },
	{ 0x102c, 0xff0c, "Joytech Wireless Advanced Controller", 0, XTYPE_XBOX },
	{ 0x12ab, 0x0004, "Honey Bee Xbox360 dancepad", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x12ab, 0x8809, "Xbox DDR dancepad", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX },
	{ 0x1430, 0x4748, "RedOctane Guitar Hero X-plorer", 0, XTYPE_XBOX360 },
	{ 0x1430, 0x8888, "TX6500+ Dance Pad (first generation)", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX },
	{ 0x146b, 0x0601, "BigBen Interactive XBOX 360 Controller", 0, XTYPE_XBOX360 },
	{ 0x1689, 0xfd00, "Razer Onza Tournament Edition", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x1689, 0xfd01, "Razer Onza Classic Edition", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x1bad, 0x0002, "Harmonix Rock Band Guitar", 0, XTYPE_XBOX360 },
	{ 0x1bad, 0x0003, "Harmonix Rock Band Drumkit", MAP_DPAD_TO_BUTTONS, XTYPE_XBOX360 },
	{ 0x1bad, 0xf016, "Mad Catz Xbox 360 Controller", 0, XTYPE_XBOX360 },
	{ 0x1bad, 0xf028, "Street Fighter IV FightPad", 0, XTYPE_XBOX360 },
	{ 0x1bad, 0xf901, "Gamestop Xbox 360 Controller", 0, XTYPE_XBOX360 },
	{ 0x1bad, 0xf903, "Tron Xbox 360 controller", 0, XTYPE_XBOX360 },
	{ 0x24c6, 0x5300, "PowerA MINI PROEX Controller", 0, XTYPE_XBOX360 },
	{ 0xffff, 0xffff, "Chinese-made Xbox Controller", 0, XTYPE_XBOX },
	{ 0x0000, 0x0000, "Generic X-Box pad", 0, XTYPE_UNKNOWN }
};

/* buttons shared with xbox and xbox360 */
static const signed short xpad_common_btn[] = {
	BTN_A, BTN_B, BTN_X, BTN_Y,			/* "analog" buttons */
	BTN_START, BTN_SELECT, BTN_THUMBL, BTN_THUMBR,	/* start/back/sticks */
	-1						/* terminating entry */
};

/* original xbox controllers only */
static const signed short xpad_btn[] = {
	BTN_C, BTN_Z,		/* "analog" buttons */
	-1			/* terminating entry */
};

/* used when dpad is mapped to buttons */
static const signed short xpad_btn_pad[] = {
	BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,		/* d-pad left, right */
	BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4,		/* d-pad up, down */
	-1				/* terminating entry */
};

/* used when triggers are mapped to buttons */
static const signed short xpad_btn_triggers[] = {
	BTN_TL2, BTN_TR2,		/* triggers left/right */
	-1
};


static const signed short xpad360_btn[] = {  /* buttons for x360 controller */
	BTN_TL, BTN_TR,		/* Button LB/RB */
	BTN_MODE,		/* The big X button */
	-1
};

static const signed short xpad_abs[] = {
	ABS_X, ABS_Y,		/* left stick */
	ABS_RX, ABS_RY,		/* right stick */
	-1			/* terminating entry */
};

/* used when dpad is mapped to axes */
static const signed short xpad_abs_pad[] = {
	ABS_HAT0X, ABS_HAT0Y,	/* d-pad axes */
	-1			/* terminating entry */
};

/* used when triggers are mapped to axes */
static const signed short xpad_abs_triggers[] = {
	ABS_Z, ABS_RZ,		/* triggers left/right */
	-1
};

/*
 * Xbox 360 has a vendor-specific class, so we cannot match it with only
 * USB_INTERFACE_INFO (also specifically refused by USB subsystem), so we
 * match against vendor id as well. Wired Xbox 360 devices have protocol 1,
 * wireless controllers have protocol 129.
 */
#define XPAD_XBOX360_VENDOR_PROTOCOL(vend,pr) \
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO, \
	.idVendor = (vend), \
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC, \
	.bInterfaceSubClass = 93, \
	.bInterfaceProtocol = (pr)
#define XPAD_XBOX360_VENDOR(vend) \
	{ XPAD_XBOX360_VENDOR_PROTOCOL(vend,1) }, \
	{ XPAD_XBOX360_VENDOR_PROTOCOL(vend,129) }

/* The Xbox One controller uses subclass 71 and protocol 208. */
#define XPAD_XBOXONE_VENDOR_PROTOCOL(vend, pr) \
	.match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_INFO, \
	.idVendor = (vend), \
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC, \
	.bInterfaceSubClass = 71, \
	.bInterfaceProtocol = (pr)
#define XPAD_XBOXONE_VENDOR(vend) \
	{ XPAD_XBOXONE_VENDOR_PROTOCOL(vend, 208) }

static struct usb_device_id xpad_table[] = {
	{ USB_INTERFACE_INFO('X', 'B', 0) },	/* X-Box USB-IF not approved class */
	XPAD_XBOX360_VENDOR(0x045e),		/* Microsoft X-Box 360 controllers */
	XPAD_XBOXONE_VENDOR(0x045e),		/* Microsoft X-Box One controllers */
	XPAD_XBOX360_VENDOR(0x046d),		/* Logitech X-Box 360 style controllers */
	XPAD_XBOX360_VENDOR(0x0738),		/* Mad Catz X-Box 360 controllers */
	{ USB_DEVICE(0x0738, 0x4540) },		/* Mad Catz Beat Pad */
	XPAD_XBOX360_VENDOR(0x0e6f),		/* 0x0e6f X-Box 360 controllers */
	XPAD_XBOX360_VENDOR(0x12ab),		/* X-Box 360 dance pads */
	XPAD_XBOX360_VENDOR(0x1430),		/* RedOctane X-Box 360 controllers */
	XPAD_XBOX360_VENDOR(0x146b),		/* BigBen Interactive Controllers */
	XPAD_XBOX360_VENDOR(0x1bad),		/* Harminix Rock Band Guitar and Drums */
	XPAD_XBOX360_VENDOR(0x0f0d),		/* Hori Controllers */
	XPAD_XBOX360_VENDOR(0x1689),		/* Razer Onza */
	XPAD_XBOX360_VENDOR(0x24c6),		/* PowerA Controllers */
	{ }
};

MODULE_DEVICE_TABLE(usb, xpad_table);

struct irq_out_data {
	struct list_head list;
	unsigned char odata[XPAD_PKT_LEN];
	u32 odata_length;
};

struct usb_xpad {
	struct input_dev *dev;		/* input device interface */
	struct usb_device *udev;	/* usb device */
	struct usb_interface *intf;	/* usb interface */

	int pad_present;

	struct urb *irq_in;		/* urb for interrupt in report */
	unsigned char *idata;		/* input data */
	dma_addr_t idata_dma;

	struct urb *bulk_out;
	unsigned char *bdata;

	struct urb *irq_out;		/* urb for interrupt out report */
	int irq_out_busy;		/* is urb submitted, odata_lock must be held */
	unsigned char *odata;		/* output data */
	dma_addr_t odata_dma;
	spinlock_t odata_lock;
	struct list_head odata_list;	/* odata_lock must be held */

	unsigned char odata_serial; /* serial number for xbox one protocol */

#if defined(CONFIG_JOYSTICK_XPAD_LEDS)
	struct xpad_led *led;
#endif
	
	int joydev_id;

	char phys[64];			/* physical device path */

	int mapping;			/* map d-pad to buttons or to axes */
	int xtype;			/* type of xbox device */
	
	const char *name;
};

static int xpad_send_ff(struct usb_xpad *xpad, int strong, int weak);

/*
 *	xpad_process_packet
 *
 *	Completes a request by converting the data into events for the
 *	input subsystem.
 *
 *	The used report descriptor was taken from ITO Takayukis website:
 *	 http://euc.jp/periphs/xbox-controller.ja.html
 */

static void xpad_process_packet(struct usb_xpad *xpad, u16 cmd, unsigned char *data)
{
	struct input_dev *dev = xpad->dev;

	if (!(xpad->mapping & MAP_STICKS_TO_NULL)) {
		/* left stick */
		input_report_abs(dev, ABS_X,
				 (__s16) le16_to_cpup((__le16 *)(data + 12)));
		input_report_abs(dev, ABS_Y,
				 ~(__s16) le16_to_cpup((__le16 *)(data + 14)));

		/* right stick */
		input_report_abs(dev, ABS_RX,
				 (__s16) le16_to_cpup((__le16 *)(data + 16)));
		input_report_abs(dev, ABS_RY,
				 ~(__s16) le16_to_cpup((__le16 *)(data + 18)));
	}

	/* triggers left/right */
	if (xpad->mapping & MAP_TRIGGERS_TO_BUTTONS) {
		input_report_key(dev, BTN_TL2, data[10]);
		input_report_key(dev, BTN_TR2, data[11]);
	} else {
		input_report_abs(dev, ABS_Z, data[10]);
		input_report_abs(dev, ABS_RZ, data[11]);
	}

	/* digital pad */
	if (xpad->mapping & MAP_DPAD_TO_BUTTONS) {
		/* dpad as buttons (left, right, up, down) */
		input_report_key(dev, BTN_TRIGGER_HAPPY1, data[2] & 0x04);
		input_report_key(dev, BTN_TRIGGER_HAPPY2, data[2] & 0x08);
		input_report_key(dev, BTN_TRIGGER_HAPPY3, data[2] & 0x01);
		input_report_key(dev, BTN_TRIGGER_HAPPY4, data[2] & 0x02);
	} else {
		input_report_abs(dev, ABS_HAT0X,
				 !!(data[2] & 0x08) - !!(data[2] & 0x04));
		input_report_abs(dev, ABS_HAT0Y,
				 !!(data[2] & 0x02) - !!(data[2] & 0x01));
	}

	/* start/back buttons and stick press left/right */
	input_report_key(dev, BTN_START,  data[2] & 0x10);
	input_report_key(dev, BTN_SELECT, data[2] & 0x20);
	input_report_key(dev, BTN_THUMBL, data[2] & 0x40);
	input_report_key(dev, BTN_THUMBR, data[2] & 0x80);

	/* "analog" buttons A, B, X, Y */
	input_report_key(dev, BTN_A, data[4]);
	input_report_key(dev, BTN_B, data[5]);
	input_report_key(dev, BTN_X, data[6]);
	input_report_key(dev, BTN_Y, data[7]);

	/* "analog" buttons black, white */
	input_report_key(dev, BTN_C, data[8]);
	input_report_key(dev, BTN_Z, data[9]);

	input_sync(dev);
}

/*
 *	xpad360_process_packet
 *
 *	Completes a request by converting the data into events for the
 *	input subsystem. It is version for xbox 360 controller
 *
 *	The used report descriptor was taken from:
 *		http://www.free60.org/wiki/Gamepad
 */

static void xpad360_process_packet(struct usb_xpad *xpad,
				   u16 cmd, unsigned char *data)
{
	struct input_dev *dev = xpad->dev;

	/* digital pad */
	if (xpad->mapping & MAP_DPAD_TO_BUTTONS) {
		/* dpad as buttons (left, right, up, down) */
		input_report_key(dev, BTN_TRIGGER_HAPPY1, data[2] & 0x04);
		input_report_key(dev, BTN_TRIGGER_HAPPY2, data[2] & 0x08);
		input_report_key(dev, BTN_TRIGGER_HAPPY3, data[2] & 0x01);
		input_report_key(dev, BTN_TRIGGER_HAPPY4, data[2] & 0x02);
	} else {
		input_report_abs(dev, ABS_HAT0X,
				 !!(data[2] & 0x08) - !!(data[2] & 0x04));
		input_report_abs(dev, ABS_HAT0Y,
				 !!(data[2] & 0x02) - !!(data[2] & 0x01));
	}

	/* start/back buttons */
	input_report_key(dev, BTN_START,  data[2] & 0x10);
	input_report_key(dev, BTN_SELECT, data[2] & 0x20);

	/* stick press left/right */
	input_report_key(dev, BTN_THUMBL, data[2] & 0x40);
	input_report_key(dev, BTN_THUMBR, data[2] & 0x80);

	/* buttons A,B,X,Y,TL,TR and MODE */
	input_report_key(dev, BTN_A,	data[3] & 0x10);
	input_report_key(dev, BTN_B,	data[3] & 0x20);
	input_report_key(dev, BTN_X,	data[3] & 0x40);
	input_report_key(dev, BTN_Y,	data[3] & 0x80);
	input_report_key(dev, BTN_TL,	data[3] & 0x01);
	input_report_key(dev, BTN_TR,	data[3] & 0x02);
	input_report_key(dev, BTN_MODE,	data[3] & 0x04);

	if (!(xpad->mapping & MAP_STICKS_TO_NULL)) {
		/* left stick */
		input_report_abs(dev, ABS_X,
				 (__s16) le16_to_cpup((__le16 *)(data + 6)));
		input_report_abs(dev, ABS_Y,
				 ~(__s16) le16_to_cpup((__le16 *)(data + 8)));

		/* right stick */
		input_report_abs(dev, ABS_RX,
				 (__s16) le16_to_cpup((__le16 *)(data + 10)));
		input_report_abs(dev, ABS_RY,
				 ~(__s16) le16_to_cpup((__le16 *)(data + 12)));
	}

	/* triggers left/right */
	if (xpad->mapping & MAP_TRIGGERS_TO_BUTTONS) {
		input_report_key(dev, BTN_TL2, data[4]);
		input_report_key(dev, BTN_TR2, data[5]);
	} else {
		input_report_abs(dev, ABS_Z, data[4]);
		input_report_abs(dev, ABS_RZ, data[5]);
	}

	input_sync(dev);
}
static void xpad_send_led_command(struct usb_xpad *xpad, int command);
static int xpad_open(struct input_dev *dev);
static void xpad_close(struct input_dev *dev);
static void xpad_set_up_abs(struct input_dev *input_dev, signed short abs);
static int xpad_init_ff(struct usb_xpad *xpad);
static int xpad_find_joydev(struct device *dev, void *data)
{
	if (strstr(dev_name(dev), "js"))
		return 1;
	
	return 0;
}

static struct workqueue_struct *my_wq;

typedef struct {
	struct work_struct my_work;
	struct usb_xpad *xpad;
} my_work_t;

static void my_wq_function( struct work_struct *work)
{
	my_work_t *my_work = (my_work_t *)work;
	int ret;
	
	struct usb_xpad *xpad = my_work->xpad;
	
	if (xpad->pad_present) {
		
		struct input_dev *input_dev;
		int i;
		
		input_dev = input_allocate_device();

		xpad->dev = input_dev;
		input_dev->name = xpad->name;
		input_dev->phys = xpad->phys;
		usb_to_input_id(xpad->udev, &input_dev->id);
		input_dev->dev.parent = &xpad->intf->dev;
		
		input_set_drvdata(input_dev, xpad);
		
		input_dev->open = xpad_open;
		input_dev->close = xpad_close;
		
		input_dev->evbit[0] = BIT_MASK(EV_KEY);
		
		if (!(xpad->mapping & MAP_STICKS_TO_NULL)) {
			input_dev->evbit[0] |= BIT_MASK(EV_ABS);
			/* set up axes */
			for (i = 0; xpad_abs[i] >= 0; i++)
				xpad_set_up_abs(input_dev, xpad_abs[i]);
		}
		
		/* set up standard buttons */
		for (i = 0; xpad_common_btn[i] >= 0; i++)
			__set_bit(xpad_common_btn[i], input_dev->keybit);
		
		/* set up model-specific ones */
		if (xpad->xtype == XTYPE_XBOX360 || xpad->xtype == XTYPE_XBOX360W ||
			xpad->xtype == XTYPE_XBOXONE) {
			for (i = 0; xpad360_btn[i] >= 0; i++)
				__set_bit(xpad360_btn[i], input_dev->keybit);
		} else {
			for (i = 0; xpad_btn[i] >= 0; i++)
				__set_bit(xpad_btn[i], input_dev->keybit);
		}
		
		if (xpad->mapping & MAP_DPAD_TO_BUTTONS) {
			for (i = 0; xpad_btn_pad[i] >= 0; i++)
				__set_bit(xpad_btn_pad[i], input_dev->keybit);
		} else {
			for (i = 0; xpad_abs_pad[i] >= 0; i++)
				xpad_set_up_abs(input_dev, xpad_abs_pad[i]);
		}
		
		if (xpad->mapping & MAP_TRIGGERS_TO_BUTTONS) {
			for (i = 0; xpad_btn_triggers[i] >= 0; i++)
				__set_bit(xpad_btn_triggers[i], input_dev->keybit);
		} else {
			for (i = 0; xpad_abs_triggers[i] >= 0; i++)
				xpad_set_up_abs(input_dev, xpad_abs_triggers[i]);
		}
		
		ret = input_register_device(xpad->dev);
		
		if (ret == 0)
		{
			struct device* joydev_dev = device_find_child(&xpad->dev->dev, NULL, xpad_find_joydev);
			
			if (joydev_dev) {
// 				printk("found joydev child with minor %i\n", MINOR(joydev_dev->devt));
				xpad->joydev_id = MINOR(joydev_dev->devt);
				xpad_send_led_command(xpad, (xpad->joydev_id % 4) + 2);
			}
		}
		else
		{
			printk("xpad: Unable to register input device: %d\n", ret);
		}
		
		xpad_init_ff(xpad);
	} else {
		input_unregister_device(xpad->dev);
	}
	
	kfree( (void *)work );
	
	return;
}

/*
 * xpad360w_process_packet
 *
 * Completes a request by converting the data into events for the
 * input subsystem. It is version for xbox 360 wireless controller.
 *
 * Byte.Bit
 * 00.1 - Status change: The controller or headset has connected/disconnected
 *                       Bits 01.7 and 01.6 are valid
 * 01.7 - Controller present
 * 01.6 - Headset present
 * 01.1 - Pad state (Bytes 4+) valid
 *
 */

static void xpad360w_process_packet(struct usb_xpad *xpad, u16 cmd, unsigned char *data)
{
	/* Presence change */
	if (data[0] & 0x08) {
		if (data[1] & 0x80) {
			
			if (!xpad->pad_present)
			{
				my_work_t * work;
				xpad->pad_present = 1;
				usb_submit_urb(xpad->bulk_out, GFP_ATOMIC);
				
				work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
				INIT_WORK( (struct work_struct *)work, my_wq_function );
				work->xpad = xpad;
				queue_work( my_wq, (struct work_struct *)work );
			}
			
		} else {
			if (xpad->pad_present)
			{
				my_work_t * work;
				xpad->pad_present = 0;

				work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
				INIT_WORK( (struct work_struct *)work, my_wq_function );
				work->xpad = xpad;
				queue_work( my_wq, (struct work_struct *)work );
			}
// 			printk("got kill packet for id %i\n", xpad->joydev_id);
		}
	}
	
// 	printk("xpad packet %hhX %hhX %hhX %hhX %hhX %hhX\n", data[0], data[1], data[2], data[3], data[4], data[5]);

	/* Valid pad data */
	if (!(data[1] & 0x1))
		return;

	xpad360_process_packet(xpad, cmd, &data[4]);
}

/*
 *	xpadone_process_buttons
 *
 *	Process a button update packet from an Xbox one controller.
 */
static void xpadone_process_buttons(struct usb_xpad *xpad,
				struct input_dev *dev,
				unsigned char *data)
{
	/* menu/view buttons */
	input_report_key(dev, BTN_START,  data[4] & 0x04);
	input_report_key(dev, BTN_SELECT, data[4] & 0x08);

	/* buttons A,B,X,Y */
	input_report_key(dev, BTN_A,	data[4] & 0x10);
	input_report_key(dev, BTN_B,	data[4] & 0x20);
	input_report_key(dev, BTN_X,	data[4] & 0x40);
	input_report_key(dev, BTN_Y,	data[4] & 0x80);

	/* digital pad */
	if (xpad->mapping & MAP_DPAD_TO_BUTTONS) {
		/* dpad as buttons (left, right, up, down) */
		input_report_key(dev, BTN_TRIGGER_HAPPY1, data[5] & 0x04);
		input_report_key(dev, BTN_TRIGGER_HAPPY2, data[5] & 0x08);
		input_report_key(dev, BTN_TRIGGER_HAPPY3, data[5] & 0x01);
		input_report_key(dev, BTN_TRIGGER_HAPPY4, data[5] & 0x02);
	} else {
		input_report_abs(dev, ABS_HAT0X,
				 !!(data[5] & 0x08) - !!(data[5] & 0x04));
		input_report_abs(dev, ABS_HAT0Y,
				 !!(data[5] & 0x02) - !!(data[5] & 0x01));
	}

	/* TL/TR */
	input_report_key(dev, BTN_TL,	data[5] & 0x10);
	input_report_key(dev, BTN_TR,	data[5] & 0x20);

	/* stick press left/right */
	input_report_key(dev, BTN_THUMBL, data[5] & 0x40);
	input_report_key(dev, BTN_THUMBR, data[5] & 0x80);

	if (!(xpad->mapping & MAP_STICKS_TO_NULL)) {
		/* left stick */
		input_report_abs(dev, ABS_X,
				 (__s16) le16_to_cpup((__le16 *)(data + 10)));
		input_report_abs(dev, ABS_Y,
				 ~(__s16) le16_to_cpup((__le16 *)(data + 12)));

		/* right stick */
		input_report_abs(dev, ABS_RX,
				 (__s16) le16_to_cpup((__le16 *)(data + 14)));
		input_report_abs(dev, ABS_RY,
				 ~(__s16) le16_to_cpup((__le16 *)(data + 16)));
	}

	/* triggers left/right */
	if (xpad->mapping & MAP_TRIGGERS_TO_BUTTONS) {
		input_report_key(dev, BTN_TL2,
				 (__u16) le16_to_cpup((__le16 *)(data + 6)));
		input_report_key(dev, BTN_TR2,
				 (__u16) le16_to_cpup((__le16 *)(data + 8)));
	} else {
		input_report_abs(dev, ABS_Z,
				 (__u16) le16_to_cpup((__le16 *)(data + 6)));
		input_report_abs(dev, ABS_RZ,
				 (__u16) le16_to_cpup((__le16 *)(data + 8)));
	}

	input_sync(dev);
}

/*
 *	xpadone_process_packet
 *
 *	Completes a request by converting the data into events for the
 *	input subsystem. This version is for the Xbox One controller.
 *
 *	The report format was gleaned from
 *	https://github.com/kylelemons/xbox/blob/master/xbox.go
 */

static void xpadone_process_packet(struct usb_xpad *xpad,
				u16 cmd, unsigned char *data)
{
	struct input_dev *dev = xpad->dev;

	switch (data[0]) {
	case 0x20:
		xpadone_process_buttons(xpad, dev, data);
		break;

	case 0x07:
		/* the xbox button has its own special report */
		input_report_key(dev, BTN_MODE, data[4] & 0x01);
		input_sync(dev);
		break;
	}
}

static void xpad_irq_in(struct urb *urb)
{
	struct usb_xpad *xpad = urb->context;
	struct device *dev = &xpad->intf->dev;
	int retval, status;

	status = urb->status;
	
// 	printk("xpad_irq_in %i\n", status);

	switch (status) {
	case 0:
		/* success */
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dev_dbg(dev, "%s - urb shutting down with status: %d\n",
			__func__, status);
		return;
	default:
		dev_dbg(dev, "%s - nonzero urb status received: %d\n",
			__func__, status);
		goto exit;
	}

	switch (xpad->xtype) {
	case XTYPE_XBOX360:
		xpad360_process_packet(xpad, 0, xpad->idata);
		break;
	case XTYPE_XBOX360W:
		xpad360w_process_packet(xpad, 0, xpad->idata);
		break;
	case XTYPE_XBOXONE:
		xpadone_process_packet(xpad, 0, xpad->idata);
		break;
	default:
		xpad_process_packet(xpad, 0, xpad->idata);
	}

exit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval)
		dev_err(dev, "%s - usb_submit_urb failed with result %d\n",
			__func__, retval);
}

static void xpad_bulk_out(struct urb *urb)
{
	struct usb_xpad *xpad = urb->context;
	struct device *dev = &xpad->intf->dev;

	switch (urb->status) {
	case 0:
		/* success */
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dev_dbg(dev, "%s - urb shutting down with status: %d\n",
			__func__, urb->status);
		break;
	default:
		dev_dbg(dev, "%s - nonzero urb status received: %d\n",
			__func__, urb->status);
	}
}

/* odata_lock must be held */
static void xpad_clear_odata_list(struct usb_xpad *xpad)
{
	struct list_head *n, *p;

	list_for_each_safe(p, n, &xpad->odata_list) {
		struct irq_out_data *out = list_entry(p, struct irq_out_data, list);
		list_del(&out->list);
		kfree(out);
	}
}

static void xpad_irq_out(struct urb *urb)
{
	struct usb_xpad *xpad = urb->context;
	struct device *dev = &xpad->intf->dev;
	int retval, status;
	unsigned long flags;
	struct irq_out_data *out;

	spin_lock_irqsave(&xpad->odata_lock, flags);

	status = urb->status;

	switch (status) {
	case 0:
		/* success, submit next packet from queue, if not empty */
		if (!list_empty(&xpad->odata_list)) {
			out = list_first_entry(&xpad->odata_list,
					       struct irq_out_data,
					       list);
			list_del(&out->list);
			memcpy(xpad->odata, out->odata, out->odata_length);
			urb->transfer_buffer_length = out->odata_length;
			kfree(out);
			goto resubmit;
		}
		xpad->irq_out_busy = 0;
		goto exit;

	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dev_dbg(dev, "%s - urb shutting down with status: %d\n",
			__func__, status);
		xpad_clear_odata_list(xpad);
		xpad->irq_out_busy = 0;
		// TODO: Clean odata_list ?
		goto exit;

	default:
		dev_dbg(dev, "%s - nonzero urb status received: %d\n",
			__func__, status);
		goto resubmit;
	}

resubmit:
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval)
		dev_err(dev, "%s - usb_submit_urb failed with result %d\n",
			__func__, retval);
exit:
	spin_unlock_irqrestore(&xpad->odata_lock, flags);
}

static int xpad_init_output(struct usb_interface *intf, struct usb_xpad *xpad)
{
	struct usb_endpoint_descriptor *ep_irq_out;
	int ep_irq_out_idx;
	int error;

	if (xpad->xtype == XTYPE_UNKNOWN)
		return 0;

	xpad->odata = usb_alloc_coherent(xpad->udev, XPAD_PKT_LEN,
					 GFP_KERNEL, &xpad->odata_dma);
	if (!xpad->odata) {
		error = -ENOMEM;
		goto fail1;
	}

	xpad->irq_out = usb_alloc_urb(0, GFP_KERNEL);
	if (!xpad->irq_out) {
		error = -ENOMEM;
		goto fail2;
	}

	INIT_LIST_HEAD(&xpad->odata_list);
	spin_lock_init(&xpad->odata_lock);

	/* Xbox One controller has in/out endpoints swapped. */
	ep_irq_out_idx = xpad->xtype == XTYPE_XBOXONE ? 0 : 1;
	ep_irq_out = &intf->cur_altsetting->endpoint[ep_irq_out_idx].desc;

	usb_fill_int_urb(xpad->irq_out, xpad->udev,
			 usb_sndintpipe(xpad->udev, ep_irq_out->bEndpointAddress),
			 xpad->odata, XPAD_PKT_LEN,
			 xpad_irq_out, xpad, ep_irq_out->bInterval);
	xpad->irq_out->transfer_dma = xpad->odata_dma;
	xpad->irq_out->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	return 0;

 fail2:	usb_free_coherent(xpad->udev, XPAD_PKT_LEN, xpad->odata, xpad->odata_dma);
 fail1:	return error;
}

static void xpad_stop_output(struct usb_xpad *xpad)
{
	unsigned long flags;

	if (xpad->xtype != XTYPE_UNKNOWN) {
		usb_kill_urb(xpad->irq_out);

		spin_lock_irqsave(&xpad->odata_lock, flags);

		/* Empty odata_list, in case URB was not pending */
		xpad_clear_odata_list(xpad);
		xpad->irq_out_busy = 0;

		spin_unlock_irqrestore(&xpad->odata_lock, flags);

		/* Stop rumble */
		xpad_send_ff(xpad, 0, 0);
	}
}

static void xpad_deinit_output(struct usb_xpad *xpad)
{
	if (xpad->xtype != XTYPE_UNKNOWN) {
		usb_free_urb(xpad->irq_out);
		usb_free_coherent(xpad->udev, XPAD_PKT_LEN,
				xpad->odata, xpad->odata_dma);
	}
}

/* xpad->odata_lock must be held */
static unsigned char* xpad_get_irq_out_buffer(struct usb_xpad *xpad)
{
	struct irq_out_data *out;

	if (xpad->irq_out_busy) {
		/* Allocate list item and add to the end of odata_list */
		out = kzalloc(sizeof(*out), GFP_ATOMIC);
		if (!out)
			return NULL;
		INIT_LIST_HEAD(&out->list);
		list_add_tail(&out->list, &xpad->odata_list);
		return out->odata;
	} else {
		return xpad->odata;
	}
}

/* xpad->odata_lock must be held */
static int xpad_submit_irq_out_buffer(struct usb_xpad *xpad, unsigned char* buf, u32 length)
{
	struct irq_out_data *out;

	if (xpad->irq_out_busy) {
		/* Get the last entry of the list */
		out = list_entry(xpad->odata_list.prev, struct irq_out_data, list);
		if (buf == out->odata) {
			out->odata_length = length;
		} else {
			dev_err(&xpad->intf->dev, "%s - bad odata_list item! %p %p\n",
				__func__, out, buf);
			return -EINVAL;
		}
	} else {
		if (buf == xpad->irq_out->transfer_buffer) {
			xpad->irq_out_busy = 1;
			xpad->irq_out->transfer_buffer_length = length;
			return usb_submit_urb(xpad->irq_out, GFP_ATOMIC);
		} else {
			dev_err(&xpad->intf->dev, "%s - bad irq_out odata! %p %p\n",
				__func__, xpad->irq_out->transfer_buffer, buf);
			return -EINVAL;
		}
	}
	return 0;
}

#ifdef CONFIG_JOYSTICK_XPAD_FF
static int xpad_send_ff(struct usb_xpad *xpad, int strong, int weak)
{
	unsigned char *odata;
	u32 transfer_length = 0;
	unsigned long flags;
	int error = 0;

	/* Check type, fall fast for unsupported types */
	switch (xpad->xtype) {
		case XTYPE_XBOX:
		case XTYPE_XBOX360:
		case XTYPE_XBOX360W:
		case XTYPE_XBOXONE:
			break;
		default:
			return -EINVAL;
	}

	spin_lock_irqsave(&xpad->odata_lock, flags);

	odata = xpad_get_irq_out_buffer(xpad);
	if (!odata) {
		spin_unlock_irqrestore(&xpad->odata_lock, flags);
		return -ENOMEM;
	}

	switch (xpad->xtype) {

		case XTYPE_XBOX:
			odata[0] = 0x00;
			odata[1] = 0x06;
			odata[2] = 0x00;
			odata[3] = strong / 256;	/* left actuator */
			odata[4] = 0x00;
			odata[5] = weak / 256;	/* right actuator */

			transfer_length = 6;
			break;

		case XTYPE_XBOX360:
			odata[0] = 0x00;
			odata[1] = 0x08;
			odata[2] = 0x00;
			odata[3] = strong / 256;  /* left actuator? */
			odata[4] = weak / 256;	/* right actuator? */
			odata[5] = 0x00;
			odata[6] = 0x00;
			odata[7] = 0x00;

			transfer_length = 8;
			break;

		case XTYPE_XBOX360W:
			odata[0] = 0x00;
			odata[1] = 0x01;
			odata[2] = 0x0F;
			odata[3] = 0xC0;
			odata[4] = 0x00;
			odata[5] = strong / 256;
			odata[6] = weak / 256;
			odata[7] = 0x00;
			odata[8] = 0x00;
			odata[9] = 0x00;
			odata[10] = 0x00;
			odata[11] = 0x00;

			transfer_length = 12;
			break;

		case XTYPE_XBOXONE:
			odata[0] = 0x09;
			odata[1] = 0x00;
			odata[2] = xpad->odata_serial++;
			odata[3] = 0x09;
			odata[4] = 0x00;
			odata[5] = 0x0F;
			odata[6] = 0x00;
			odata[7] = 0x00;
			odata[8] = strong / 512;
			odata[9] = weak / 512;
			odata[10] = 0xFF;
			odata[11] = 0x00;
			odata[12] = 0x00;

			transfer_length = 13;
			break;
	}

	error = xpad_submit_irq_out_buffer(xpad, odata, transfer_length);

	spin_unlock_irqrestore(&xpad->odata_lock, flags);
	return error;
}

static int xpad_play_effect(struct input_dev *dev, void *data, struct ff_effect *effect)
{
	struct usb_xpad *xpad = input_get_drvdata(dev);

	if (effect->type == FF_RUMBLE) {
		__u16 strong = effect->u.rumble.strong_magnitude;
		__u16 weak = effect->u.rumble.weak_magnitude;

		return xpad_send_ff(xpad, strong, weak);
	}

	return 0;
}

static int xpad_init_ff(struct usb_xpad *xpad)
{
	if (xpad->xtype == XTYPE_UNKNOWN)
		return 0;

	input_set_capability(xpad->dev, EV_FF, FF_RUMBLE);

	return input_ff_create_memless(xpad->dev, NULL, xpad_play_effect);
}

#else
static int xpad_init_ff(struct usb_xpad *xpad) { return 0; }
static int xpad_send_ff(struct usb_xpad *xpad, int strong, int weak) { return 0; }
#endif

#if defined(CONFIG_JOYSTICK_XPAD_LEDS)
#include <linux/leds.h>

struct xpad_led {
	char name[16];
	struct led_classdev led_cdev;
	struct usb_xpad *xpad;
};

static void xpad_send_led_command(struct usb_xpad *xpad, int command)
{
	unsigned char *odata;
	u32 transfer_length = 0;
	unsigned long flags;
	int error = 0;

	if ((unsigned)command > 15)
		return;

	spin_lock_irqsave(&xpad->odata_lock, flags);

	odata = xpad_get_irq_out_buffer(xpad);
	if (!odata) {
		spin_unlock_irqrestore(&xpad->odata_lock, flags);
		return;
	}

	switch (xpad->xtype) {
		
		case XTYPE_XBOX360:
			odata[0] = 0x01;
			odata[1] = 0x03;
			odata[2] = command;
			transfer_length = 3;
			break;

		case XTYPE_XBOX360W:
			odata[0] = 0x00;
			odata[1] = 0x00;
			odata[2] = 0x08;
			odata[3] = 0x40 + (command % 0x0e);
			odata[4] = 0x00;
			odata[5] = 0x00;
			odata[6] = 0x00;
			odata[7] = 0x00;
			odata[8] = 0x00;
			odata[9] = 0x00;
			odata[10] = 0x00;
			odata[11] = 0x00;
			transfer_length = 12;
			break;
	}

	if (transfer_length) {
		error = xpad_submit_irq_out_buffer(xpad, odata, transfer_length);
	}
	spin_unlock_irqrestore(&xpad->odata_lock, flags);
}

static void xpad_led_set(struct led_classdev *led_cdev,
			 enum led_brightness value)
{
	struct xpad_led *xpad_led = container_of(led_cdev,
						 struct xpad_led, led_cdev);

	xpad_send_led_command(xpad_led->xpad, value);
}

static int xpad_led_probe(struct usb_xpad *xpad)
{
	static atomic_t led_seq	= ATOMIC_INIT(0);
	long led_no;
	struct xpad_led *led;
	struct led_classdev *led_cdev;
	int error;
	
// 	printk("xpad_led_probe\n");

	if (xpad->xtype != XTYPE_XBOX360 && xpad->xtype != XTYPE_XBOX360W)
		return 0;

	xpad->led = led = kzalloc(sizeof(struct xpad_led), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	led_no = (long)atomic_inc_return(&led_seq) - 1;

	snprintf(led->name, sizeof(led->name), "xpad%ld", led_no);
	led->xpad = xpad;

	led_cdev = &led->led_cdev;
	led_cdev->name = led->name;
	led_cdev->brightness_set = xpad_led_set;

	error = led_classdev_register(&xpad->udev->dev, led_cdev);
	if (error) {
		kfree(led);
		xpad->led = NULL;
		return error;
	}

	return 0;
}

static void xpad_led_disconnect(struct usb_xpad *xpad)
{
	struct xpad_led *xpad_led = xpad->led;

	if (xpad_led) {
		led_classdev_unregister(&xpad_led->led_cdev);
		kfree(xpad_led);
	}
}
#else
static int xpad_led_probe(struct usb_xpad *xpad) { return 0; }
static void xpad_led_disconnect(struct usb_xpad *xpad) { }
#endif


static int xpad_open(struct input_dev *dev)
{
	unsigned long flags;
	int ret;
	unsigned char *odata;
	struct usb_xpad *xpad = input_get_drvdata(dev);
// 	printk("xpad open driver data %x\n", (unsigned int)xpad);

	/* URB was submitted in probe */
	if (xpad->xtype == XTYPE_XBOX360W)
		return 0;

	xpad->irq_in->dev = xpad->udev;
	if (usb_submit_urb(xpad->irq_in, GFP_KERNEL))
		return -EIO;

	if (xpad->xtype == XTYPE_XBOXONE) {
		spin_lock_irqsave(&xpad->odata_lock, flags);

		odata = xpad_get_irq_out_buffer(xpad);
		if (!odata) {
			spin_unlock_irqrestore(&xpad->odata_lock, flags);
			return -ENOMEM;
		}

		xpad->odata_serial = 0;
		/* Xbox one controller needs to be initialized. */
		odata[0] = 0x05;
		odata[1] = 0x20;
		odata[2] = xpad->odata_serial++; /* packet serial */
		odata[3] = 0x01; /* rumble bit enable?  */
		odata[4] = 0x00;
		ret = xpad_submit_irq_out_buffer(xpad, odata, 5);

		spin_unlock_irqrestore(&xpad->odata_lock, flags);
		return ret;
	}

	return 0;
}

static void xpad_close(struct input_dev *dev)
{
	struct usb_xpad *xpad = input_get_drvdata(dev);

	if (xpad->xtype != XTYPE_XBOX360W)
		usb_kill_urb(xpad->irq_in);

	xpad_stop_output(xpad);
}

static void xpad_set_up_abs(struct input_dev *input_dev, signed short abs)
{
	struct usb_xpad *xpad = input_get_drvdata(input_dev);
	set_bit(abs, input_dev->absbit);

	switch (abs) {
	case ABS_X:
	case ABS_Y:
	case ABS_RX:
	case ABS_RY:	/* the two sticks */
		input_set_abs_params(input_dev, abs, -32768, 32767, 16, 128);
		break;
	case ABS_Z:
	case ABS_RZ:	/* the triggers (if mapped to axes) */
		if (xpad->xtype == XTYPE_XBOXONE)
			input_set_abs_params(input_dev, abs, 0, 1023, 0, 0);
		else
			input_set_abs_params(input_dev, abs, 0, 255, 0, 0);
		break;
	case ABS_HAT0X:
	case ABS_HAT0Y:	/* the d-pad (only if dpad is mapped to axes */
		input_set_abs_params(input_dev, abs, -1, 1, 0, 0);
		break;
	}
}

static int xpad_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_xpad *xpad;
	struct usb_endpoint_descriptor *ep_irq_in;
	int ep_irq_in_idx;
	int i, error;
	unsigned long flags;
	unsigned char *odata;
	
	if (!my_wq) {
		my_wq = create_workqueue("xpad_queue");
	}

	for (i = 0; xpad_device[i].idVendor; i++) {
		if ((le16_to_cpu(udev->descriptor.idVendor) == xpad_device[i].idVendor) &&
		    (le16_to_cpu(udev->descriptor.idProduct) == xpad_device[i].idProduct))
			break;
	}
	
	if (xpad_device[i].xtype == XTYPE_XBOXONE &&
		intf->cur_altsetting->desc.bInterfaceNumber != 0) {
		/*
		 * The Xbox One controller lists three interfaces all with the
		 * same interface class, subclass and protocol. Differentiate by
		 * interface number.
		 */
		return -ENODEV;
	}

	xpad = kzalloc(sizeof(struct usb_xpad), GFP_KERNEL);

	xpad->name = xpad_device[i].name;
	
	xpad->idata = usb_alloc_coherent(udev, XPAD_PKT_LEN,
					 GFP_KERNEL, &xpad->idata_dma);
	if (!xpad->idata) {
		error = -ENOMEM;
		goto fail1;
	}

	xpad->irq_in = usb_alloc_urb(0, GFP_KERNEL);
	if (!xpad->irq_in) {
		error = -ENOMEM;
		goto fail2;
	}

	xpad->udev = udev;
	xpad->intf = intf;
	xpad->mapping = xpad_device[i].mapping;
	xpad->xtype = xpad_device[i].xtype;

	if (xpad->xtype == XTYPE_UNKNOWN) {
		if (intf->cur_altsetting->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC) {
			if (intf->cur_altsetting->desc.bInterfaceProtocol == 129)
				xpad->xtype = XTYPE_XBOX360W;
			else
				xpad->xtype = XTYPE_XBOX360;
		} else
			xpad->xtype = XTYPE_XBOX;

		if (dpad_to_buttons)
			xpad->mapping |= MAP_DPAD_TO_BUTTONS;
		if (triggers_to_buttons)
			xpad->mapping |= MAP_TRIGGERS_TO_BUTTONS;
		if (sticks_to_null)
			xpad->mapping |= MAP_STICKS_TO_NULL;
	}

	error = xpad_init_output(intf, xpad);
	if (error)
		goto fail3;

	usb_make_path(xpad->udev, xpad->phys, sizeof(xpad->phys));
	strlcat(xpad->phys, "/input0", sizeof(xpad->phys));

	error = xpad_led_probe(xpad);
	if (error)
		goto fail5;

	/* Xbox One controller has in/out endpoints swapped. */
	ep_irq_in_idx = xpad->xtype == XTYPE_XBOXONE ? 1 : 0;
	ep_irq_in = &intf->cur_altsetting->endpoint[ep_irq_in_idx].desc;

	usb_fill_int_urb(xpad->irq_in, udev,
			 usb_rcvintpipe(udev, ep_irq_in->bEndpointAddress),
			 xpad->idata, XPAD_PKT_LEN, xpad_irq_in,
			 xpad, ep_irq_in->bInterval);
	xpad->irq_in->transfer_dma = xpad->idata_dma;
	xpad->irq_in->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	usb_set_intfdata(intf, xpad);

	if (xpad->xtype == XTYPE_XBOX360W) {
		/*
		 * Setup the message to set the LEDs on the
		 * controller when it shows up
		 */
		spin_lock_irqsave(&xpad->odata_lock, flags);
		xpad->bulk_out = usb_alloc_urb(0, GFP_KERNEL);
		if (!xpad->bulk_out) {
			error = -ENOMEM;
			goto fail7;
		}

		xpad->bdata = kzalloc(XPAD_PKT_LEN, GFP_KERNEL);
		if (!xpad->bdata) {
			error = -ENOMEM;
			goto fail8;
		}

		xpad->bdata[2] = 0x08;
		switch (intf->cur_altsetting->desc.bInterfaceNumber) {
		case 0:
			xpad->bdata[3] = 0x42;
			break;
		case 2:
			xpad->bdata[3] = 0x43;
			break;
		case 4:
			xpad->bdata[3] = 0x44;
			break;
		case 6:
			xpad->bdata[3] = 0x45;
		}

		ep_irq_in = &intf->cur_altsetting->endpoint[1].desc;
		usb_fill_bulk_urb(xpad->bulk_out, udev,
				usb_sndbulkpipe(udev, ep_irq_in->bEndpointAddress),
				xpad->bdata, XPAD_PKT_LEN, xpad_bulk_out, xpad);

		/*
		 * Submit the int URB immediately rather than waiting for open
		 * because we get status messages from the device whether
		 * or not any controllers are attached.  In fact, it's
		 * exactly the message that a controller has arrived that
		 * we're waiting for.
		 */
		xpad->irq_in->dev = xpad->udev;
		error = usb_submit_urb(xpad->irq_in, GFP_KERNEL);
		
		spin_unlock_irqrestore(&xpad->odata_lock, flags);
		if (error)
			goto fail9;
		
		// I don't know how to check controller state on driver load so just slam them
		// off so that people have to turn them on, triggering a state update
		
		// got the power off packet from an OSX reverse-engineered driver:
		// http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/OsxDriver#toc1
		spin_lock_irqsave(&xpad->odata_lock, flags);

		odata = xpad_get_irq_out_buffer(xpad);
		if (odata) {
			odata[0] = 0x00;
			odata[1] = 0x00;
			odata[2] = 0x08;
			odata[3] = 0xC0;
			odata[4] = 0x00;
			odata[5] = 0x00;
			odata[6] = 0x00;
			odata[7] = 0x00;
			odata[8] = 0x00;
			odata[9] = 0x00;
			odata[10] = 0x00;
			odata[11] = 0x00;
			xpad_submit_irq_out_buffer(xpad, odata, 12);
		}
		spin_unlock_irqrestore(&xpad->odata_lock, flags);
	} else {
		my_work_t *work;
		xpad->pad_present = 1;
		
		work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
		INIT_WORK( (struct work_struct *)work, my_wq_function );
		work->xpad = xpad;
		queue_work( my_wq, (struct work_struct *)work );
	}

	return 0;

 fail9:	kfree(xpad->bdata);
 fail8:	usb_free_urb(xpad->bulk_out);
 fail7:	//input_unregister_device(input_dev);
	//input_dev = NULL;
// fail6:
	xpad_led_disconnect(xpad);
 fail5:	//if (input_dev)
		//input_ff_destroy(input_dev);
// fail4:
	xpad_deinit_output(xpad);
 fail3:	usb_free_urb(xpad->irq_in);
 fail2:	usb_free_coherent(udev, XPAD_PKT_LEN, xpad->idata, xpad->idata_dma);
 fail1:	//input_free_device(input_dev);
	kfree(xpad);
	return error;

}

static void xpad_disconnect(struct usb_interface *intf)
{
	struct usb_xpad *xpad = usb_get_intfdata (intf);

// 	printk("xpad_disconnect\n");
	xpad_led_disconnect(xpad);
	
	if (xpad->pad_present)
	{
		xpad->pad_present = 0;
		input_unregister_device(xpad->dev);
	}
	xpad_deinit_output(xpad);

	if (xpad->xtype == XTYPE_XBOX360W) {
		usb_kill_urb(xpad->bulk_out);
		usb_free_urb(xpad->bulk_out);
		usb_kill_urb(xpad->irq_in);
	}

	usb_free_urb(xpad->irq_in);
	usb_free_coherent(xpad->udev, XPAD_PKT_LEN,
			xpad->idata, xpad->idata_dma);

	kfree(xpad->bdata);
	kfree(xpad);

	usb_set_intfdata(intf, NULL);
}

static struct usb_driver xpad_driver = {
	.name		= "xpad",
	.probe		= xpad_probe,
	.disconnect	= xpad_disconnect,
	.id_table	= xpad_table,
};

module_usb_driver(xpad_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
