
CC = gcc
KVER  := $(shell uname -r)
#KSRC := /lib/modules/$(KVER)/build
#MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/rtlwifi
#FIRMWAREDIR := /lib/firmware/
#PWD := $(shell pwd)
#CLR_MODULE_FILES := *.mod.c *.mod *.o .*.cmd *.ko *~ .tmp_versions* modules.order Module.symvers
#SYMBOL_FILE := Module.symvers

#EXTRA_CFLAGS += -O2
#obj-m += rtlwifi.o
PCI_MAIN_OBJS	:= base.o	\
		rc.o	\
		debug.o	\
		regd.o	\
		efuse.o	\
		cam.o	\
		ps.o	\
		core.o	\
		stats.o	\
		pci.o	\

#rtlwifi-objs += $(PCI_MAIN_OBJS)

#obj-m += btcoexist.o
BT_COEXIST_OBJS:=	btcoexist/halbtc8192e2ant.o\
			btcoexist/halbtc8723b1ant.o\
			btcoexist/halbtc8723b2ant.o\
			btcoexist/halbtcoutsrc.o\
			btcoexist/rtl_btc.o	\

#btcoexist-objs += $(BT_COEXIST_OBJS)

obj-m += rtl8821ae.o

PCI_8821AE_HAL_OBJS:=	\
	rtl8821ae/hw.o		\
	rtl8821ae/table.o		\
	rtl8821ae/sw.o		\
	rtl8821ae/trx.o		\
	rtl8821ae/led.o		\
	rtl8821ae/fw.o		\
	rtl8821ae/phy.o		\
	rtl8821ae/rf.o		\
	rtl8821ae/dm.o		\
	rtl8821ae/pwrseq.o	\
	rtl8821ae/pwrseqcmd.o	\
	rtl8821ae/hal_btc.o	\
	rtl8821ae/hal_bt_coexist.o	\

rtl8821ae-objs += $(BT_COEXIST_OBJS) $(PCI_MAIN_OBJS) $(PCI_8821AE_HAL_OBJS) 



all: 
	$(MAKE) -C $(KSRC) M=$(PWD) modules
	@cp $(SYMBOL_FILE) btcoexist/
	@make -C btcoexist/
#	@cp $(SYMBOL_FILE) rtl8192ce/
#	@make -C rtl8192ce/
#	@cp $(SYMBOL_FILE) rtl8192se/
#	@make -C rtl8192se/
#	@cp $(SYMBOL_FILE) rtl8192de/
#	@make -C rtl8192de/
#	@cp $(SYMBOL_FILE) rtl8723e/
#	@make -C rtl8723e/
#	@cp $(SYMBOL_FILE) rtl8188ee/
#	@make -C rtl8188ee/
#	@cp $(SYMBOL_FILE) rtl8723be/
#	@cp btcoexist/$(SYMBOL_FILE) rtl8723be/
#	@make -C rtl8723be/
#	@cp $(SYMBOL_FILE) rtl8192ee/
#	@cp btcoexist/$(SYMBOL_FILE) rtl8192ee/
#	@make -C rtl8192ee/
	@cp $(SYMBOL_FILE) rtl8821ae/
	cp btcoexist/$(SYMBOL_FILE) rtl8821ae/
	@make -C rtl8821ae/
