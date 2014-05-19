
CC = gcc
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
PWD := $(shell pwd)

EXTRA_CFLAGS += -O2
#EXTRA_CFLAGS += -DRTK_FW_HDR

####################################################
obj-m := btcoexist.o


BT_COEXIST_OBJS:=	halbtc8192e2ant.o\
			halbtc8723b1ant.o\
			halbtc8723b2ant.o\
			halbtc8821a2ant.o\
			halbtc8821a1ant.o\
			halbtcoutsrc.o\
			rtl_btc.o	\

####################################################

btcoexist-objs += $(BT_COEXIST_OBJS)

default:
	        $(MAKE) -C $(KSRC) M=$(PWD) modules

.PHONY:clean
clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
	rm -fr Modules.symvers
	rm -fr Module.symvers
	rm -fr Module.markers
	rm -fr modules.order
	rm -fr tags

