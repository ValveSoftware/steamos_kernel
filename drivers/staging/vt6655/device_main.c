/*
 * Copyright (c) 1996, 2003 VIA Networking Technologies, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * File: device_main.c
 *
 * Purpose: driver entry for initial, open, close, tx and rx.
 *
 * Author: Lyndon Chen
 *
 * Date: Jan 8, 2003
 *
 * Functions:
 *
 *   vt6655_probe - module initial (insmod) driver entry
 *   vt6655_remove - module remove entry
 *   vt6655_init_info - device structure resource allocation function
 *   device_free_info - device structure resource free function
 *   device_get_pci_info - get allocated pci io/mem resource
 *   device_print_info - print out resource
 *   device_open - allocate dma/descripter resource & initial mac/bbp function
 *   device_xmit - asynchrous data tx function
 *   device_intr - interrupt handle function
 *   device_set_multi - set mac filter
 *   device_ioctl - ioctl entry
 *   device_close - shutdown mac/bbp & free dma/descripter resource
 *   device_rx_srv - rx service function
 *   device_receive_frame - rx data function
 *   device_alloc_rx_buf - rx buffer pre-allocated function
 *   device_alloc_frag_buf - rx fragement pre-allocated function
 *   device_free_tx_buf - free tx buffer function
 *   device_free_frag_buf- free de-fragement buffer
 *   device_dma0_tx_80211- tx 802.11 frame via dma0
 *   device_dma0_xmit- tx PS bufferred frame via dma0
 *   device_init_rd0_ring- initial rd dma0 ring
 *   device_init_rd1_ring- initial rd dma1 ring
 *   device_init_td0_ring- initial tx dma0 ring buffer
 *   device_init_td1_ring- initial tx dma1 ring buffer
 *   device_init_registers- initial MAC & BBP & RF internal registers.
 *   device_init_rings- initial tx/rx ring buffer
 *   device_init_defrag_cb- initial & allocate de-fragement buffer.
 *   device_free_rings- free all allocated ring buffer
 *   device_tx_srv- tx interrupt service function
 *
 * Revision History:
 */
#undef __NO_VERSION__

#include <linux/file.h>
#include "device.h"
#include "card.h"
#include "channel.h"
#include "baseband.h"
#include "mac.h"
#include "tether.h"
#include "wmgr.h"
#include "wctl.h"
#include "power.h"
#include "wcmd.h"
#include "iocmd.h"
#include "tcrc.h"
#include "rxtx.h"
#include "wroute.h"
#include "bssdb.h"
#include "hostap.h"
#include "wpactl.h"
#include "ioctl.h"
#include "iwctl.h"
#include "dpc.h"
#include "datarate.h"
#include "rf.h"
#include "iowpa.h"
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/slab.h>

/*---------------------  Static Definitions -------------------------*/
//
// Define module options
//
MODULE_AUTHOR("VIA Networking Technologies, Inc., <lyndonchen@vntek.com.tw>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VIA Networking Solomon-A/B/G Wireless LAN Adapter Driver");

#define DEVICE_PARAM(N, D)

#define RX_DESC_MIN0     16
#define RX_DESC_MAX0     128
#define RX_DESC_DEF0     32
DEVICE_PARAM(RxDescriptors0, "Number of receive descriptors0");

#define RX_DESC_MIN1     16
#define RX_DESC_MAX1     128
#define RX_DESC_DEF1     32
DEVICE_PARAM(RxDescriptors1, "Number of receive descriptors1");

#define TX_DESC_MIN0     16
#define TX_DESC_MAX0     128
#define TX_DESC_DEF0     32
DEVICE_PARAM(TxDescriptors0, "Number of transmit descriptors0");

#define TX_DESC_MIN1     16
#define TX_DESC_MAX1     128
#define TX_DESC_DEF1     64
DEVICE_PARAM(TxDescriptors1, "Number of transmit descriptors1");

#define IP_ALIG_DEF     0
/* IP_byte_align[] is used for IP header unsigned long byte aligned
   0: indicate the IP header won't be unsigned long byte aligned.(Default) .
   1: indicate the IP header will be unsigned long byte aligned.
   In some environment, the IP header should be unsigned long byte aligned,
   or the packet will be droped when we receive it. (eg: IPVS)
*/
DEVICE_PARAM(IP_byte_align, "Enable IP header dword aligned");

#define INT_WORKS_DEF   20
#define INT_WORKS_MIN   10
#define INT_WORKS_MAX   64

DEVICE_PARAM(int_works, "Number of packets per interrupt services");

#define CHANNEL_MIN     1
#define CHANNEL_MAX     14
#define CHANNEL_DEF     6

DEVICE_PARAM(Channel, "Channel number");

/* PreambleType[] is the preamble length used for transmit.
   0: indicate allows long preamble type
   1: indicate allows short preamble type
*/

#define PREAMBLE_TYPE_DEF     1

DEVICE_PARAM(PreambleType, "Preamble Type");

#define RTS_THRESH_MIN     512
#define RTS_THRESH_MAX     2347
#define RTS_THRESH_DEF     2347

DEVICE_PARAM(RTSThreshold, "RTS threshold");

#define FRAG_THRESH_MIN     256
#define FRAG_THRESH_MAX     2346
#define FRAG_THRESH_DEF     2346

DEVICE_PARAM(FragThreshold, "Fragmentation threshold");

#define DATA_RATE_MIN     0
#define DATA_RATE_MAX     13
#define DATA_RATE_DEF     13
/* datarate[] index
   0: indicate 1 Mbps   0x02
   1: indicate 2 Mbps   0x04
   2: indicate 5.5 Mbps 0x0B
   3: indicate 11 Mbps  0x16
   4: indicate 6 Mbps   0x0c
   5: indicate 9 Mbps   0x12
   6: indicate 12 Mbps  0x18
   7: indicate 18 Mbps  0x24
   8: indicate 24 Mbps  0x30
   9: indicate 36 Mbps  0x48
   10: indicate 48 Mbps  0x60
   11: indicate 54 Mbps  0x6c
   12: indicate 72 Mbps  0x90
   13: indicate auto rate
*/

DEVICE_PARAM(ConnectionRate, "Connection data rate");

#define OP_MODE_DEF     0

DEVICE_PARAM(OPMode, "Infrastruct, adhoc, AP mode ");

/* OpMode[] is used for transmit.
   0: indicate infrastruct mode used
   1: indicate adhoc mode used
   2: indicate AP mode used
*/

/* PSMode[]
   0: indicate disable power saving mode
   1: indicate enable power saving mode
*/

#define PS_MODE_DEF     0

DEVICE_PARAM(PSMode, "Power saving mode");

#define SHORT_RETRY_MIN     0
#define SHORT_RETRY_MAX     31
#define SHORT_RETRY_DEF     8

DEVICE_PARAM(ShortRetryLimit, "Short frame retry limits");

#define LONG_RETRY_MIN     0
#define LONG_RETRY_MAX     15
#define LONG_RETRY_DEF     4

DEVICE_PARAM(LongRetryLimit, "long frame retry limits");

/* BasebandType[] baseband type selected
   0: indicate 802.11a type
   1: indicate 802.11b type
   2: indicate 802.11g type
*/
#define BBP_TYPE_MIN     0
#define BBP_TYPE_MAX     2
#define BBP_TYPE_DEF     2

DEVICE_PARAM(BasebandType, "baseband type");

/* 80211hEnable[]
   0: indicate disable 802.11h
   1: indicate enable 802.11h
*/

#define X80211h_MODE_DEF     0

DEVICE_PARAM(b80211hEnable, "802.11h mode");

/* 80211hEnable[]
   0: indicate disable 802.11h
   1: indicate enable 802.11h
*/

#define DIVERSITY_ANT_DEF     0

DEVICE_PARAM(bDiversityANTEnable, "ANT diversity mode");

//
// Static vars definitions
//
static CHIP_INFO chip_info_table[] = {
	{ VT3253,       "VIA Networking Solomon-A/B/G Wireless LAN Adapter ",
	  256, 1,     DEVICE_FLAGS_IP_ALIGN|DEVICE_FLAGS_TX_ALIGN },
	{0, NULL}
};

static const struct pci_device_id vt6655_pci_id_table[] = {
	{ PCI_VDEVICE(VIA, 0x3253), (kernel_ulong_t)chip_info_table},
	{ 0, }
};

/*---------------------  Static Functions  --------------------------*/

static int  vt6655_probe(struct pci_dev *pcid, const struct pci_device_id *ent);
static void vt6655_init_info(struct pci_dev *pcid,
			     struct vnt_private **ppDevice, PCHIP_INFO);
static void device_free_info(struct vnt_private *pDevice);
static bool device_get_pci_info(struct vnt_private *, struct pci_dev *pcid);
static void device_print_info(struct vnt_private *pDevice);
static void device_init_diversity_timer(struct vnt_private *pDevice);
static int  device_open(struct net_device *dev);
static int  device_xmit(struct sk_buff *skb, struct net_device *dev);
static  irqreturn_t  device_intr(int irq,  void *dev_instance);
static void device_set_multi(struct net_device *dev);
static int  device_close(struct net_device *dev);
static int  device_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);

#ifdef CONFIG_PM
static int device_notify_reboot(struct notifier_block *, unsigned long event, void *ptr);
static int viawget_suspend(struct pci_dev *pcid, pm_message_t state);
static int viawget_resume(struct pci_dev *pcid);
static struct notifier_block device_notifier = {
	.notifier_call = device_notify_reboot,
	.next = NULL,
	.priority = 0,
};
#endif

static void device_init_rd0_ring(struct vnt_private *pDevice);
static void device_init_rd1_ring(struct vnt_private *pDevice);
static void device_init_defrag_cb(struct vnt_private *pDevice);
static void device_init_td0_ring(struct vnt_private *pDevice);
static void device_init_td1_ring(struct vnt_private *pDevice);

static int  device_dma0_tx_80211(struct sk_buff *skb, struct net_device *dev);
//2008-0714<Add>by Mike Liu
static bool device_release_WPADEV(struct vnt_private *pDevice);

static int  ethtool_ioctl(struct net_device *dev, void __user *useraddr);
static int  device_rx_srv(struct vnt_private *pDevice, unsigned int uIdx);
static int  device_tx_srv(struct vnt_private *pDevice, unsigned int uIdx);
static bool device_alloc_rx_buf(struct vnt_private *pDevice, PSRxDesc pDesc);
static void device_init_registers(struct vnt_private *pDevice);
static void device_free_tx_buf(struct vnt_private *pDevice, PSTxDesc pDesc);
static void device_free_td0_ring(struct vnt_private *pDevice);
static void device_free_td1_ring(struct vnt_private *pDevice);
static void device_free_rd0_ring(struct vnt_private *pDevice);
static void device_free_rd1_ring(struct vnt_private *pDevice);
static void device_free_rings(struct vnt_private *pDevice);
static void device_free_frag_buf(struct vnt_private *pDevice);
static int Config_FileGetParameter(unsigned char *string,
				   unsigned char *dest, unsigned char *source);

/*---------------------  Export Variables  --------------------------*/

/*---------------------  Export Functions  --------------------------*/

static char *get_chip_name(int chip_id)
{
	int i;

	for (i = 0; chip_info_table[i].name != NULL; i++)
		if (chip_info_table[i].chip_id == chip_id)
			break;
	return chip_info_table[i].name;
}

static void vt6655_remove(struct pci_dev *pcid)
{
	struct vnt_private *pDevice = pci_get_drvdata(pcid);

	if (pDevice == NULL)
		return;
	device_free_info(pDevice);
}

static void device_get_options(struct vnt_private *pDevice)
{
	POPTIONS pOpts = &(pDevice->sOpts);

	pOpts->nRxDescs0 = RX_DESC_DEF0;
	pOpts->nRxDescs1 = RX_DESC_DEF1;
	pOpts->nTxDescs[0] = TX_DESC_DEF0;
	pOpts->nTxDescs[1] = TX_DESC_DEF1;
	pOpts->flags |= DEVICE_FLAGS_IP_ALIGN;
	pOpts->int_works = INT_WORKS_DEF;
	pOpts->rts_thresh = RTS_THRESH_DEF;
	pOpts->frag_thresh = FRAG_THRESH_DEF;
	pOpts->data_rate = DATA_RATE_DEF;
	pOpts->channel_num = CHANNEL_DEF;

	pOpts->flags |= DEVICE_FLAGS_PREAMBLE_TYPE;
	pOpts->flags |= DEVICE_FLAGS_OP_MODE;
	pOpts->short_retry = SHORT_RETRY_DEF;
	pOpts->long_retry = LONG_RETRY_DEF;
	pOpts->bbp_type = BBP_TYPE_DEF;
	pOpts->flags |= DEVICE_FLAGS_80211h_MODE;
	pOpts->flags |= DEVICE_FLAGS_DiversityANT;
}

static void
device_set_options(struct vnt_private *pDevice)
{
	unsigned char abyBroadcastAddr[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	unsigned char abySNAP_RFC1042[ETH_ALEN] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00};
	unsigned char abySNAP_Bridgetunnel[ETH_ALEN] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0xF8};

	memcpy(pDevice->abyBroadcastAddr, abyBroadcastAddr, ETH_ALEN);
	memcpy(pDevice->abySNAP_RFC1042, abySNAP_RFC1042, ETH_ALEN);
	memcpy(pDevice->abySNAP_Bridgetunnel, abySNAP_Bridgetunnel, ETH_ALEN);

	pDevice->uChannel = pDevice->sOpts.channel_num;
	pDevice->wRTSThreshold = pDevice->sOpts.rts_thresh;
	pDevice->wFragmentationThreshold = pDevice->sOpts.frag_thresh;
	pDevice->byShortRetryLimit = pDevice->sOpts.short_retry;
	pDevice->byLongRetryLimit = pDevice->sOpts.long_retry;
	pDevice->wMaxTransmitMSDULifetime = DEFAULT_MSDU_LIFETIME;
	pDevice->byShortPreamble = (pDevice->sOpts.flags & DEVICE_FLAGS_PREAMBLE_TYPE) ? 1 : 0;
	pDevice->byOpMode = (pDevice->sOpts.flags & DEVICE_FLAGS_OP_MODE) ? 1 : 0;
	pDevice->ePSMode = (pDevice->sOpts.flags & DEVICE_FLAGS_PS_MODE) ? 1 : 0;
	pDevice->b11hEnable = (pDevice->sOpts.flags & DEVICE_FLAGS_80211h_MODE) ? 1 : 0;
	pDevice->bDiversityRegCtlON = (pDevice->sOpts.flags & DEVICE_FLAGS_DiversityANT) ? 1 : 0;
	pDevice->uConnectionRate = pDevice->sOpts.data_rate;
	if (pDevice->uConnectionRate < RATE_AUTO)
		pDevice->bFixRate = true;
	pDevice->byBBType = pDevice->sOpts.bbp_type;
	pDevice->byPacketType = (VIA_PKT_TYPE)pDevice->byBBType;
	pDevice->byAutoFBCtrl = AUTO_FB_0;
	pDevice->bUpdateBBVGA = true;
	pDevice->byFOETuning = 0;
	pDevice->byPreambleType = 0;

	pr_debug(" uChannel= %d\n", (int)pDevice->uChannel);
	pr_debug(" byOpMode= %d\n", (int)pDevice->byOpMode);
	pr_debug(" ePSMode= %d\n", (int)pDevice->ePSMode);
	pr_debug(" wRTSThreshold= %d\n", (int)pDevice->wRTSThreshold);
	pr_debug(" byShortRetryLimit= %d\n", (int)pDevice->byShortRetryLimit);
	pr_debug(" byLongRetryLimit= %d\n", (int)pDevice->byLongRetryLimit);
	pr_debug(" byPreambleType= %d\n", (int)pDevice->byPreambleType);
	pr_debug(" byShortPreamble= %d\n", (int)pDevice->byShortPreamble);
	pr_debug(" uConnectionRate= %d\n", (int)pDevice->uConnectionRate);
	pr_debug(" byBBType= %d\n", (int)pDevice->byBBType);
	pr_debug(" pDevice->b11hEnable= %d\n", (int)pDevice->b11hEnable);
	pr_debug(" pDevice->bDiversityRegCtlON= %d\n",
		 (int)pDevice->bDiversityRegCtlON);
}

static void s_vCompleteCurrentMeasure(struct vnt_private *pDevice,
				      unsigned char byResult)
{
	unsigned int ii;
	unsigned long dwDuration = 0;
	unsigned char byRPI0 = 0;

	for (ii = 1; ii < 8; ii++) {
		pDevice->dwRPIs[ii] *= 255;
		dwDuration |= *((unsigned short *)(pDevice->pCurrMeasureEID->sReq.abyDuration));
		dwDuration <<= 10;
		pDevice->dwRPIs[ii] /= dwDuration;
		pDevice->abyRPIs[ii] = (unsigned char)pDevice->dwRPIs[ii];
		byRPI0 += pDevice->abyRPIs[ii];
	}
	pDevice->abyRPIs[0] = (0xFF - byRPI0);

	if (pDevice->uNumOfMeasureEIDs == 0) {
		VNTWIFIbMeasureReport(pDevice->pMgmt,
				      true,
				      pDevice->pCurrMeasureEID,
				      byResult,
				      pDevice->byBasicMap,
				      pDevice->byCCAFraction,
				      pDevice->abyRPIs
			);
	} else {
		VNTWIFIbMeasureReport(pDevice->pMgmt,
				      false,
				      pDevice->pCurrMeasureEID,
				      byResult,
				      pDevice->byBasicMap,
				      pDevice->byCCAFraction,
				      pDevice->abyRPIs
			);
		CARDbStartMeasure(pDevice, pDevice->pCurrMeasureEID++, pDevice->uNumOfMeasureEIDs);
	}
}

//
// Initialisation of MAC & BBP registers
//

static void device_init_registers(struct vnt_private *pDevice)
{
	unsigned int ii;
	unsigned char byValue;
	unsigned char byValue1;
	unsigned char byCCKPwrdBm = 0;
	unsigned char byOFDMPwrdBm = 0;
	int zonetype = 0;
	PSMgmtObject    pMgmt = &(pDevice->sMgmtObj);

	MACbShutdown(pDevice->PortOffset);
	BBvSoftwareReset(pDevice->PortOffset);

	/* Do MACbSoftwareReset in MACvInitialize */
	MACbSoftwareReset(pDevice->PortOffset);

	pDevice->bAES = false;

	/* Only used in 11g type, sync with ERP IE */
	pDevice->bProtectMode = false;

	pDevice->bNonERPPresent = false;
	pDevice->bBarkerPreambleMd = false;
	pDevice->wCurrentRate = RATE_1M;
	pDevice->byTopOFDMBasicRate = RATE_24M;
	pDevice->byTopCCKBasicRate = RATE_1M;

	/* Target to IF pin while programming to RF chip. */
	pDevice->byRevId = 0;

	/* init MAC */
	MACvInitialize(pDevice->PortOffset);

	/* Get Local ID */
	VNSvInPortB(pDevice->PortOffset + MAC_REG_LOCALID, &pDevice->byLocalID);

	spin_lock_irq(&pDevice->lock);

	SROMvReadAllContents(pDevice->PortOffset, pDevice->abyEEPROM);

	spin_unlock_irq(&pDevice->lock);

	/* Get Channel range */
	pDevice->byMinChannel = 1;
	pDevice->byMaxChannel = CB_MAX_CHANNEL;

	/* Get Antena */
	byValue = SROMbyReadEmbedded(pDevice->PortOffset, EEP_OFS_ANTENNA);
	if (byValue & EEP_ANTINV)
		pDevice->bTxRxAntInv = true;
	else
		pDevice->bTxRxAntInv = false;

	byValue &= (EEP_ANTENNA_AUX | EEP_ANTENNA_MAIN);
	/* if not set default is All */
	if (byValue == 0)
		byValue = (EEP_ANTENNA_AUX | EEP_ANTENNA_MAIN);

	pDevice->ulDiversityNValue = 100*260;
	pDevice->ulDiversityMValue = 100*16;
	pDevice->byTMax = 1;
	pDevice->byTMax2 = 4;
	pDevice->ulSQ3TH = 0;
	pDevice->byTMax3 = 64;

	if (byValue == (EEP_ANTENNA_AUX | EEP_ANTENNA_MAIN)) {
		pDevice->byAntennaCount = 2;
		pDevice->byTxAntennaMode = ANT_B;
		pDevice->dwTxAntennaSel = 1;
		pDevice->dwRxAntennaSel = 1;

		if (pDevice->bTxRxAntInv)
			pDevice->byRxAntennaMode = ANT_A;
		else
			pDevice->byRxAntennaMode = ANT_B;

		byValue1 = SROMbyReadEmbedded(pDevice->PortOffset,
					      EEP_OFS_ANTENNA);

		if ((byValue1 & 0x08) == 0)
			pDevice->bDiversityEnable = false;
		else
			pDevice->bDiversityEnable = true;
	} else  {
		pDevice->bDiversityEnable = false;
		pDevice->byAntennaCount = 1;
		pDevice->dwTxAntennaSel = 0;
		pDevice->dwRxAntennaSel = 0;

		if (byValue & EEP_ANTENNA_AUX) {
			pDevice->byTxAntennaMode = ANT_A;

			if (pDevice->bTxRxAntInv)
				pDevice->byRxAntennaMode = ANT_B;
			else
				pDevice->byRxAntennaMode = ANT_A;
		} else {
			pDevice->byTxAntennaMode = ANT_B;

			if (pDevice->bTxRxAntInv)
				pDevice->byRxAntennaMode = ANT_A;
			else
				pDevice->byRxAntennaMode = ANT_B;
		}
	}

	pr_debug("bDiversityEnable=[%d],NValue=[%d],MValue=[%d],TMax=[%d],TMax2=[%d]\n",
		 pDevice->bDiversityEnable, (int)pDevice->ulDiversityNValue,
		 (int)pDevice->ulDiversityMValue, pDevice->byTMax,
		 pDevice->byTMax2);

	/* zonetype initial */
	pDevice->byOriginalZonetype = pDevice->abyEEPROM[EEP_OFS_ZONETYPE];
	zonetype = Config_FileOperation(pDevice, false, NULL);

	if (zonetype >= 0) {
		if ((zonetype == 0) &&
		    (pDevice->abyEEPROM[EEP_OFS_ZONETYPE] != 0x00)) {
			/* for USA */
			pDevice->abyEEPROM[EEP_OFS_ZONETYPE] = 0;
			pDevice->abyEEPROM[EEP_OFS_MAXCHANNEL] = 0x0B;

			pr_debug("Init Zone Type :USA\n");
		} else if ((zonetype == 1) &&
			 (pDevice->abyEEPROM[EEP_OFS_ZONETYPE] != 0x01)) {
			/* for Japan */
			pDevice->abyEEPROM[EEP_OFS_ZONETYPE] = 0x01;
			pDevice->abyEEPROM[EEP_OFS_MAXCHANNEL] = 0x0D;
		} else if ((zonetype == 2) &&
			  (pDevice->abyEEPROM[EEP_OFS_ZONETYPE] != 0x02)) {
			/* for Europe */
			pDevice->abyEEPROM[EEP_OFS_ZONETYPE] = 0x02;
			pDevice->abyEEPROM[EEP_OFS_MAXCHANNEL] = 0x0D;

			pr_debug("Init Zone Type :Europe\n");
		} else {
			if (zonetype != pDevice->abyEEPROM[EEP_OFS_ZONETYPE])
				pr_debug("zonetype in file[%02x] mismatch with in EEPROM[%02x]\n",
					 zonetype,
					 pDevice->abyEEPROM[EEP_OFS_ZONETYPE]);
			else
				pr_debug("Read Zonetype file success,use default zonetype setting[%02x]\n",
					 zonetype);
		}
	} else {
		pr_debug("Read Zonetype file fail,use default zonetype setting[%02x]\n",
			 SROMbyReadEmbedded(pDevice->PortOffset, EEP_OFS_ZONETYPE));
	}

	/* Get RFType */
	pDevice->byRFType = SROMbyReadEmbedded(pDevice->PortOffset, EEP_OFS_RFTYPE);

	/* force change RevID for VT3253 emu */
	if ((pDevice->byRFType & RF_EMU) != 0)
			pDevice->byRevId = 0x80;

	pDevice->byRFType &= RF_MASK;
	pr_debug("pDevice->byRFType = %x\n", pDevice->byRFType);

	if (!pDevice->bZoneRegExist)
		pDevice->byZoneType = pDevice->abyEEPROM[EEP_OFS_ZONETYPE];

	pr_debug("pDevice->byZoneType = %x\n", pDevice->byZoneType);

	/* Init RF module */
	RFbInit(pDevice);

	/* Get Desire Power Value */
	pDevice->byCurPwr = 0xFF;
	pDevice->byCCKPwr = SROMbyReadEmbedded(pDevice->PortOffset, EEP_OFS_PWR_CCK);
	pDevice->byOFDMPwrG = SROMbyReadEmbedded(pDevice->PortOffset, EEP_OFS_PWR_OFDMG);

	/* Load power Table */
	for (ii = 0; ii < CB_MAX_CHANNEL_24G; ii++) {
		pDevice->abyCCKPwrTbl[ii + 1] =
			SROMbyReadEmbedded(pDevice->PortOffset,
					   (unsigned char)(ii + EEP_OFS_CCK_PWR_TBL));
		if (pDevice->abyCCKPwrTbl[ii + 1] == 0)
			pDevice->abyCCKPwrTbl[ii+1] = pDevice->byCCKPwr;

		pDevice->abyOFDMPwrTbl[ii + 1] =
			SROMbyReadEmbedded(pDevice->PortOffset,
					   (unsigned char)(ii + EEP_OFS_OFDM_PWR_TBL));
		if (pDevice->abyOFDMPwrTbl[ii + 1] == 0)
			pDevice->abyOFDMPwrTbl[ii + 1] = pDevice->byOFDMPwrG;

		pDevice->abyCCKDefaultPwr[ii + 1] = byCCKPwrdBm;
		pDevice->abyOFDMDefaultPwr[ii + 1] = byOFDMPwrdBm;
	}

	/* recover 12,13 ,14channel for EUROPE by 11 channel */
	if (((pDevice->abyEEPROM[EEP_OFS_ZONETYPE] == ZoneType_Japan) ||
	     (pDevice->abyEEPROM[EEP_OFS_ZONETYPE] == ZoneType_Europe)) &&
	    (pDevice->byOriginalZonetype == ZoneType_USA)) {
		for (ii = 11; ii < 14; ii++) {
			pDevice->abyCCKPwrTbl[ii] = pDevice->abyCCKPwrTbl[10];
			pDevice->abyOFDMPwrTbl[ii] = pDevice->abyOFDMPwrTbl[10];

		}
	}

	/* Load OFDM A Power Table */
	for (ii = 0; ii < CB_MAX_CHANNEL_5G; ii++) {
		pDevice->abyOFDMPwrTbl[ii + CB_MAX_CHANNEL_24G + 1] =
			SROMbyReadEmbedded(pDevice->PortOffset,
					   (unsigned char)(ii + EEP_OFS_OFDMA_PWR_TBL));

		pDevice->abyOFDMDefaultPwr[ii + CB_MAX_CHANNEL_24G + 1] =
			SROMbyReadEmbedded(pDevice->PortOffset,
					   (unsigned char)(ii + EEP_OFS_OFDMA_PWR_dBm));
	}

	init_channel_table((void *)pDevice);

	if (pDevice->byLocalID > REV_ID_VT3253_B1) {
		MACvSelectPage1(pDevice->PortOffset);

		VNSvOutPortB(pDevice->PortOffset + MAC_REG_MSRCTL + 1,
			     (MSRCTL1_TXPWR | MSRCTL1_CSAPAREN));

		MACvSelectPage0(pDevice->PortOffset);
	}

	/* use relative tx timeout and 802.11i D4 */
	MACvWordRegBitsOn(pDevice->PortOffset,
			  MAC_REG_CFG, (CFG_TKIPOPT | CFG_NOTXTIMEOUT));

	/* set performance parameter by registry */
	MACvSetShortRetryLimit(pDevice->PortOffset, pDevice->byShortRetryLimit);
	MACvSetLongRetryLimit(pDevice->PortOffset, pDevice->byLongRetryLimit);

	/* reset TSF counter */
	VNSvOutPortB(pDevice->PortOffset + MAC_REG_TFTCTL, TFTCTL_TSFCNTRST);
	/* enable TSF counter */
	VNSvOutPortB(pDevice->PortOffset + MAC_REG_TFTCTL, TFTCTL_TSFCNTREN);

	/* initialize BBP registers */
	BBbVT3253Init(pDevice);

	if (pDevice->bUpdateBBVGA) {
		pDevice->byBBVGACurrent = pDevice->abyBBVGA[0];
		pDevice->byBBVGANew = pDevice->byBBVGACurrent;
		BBvSetVGAGainOffset(pDevice, pDevice->abyBBVGA[0]);
	}

	BBvSetRxAntennaMode(pDevice->PortOffset, pDevice->byRxAntennaMode);
	BBvSetTxAntennaMode(pDevice->PortOffset, pDevice->byTxAntennaMode);

	pDevice->byCurrentCh = 0;

	/* Set BB and packet type at the same time. */
	/* Set Short Slot Time, xIFS, and RSPINF. */
	if (pDevice->uConnectionRate == RATE_AUTO)
		pDevice->wCurrentRate = RATE_54M;
	else
		pDevice->wCurrentRate = (unsigned short)pDevice->uConnectionRate;

	/* default G Mode */
	VNTWIFIbConfigPhyMode(pDevice->pMgmt, PHY_TYPE_11G);
	VNTWIFIbConfigPhyMode(pDevice->pMgmt, PHY_TYPE_AUTO);

	pDevice->bRadioOff = false;

	pDevice->byRadioCtl = SROMbyReadEmbedded(pDevice->PortOffset,
						 EEP_OFS_RADIOCTL);
	pDevice->bHWRadioOff = false;

	if (pDevice->byRadioCtl & EEP_RADIOCTL_ENABLE) {
		/* Get GPIO */
		MACvGPIOIn(pDevice->PortOffset, &pDevice->byGPIO);

		if (((pDevice->byGPIO & GPIO0_DATA) &&
		     !(pDevice->byRadioCtl & EEP_RADIOCTL_INV)) ||
		     (!(pDevice->byGPIO & GPIO0_DATA) &&
		     (pDevice->byRadioCtl & EEP_RADIOCTL_INV)))
			pDevice->bHWRadioOff = true;
	}

	if (pDevice->bHWRadioOff || pDevice->bRadioControlOff)
		CARDbRadioPowerOff(pDevice);

	pMgmt->eScanType = WMAC_SCAN_PASSIVE;

	/* get Permanent network address */
	SROMvReadEtherAddress(pDevice->PortOffset, pDevice->abyCurrentNetAddr);
	pr_debug("Network address = %pM\n", pDevice->abyCurrentNetAddr);

	/* reset Tx pointer */
	CARDvSafeResetRx(pDevice);
	/* reset Rx pointer */
	CARDvSafeResetTx(pDevice);

	if (pDevice->byLocalID <= REV_ID_VT3253_A1)
		MACvRegBitsOn(pDevice->PortOffset, MAC_REG_RCR, RCR_WPAERR);

	pDevice->eEncryptionStatus = Ndis802_11EncryptionDisabled;

	/* Turn On Rx DMA */
	MACvReceive0(pDevice->PortOffset);
	MACvReceive1(pDevice->PortOffset);

	/* start the adapter */
	MACvStart(pDevice->PortOffset);

	netif_stop_queue(pDevice->dev);
}

static void device_init_diversity_timer(struct vnt_private *pDevice)
{
	init_timer(&pDevice->TimerSQ3Tmax1);
	pDevice->TimerSQ3Tmax1.data = (unsigned long) pDevice;
	pDevice->TimerSQ3Tmax1.function = (TimerFunction)TimerSQ3CallBack;
	pDevice->TimerSQ3Tmax1.expires = RUN_AT(HZ);

	init_timer(&pDevice->TimerSQ3Tmax2);
	pDevice->TimerSQ3Tmax2.data = (unsigned long) pDevice;
	pDevice->TimerSQ3Tmax2.function = (TimerFunction)TimerSQ3CallBack;
	pDevice->TimerSQ3Tmax2.expires = RUN_AT(HZ);

	init_timer(&pDevice->TimerSQ3Tmax3);
	pDevice->TimerSQ3Tmax3.data = (unsigned long) pDevice;
	pDevice->TimerSQ3Tmax3.function = (TimerFunction)TimerState1CallBack;
	pDevice->TimerSQ3Tmax3.expires = RUN_AT(HZ);
}

static bool device_release_WPADEV(struct vnt_private *pDevice)
{
	viawget_wpa_header *wpahdr;
	int ii = 0;

	//send device close to wpa_supplicnat layer
	if (pDevice->bWPADEVUp) {
		wpahdr = (viawget_wpa_header *)pDevice->skb->data;
		wpahdr->type = VIAWGET_DEVICECLOSE_MSG;
		wpahdr->resp_ie_len = 0;
		wpahdr->req_ie_len = 0;
		skb_put(pDevice->skb, sizeof(viawget_wpa_header));
		pDevice->skb->dev = pDevice->wpadev;
		skb_reset_mac_header(pDevice->skb);
		pDevice->skb->pkt_type = PACKET_HOST;
		pDevice->skb->protocol = htons(ETH_P_802_2);
		memset(pDevice->skb->cb, 0, sizeof(pDevice->skb->cb));
		netif_rx(pDevice->skb);
		pDevice->skb = dev_alloc_skb((int)pDevice->rx_buf_sz);

		while (pDevice->bWPADEVUp) {
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ / 20);          //wait 50ms
			ii++;
			if (ii > 20)
				break;
		}
	}
	return true;
}

static const struct net_device_ops device_netdev_ops = {
	.ndo_open               = device_open,
	.ndo_stop               = device_close,
	.ndo_do_ioctl           = device_ioctl,
	.ndo_start_xmit         = device_xmit,
	.ndo_set_rx_mode	= device_set_multi,
};

static int
vt6655_probe(struct pci_dev *pcid, const struct pci_device_id *ent)
{
	static bool bFirst = true;
	struct net_device *dev = NULL;
	PCHIP_INFO  pChip_info = (PCHIP_INFO)ent->driver_data;
	struct vnt_private *pDevice;
	int         rc;

	dev = alloc_etherdev(sizeof(*pDevice));

	pDevice = netdev_priv(dev);

	if (dev == NULL) {
		pr_err(DEVICE_NAME ": allocate net device failed\n");
		return -ENOMEM;
	}

	// Chain it all together
	SET_NETDEV_DEV(dev, &pcid->dev);

	if (bFirst) {
		pr_notice("%s Ver. %s\n", DEVICE_FULL_DRV_NAM, DEVICE_VERSION);
		pr_notice("Copyright (c) 2003 VIA Networking Technologies, Inc.\n");
		bFirst = false;
	}

	vt6655_init_info(pcid, &pDevice, pChip_info);
	pDevice->dev = dev;

	if (pci_enable_device(pcid)) {
		device_free_info(pDevice);
		return -ENODEV;
	}
	dev->irq = pcid->irq;

#ifdef	DEBUG
	pr_debug("Before get pci_info memaddr is %x\n", pDevice->memaddr);
#endif
	if (!device_get_pci_info(pDevice, pcid)) {
		pr_err(DEVICE_NAME ": Failed to find PCI device.\n");
		device_free_info(pDevice);
		return -ENODEV;
	}

#if 1

#ifdef	DEBUG

	pr_debug("after get pci_info memaddr is %x, io addr is %x,io_size is %d\n", pDevice->memaddr, pDevice->ioaddr, pDevice->io_size);
	{
		int i;
		u32 bar, len;
		u32 address[] = {
			PCI_BASE_ADDRESS_0,
			PCI_BASE_ADDRESS_1,
			PCI_BASE_ADDRESS_2,
			PCI_BASE_ADDRESS_3,
			PCI_BASE_ADDRESS_4,
			PCI_BASE_ADDRESS_5,
			0};
		for (i = 0; address[i]; i++) {
			pci_read_config_dword(pcid, address[i], &bar);
			pr_debug("bar %d is %x\n", i, bar);
			if (!bar) {
				pr_debug("bar %d not implemented\n", i);
				continue;
			}
			if (bar & PCI_BASE_ADDRESS_SPACE_IO) {
				/* This is IO */

				len = bar & (PCI_BASE_ADDRESS_IO_MASK & 0xFFFF);
				len = len & ~(len - 1);

				pr_debug("IO space:  len in IO %x, BAR %d\n", len, i);
			} else {
				len = bar & 0xFFFFFFF0;
				len = ~len + 1;

				pr_debug("len in MEM %x, BAR %d\n", len, i);
			}
		}
	}
#endif

#endif

	pDevice->PortOffset = ioremap(pDevice->memaddr & PCI_BASE_ADDRESS_MEM_MASK, pDevice->io_size);

	if (pDevice->PortOffset == NULL) {
		pr_err(DEVICE_NAME ": Failed to IO remapping ..\n");
		device_free_info(pDevice);
		return -ENODEV;
	}

	rc = pci_request_regions(pcid, DEVICE_NAME);
	if (rc) {
		pr_err(DEVICE_NAME ": Failed to find PCI device\n");
		device_free_info(pDevice);
		return -ENODEV;
	}

	dev->base_addr = pDevice->ioaddr;
	// do reset
	if (!MACbSoftwareReset(pDevice->PortOffset)) {
		pr_err(DEVICE_NAME ": Failed to access MAC hardware..\n");
		device_free_info(pDevice);
		return -ENODEV;
	}
	// initial to reload eeprom
	MACvInitialize(pDevice->PortOffset);
	MACvReadEtherAddress(pDevice->PortOffset, dev->dev_addr);

	device_get_options(pDevice);
	device_set_options(pDevice);
	//Mask out the options cannot be set to the chip
	pDevice->sOpts.flags &= pChip_info->flags;

	//Enable the chip specified capabilities
	pDevice->flags = pDevice->sOpts.flags | (pChip_info->flags & 0xFF000000UL);
	pDevice->tx_80211 = device_dma0_tx_80211;
	pDevice->sMgmtObj.pAdapter = (void *)pDevice;
	pDevice->pMgmt = &(pDevice->sMgmtObj);

	dev->irq                = pcid->irq;
	dev->netdev_ops         = &device_netdev_ops;

	dev->wireless_handlers = (struct iw_handler_def *)&iwctl_handler_def;

	rc = register_netdev(dev);
	if (rc) {
		pr_err(DEVICE_NAME " Failed to register netdev\n");
		device_free_info(pDevice);
		return -ENODEV;
	}
	device_print_info(pDevice);
	pci_set_drvdata(pcid, pDevice);
	return 0;
}

static void device_print_info(struct vnt_private *pDevice)
{
	struct net_device *dev = pDevice->dev;

	pr_info("%s: %s\n", dev->name, get_chip_name(pDevice->chip_id));
	pr_info("%s: MAC=%pM IO=0x%lx Mem=0x%lx IRQ=%d\n",
		dev->name, dev->dev_addr, (unsigned long)pDevice->ioaddr,
		(unsigned long)pDevice->PortOffset, pDevice->dev->irq);
}

static void vt6655_init_info(struct pci_dev *pcid,
			     struct vnt_private **ppDevice,
			     PCHIP_INFO pChip_info)
{
	memset(*ppDevice, 0, sizeof(**ppDevice));

	(*ppDevice)->pcid = pcid;
	(*ppDevice)->chip_id = pChip_info->chip_id;
	(*ppDevice)->io_size = pChip_info->io_size;
	(*ppDevice)->nTxQueues = pChip_info->nTxQueue;
	(*ppDevice)->multicast_limit = 32;

	spin_lock_init(&((*ppDevice)->lock));
}

static bool device_get_pci_info(struct vnt_private *pDevice,
				struct pci_dev *pcid)
{
	u16 pci_cmd;
	u8  b;
	unsigned int cis_addr;

	pci_read_config_byte(pcid, PCI_REVISION_ID, &pDevice->byRevId);
	pci_read_config_word(pcid, PCI_SUBSYSTEM_ID, &pDevice->SubSystemID);
	pci_read_config_word(pcid, PCI_SUBSYSTEM_VENDOR_ID, &pDevice->SubVendorID);
	pci_read_config_word(pcid, PCI_COMMAND, (u16 *)&(pci_cmd));

	pci_set_master(pcid);

	pDevice->memaddr = pci_resource_start(pcid, 0);
	pDevice->ioaddr = pci_resource_start(pcid, 1);

	cis_addr = pci_resource_start(pcid, 2);

	pDevice->pcid = pcid;

	pci_read_config_byte(pcid, PCI_COMMAND, &b);
	pci_write_config_byte(pcid, PCI_COMMAND, (b|PCI_COMMAND_MASTER));

	return true;
}

static void device_free_info(struct vnt_private *pDevice)
{
	struct net_device *dev = pDevice->dev;

	ASSERT(pDevice);
//2008-0714-01<Add>by chester
	device_release_WPADEV(pDevice);

//2008-07-21-01<Add>by MikeLiu
//unregister wpadev
	if (wpa_set_wpadev(pDevice, 0) != 0)
		pr_err("unregister wpadev fail?\n");

#ifdef HOSTAP
	if (dev)
		vt6655_hostap_set_hostapd(pDevice, 0, 0);
#endif
	if (dev)
		unregister_netdev(dev);

	if (pDevice->PortOffset)
		iounmap(pDevice->PortOffset);

	if (pDevice->pcid)
		pci_release_regions(pDevice->pcid);
	if (dev)
		free_netdev(dev);
}

static bool device_init_rings(struct vnt_private *pDevice)
{
	void *vir_pool;

	/*allocate all RD/TD rings a single pool*/
	vir_pool = pci_zalloc_consistent(pDevice->pcid,
					 pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc) +
					 pDevice->sOpts.nRxDescs1 * sizeof(SRxDesc) +
					 pDevice->sOpts.nTxDescs[0] * sizeof(STxDesc) +
					 pDevice->sOpts.nTxDescs[1] * sizeof(STxDesc),
					 &pDevice->pool_dma);
	if (vir_pool == NULL) {
		dev_err(&pDevice->pcid->dev, "allocate desc dma memory failed\n");
		return false;
	}

	pDevice->aRD0Ring = vir_pool;
	pDevice->aRD1Ring = vir_pool +
		pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc);

	pDevice->rd0_pool_dma = pDevice->pool_dma;
	pDevice->rd1_pool_dma = pDevice->rd0_pool_dma +
		pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc);

	pDevice->tx0_bufs = pci_zalloc_consistent(pDevice->pcid,
						  pDevice->sOpts.nTxDescs[0] * PKT_BUF_SZ +
						  pDevice->sOpts.nTxDescs[1] * PKT_BUF_SZ +
						  CB_BEACON_BUF_SIZE +
						  CB_MAX_BUF_SIZE,
						  &pDevice->tx_bufs_dma0);
	if (pDevice->tx0_bufs == NULL) {
		dev_err(&pDevice->pcid->dev, "allocate buf dma memory failed\n");

		pci_free_consistent(pDevice->pcid,
				    pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc) +
				    pDevice->sOpts.nRxDescs1 * sizeof(SRxDesc) +
				    pDevice->sOpts.nTxDescs[0] * sizeof(STxDesc) +
				    pDevice->sOpts.nTxDescs[1] * sizeof(STxDesc),
				    vir_pool, pDevice->pool_dma
			);
		return false;
	}

	pDevice->td0_pool_dma = pDevice->rd1_pool_dma +
		pDevice->sOpts.nRxDescs1 * sizeof(SRxDesc);

	pDevice->td1_pool_dma = pDevice->td0_pool_dma +
		pDevice->sOpts.nTxDescs[0] * sizeof(STxDesc);

	// vir_pool: pvoid type
	pDevice->apTD0Rings = vir_pool
		+ pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc)
		+ pDevice->sOpts.nRxDescs1 * sizeof(SRxDesc);

	pDevice->apTD1Rings = vir_pool
		+ pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc)
		+ pDevice->sOpts.nRxDescs1 * sizeof(SRxDesc)
		+ pDevice->sOpts.nTxDescs[0] * sizeof(STxDesc);

	pDevice->tx1_bufs = pDevice->tx0_bufs +
		pDevice->sOpts.nTxDescs[0] * PKT_BUF_SZ;

	pDevice->tx_beacon_bufs = pDevice->tx1_bufs +
		pDevice->sOpts.nTxDescs[1] * PKT_BUF_SZ;

	pDevice->pbyTmpBuff = pDevice->tx_beacon_bufs +
		CB_BEACON_BUF_SIZE;

	pDevice->tx_bufs_dma1 = pDevice->tx_bufs_dma0 +
		pDevice->sOpts.nTxDescs[0] * PKT_BUF_SZ;

	pDevice->tx_beacon_dma = pDevice->tx_bufs_dma1 +
		pDevice->sOpts.nTxDescs[1] * PKT_BUF_SZ;

	return true;
}

static void device_free_rings(struct vnt_private *pDevice)
{
	pci_free_consistent(pDevice->pcid,
			    pDevice->sOpts.nRxDescs0 * sizeof(SRxDesc) +
			    pDevice->sOpts.nRxDescs1 * sizeof(SRxDesc) +
			    pDevice->sOpts.nTxDescs[0] * sizeof(STxDesc) +
			    pDevice->sOpts.nTxDescs[1] * sizeof(STxDesc)
			    ,
			    pDevice->aRD0Ring, pDevice->pool_dma
		);

	if (pDevice->tx0_bufs)
		pci_free_consistent(pDevice->pcid,
				    pDevice->sOpts.nTxDescs[0] * PKT_BUF_SZ +
				    pDevice->sOpts.nTxDescs[1] * PKT_BUF_SZ +
				    CB_BEACON_BUF_SIZE +
				    CB_MAX_BUF_SIZE,
				    pDevice->tx0_bufs, pDevice->tx_bufs_dma0
			);
}

static void device_init_rd0_ring(struct vnt_private *pDevice)
{
	int i;
	dma_addr_t      curr = pDevice->rd0_pool_dma;
	PSRxDesc        pDesc;

	/* Init the RD0 ring entries */
	for (i = 0; i < pDevice->sOpts.nRxDescs0; i ++, curr += sizeof(SRxDesc)) {
		pDesc = &(pDevice->aRD0Ring[i]);
		pDesc->pRDInfo = alloc_rd_info();
		ASSERT(pDesc->pRDInfo);
		if (!device_alloc_rx_buf(pDevice, pDesc))
			dev_err(&pDevice->pcid->dev, "can not alloc rx bufs\n");

		pDesc->next = &(pDevice->aRD0Ring[(i+1) % pDevice->sOpts.nRxDescs0]);
		pDesc->pRDInfo->curr_desc = cpu_to_le32(curr);
		pDesc->next_desc = cpu_to_le32(curr + sizeof(SRxDesc));
	}

	if (i > 0)
		pDevice->aRD0Ring[i-1].next_desc = cpu_to_le32(pDevice->rd0_pool_dma);
	pDevice->pCurrRD[0] = &(pDevice->aRD0Ring[0]);
}

static void device_init_rd1_ring(struct vnt_private *pDevice)
{
	int i;
	dma_addr_t      curr = pDevice->rd1_pool_dma;
	PSRxDesc        pDesc;

	/* Init the RD1 ring entries */
	for (i = 0; i < pDevice->sOpts.nRxDescs1; i ++, curr += sizeof(SRxDesc)) {
		pDesc = &(pDevice->aRD1Ring[i]);
		pDesc->pRDInfo = alloc_rd_info();
		ASSERT(pDesc->pRDInfo);
		if (!device_alloc_rx_buf(pDevice, pDesc))
			dev_err(&pDevice->pcid->dev, "can not alloc rx bufs\n");

		pDesc->next = &(pDevice->aRD1Ring[(i+1) % pDevice->sOpts.nRxDescs1]);
		pDesc->pRDInfo->curr_desc = cpu_to_le32(curr);
		pDesc->next_desc = cpu_to_le32(curr + sizeof(SRxDesc));
	}

	if (i > 0)
		pDevice->aRD1Ring[i-1].next_desc = cpu_to_le32(pDevice->rd1_pool_dma);
	pDevice->pCurrRD[1] = &(pDevice->aRD1Ring[0]);
}

static void device_init_defrag_cb(struct vnt_private *pDevice)
{
	int i;
	PSDeFragControlBlock pDeF;

	/* Init the fragment ctl entries */
	for (i = 0; i < CB_MAX_RX_FRAG; i++) {
		pDeF = &(pDevice->sRxDFCB[i]);
		if (!device_alloc_frag_buf(pDevice, pDeF))
			dev_err(&pDevice->pcid->dev, "can not alloc frag bufs\n");
	}
	pDevice->cbDFCB = CB_MAX_RX_FRAG;
	pDevice->cbFreeDFCB = pDevice->cbDFCB;
}

static void device_free_rd0_ring(struct vnt_private *pDevice)
{
	int i;

	for (i = 0; i < pDevice->sOpts.nRxDescs0; i++) {
		PSRxDesc        pDesc = &(pDevice->aRD0Ring[i]);
		PDEVICE_RD_INFO  pRDInfo = pDesc->pRDInfo;

		pci_unmap_single(pDevice->pcid, pRDInfo->skb_dma,
				 pDevice->rx_buf_sz, PCI_DMA_FROMDEVICE);

		dev_kfree_skb(pRDInfo->skb);

		kfree((void *)pDesc->pRDInfo);
	}
}

static void device_free_rd1_ring(struct vnt_private *pDevice)
{
	int i;

	for (i = 0; i < pDevice->sOpts.nRxDescs1; i++) {
		PSRxDesc        pDesc = &(pDevice->aRD1Ring[i]);
		PDEVICE_RD_INFO  pRDInfo = pDesc->pRDInfo;

		pci_unmap_single(pDevice->pcid, pRDInfo->skb_dma,
				 pDevice->rx_buf_sz, PCI_DMA_FROMDEVICE);

		dev_kfree_skb(pRDInfo->skb);

		kfree((void *)pDesc->pRDInfo);
	}
}

static void device_free_frag_buf(struct vnt_private *pDevice)
{
	PSDeFragControlBlock pDeF;
	int i;

	for (i = 0; i < CB_MAX_RX_FRAG; i++) {
		pDeF = &(pDevice->sRxDFCB[i]);

		if (pDeF->skb)
			dev_kfree_skb(pDeF->skb);

	}
}

static void device_init_td0_ring(struct vnt_private *pDevice)
{
	int i;
	dma_addr_t  curr;
	PSTxDesc        pDesc;

	curr = pDevice->td0_pool_dma;
	for (i = 0; i < pDevice->sOpts.nTxDescs[0]; i++, curr += sizeof(STxDesc)) {
		pDesc = &(pDevice->apTD0Rings[i]);
		pDesc->pTDInfo = alloc_td_info();
		ASSERT(pDesc->pTDInfo);
		if (pDevice->flags & DEVICE_FLAGS_TX_ALIGN) {
			pDesc->pTDInfo->buf = pDevice->tx0_bufs + (i)*PKT_BUF_SZ;
			pDesc->pTDInfo->buf_dma = pDevice->tx_bufs_dma0 + (i)*PKT_BUF_SZ;
		}
		pDesc->next = &(pDevice->apTD0Rings[(i+1) % pDevice->sOpts.nTxDescs[0]]);
		pDesc->pTDInfo->curr_desc = cpu_to_le32(curr);
		pDesc->next_desc = cpu_to_le32(curr+sizeof(STxDesc));
	}

	if (i > 0)
		pDevice->apTD0Rings[i-1].next_desc = cpu_to_le32(pDevice->td0_pool_dma);
	pDevice->apTailTD[0] = pDevice->apCurrTD[0] = &(pDevice->apTD0Rings[0]);
}

static void device_init_td1_ring(struct vnt_private *pDevice)
{
	int i;
	dma_addr_t  curr;
	PSTxDesc    pDesc;

	/* Init the TD ring entries */
	curr = pDevice->td1_pool_dma;
	for (i = 0; i < pDevice->sOpts.nTxDescs[1]; i++, curr += sizeof(STxDesc)) {
		pDesc = &(pDevice->apTD1Rings[i]);
		pDesc->pTDInfo = alloc_td_info();
		ASSERT(pDesc->pTDInfo);
		if (pDevice->flags & DEVICE_FLAGS_TX_ALIGN) {
			pDesc->pTDInfo->buf = pDevice->tx1_bufs + (i) * PKT_BUF_SZ;
			pDesc->pTDInfo->buf_dma = pDevice->tx_bufs_dma1 + (i) * PKT_BUF_SZ;
		}
		pDesc->next = &(pDevice->apTD1Rings[(i + 1) % pDevice->sOpts.nTxDescs[1]]);
		pDesc->pTDInfo->curr_desc = cpu_to_le32(curr);
		pDesc->next_desc = cpu_to_le32(curr+sizeof(STxDesc));
	}

	if (i > 0)
		pDevice->apTD1Rings[i-1].next_desc = cpu_to_le32(pDevice->td1_pool_dma);
	pDevice->apTailTD[1] = pDevice->apCurrTD[1] = &(pDevice->apTD1Rings[0]);
}

static void device_free_td0_ring(struct vnt_private *pDevice)
{
	int i;

	for (i = 0; i < pDevice->sOpts.nTxDescs[0]; i++) {
		PSTxDesc        pDesc = &(pDevice->apTD0Rings[i]);
		PDEVICE_TD_INFO  pTDInfo = pDesc->pTDInfo;

		if (pTDInfo->skb_dma && (pTDInfo->skb_dma != pTDInfo->buf_dma))
			pci_unmap_single(pDevice->pcid, pTDInfo->skb_dma,
					 pTDInfo->skb->len, PCI_DMA_TODEVICE);

		if (pTDInfo->skb)
			dev_kfree_skb(pTDInfo->skb);

		kfree((void *)pDesc->pTDInfo);
	}
}

static void device_free_td1_ring(struct vnt_private *pDevice)
{
	int i;

	for (i = 0; i < pDevice->sOpts.nTxDescs[1]; i++) {
		PSTxDesc        pDesc = &(pDevice->apTD1Rings[i]);
		PDEVICE_TD_INFO  pTDInfo = pDesc->pTDInfo;

		if (pTDInfo->skb_dma && (pTDInfo->skb_dma != pTDInfo->buf_dma))
			pci_unmap_single(pDevice->pcid, pTDInfo->skb_dma,
					 pTDInfo->skb->len, PCI_DMA_TODEVICE);

		if (pTDInfo->skb)
			dev_kfree_skb(pTDInfo->skb);

		kfree((void *)pDesc->pTDInfo);
	}
}

/*-----------------------------------------------------------------*/

static int device_rx_srv(struct vnt_private *pDevice, unsigned int uIdx)
{
	PSRxDesc    pRD;
	int works = 0;

	for (pRD = pDevice->pCurrRD[uIdx];
	     pRD->m_rd0RD0.f1Owner == OWNED_BY_HOST;
	     pRD = pRD->next) {
		if (works++ > 15)
			break;

		if (!pRD->pRDInfo->skb)
			break;

		if (device_receive_frame(pDevice, pRD)) {
			if (!device_alloc_rx_buf(pDevice, pRD)) {
				dev_err(&pDevice->pcid->dev,
					"can not allocate rx buf\n");
				break;
			}
		}
		pRD->m_rd0RD0.f1Owner = OWNED_BY_NIC;
		pDevice->dev->last_rx = jiffies;
	}

	pDevice->pCurrRD[uIdx] = pRD;

	return works;
}

static bool device_alloc_rx_buf(struct vnt_private *pDevice, PSRxDesc pRD)
{
	PDEVICE_RD_INFO pRDInfo = pRD->pRDInfo;

	pRDInfo->skb = dev_alloc_skb((int)pDevice->rx_buf_sz);
	if (pRDInfo->skb == NULL)
		return false;
	ASSERT(pRDInfo->skb);
	pRDInfo->skb->dev = pDevice->dev;
	pRDInfo->skb_dma = pci_map_single(pDevice->pcid, skb_tail_pointer(pRDInfo->skb),
					  pDevice->rx_buf_sz, PCI_DMA_FROMDEVICE);
	*((unsigned int *)&(pRD->m_rd0RD0)) = 0; /* FIX cast */

	pRD->m_rd0RD0.wResCount = cpu_to_le16(pDevice->rx_buf_sz);
	pRD->m_rd0RD0.f1Owner = OWNED_BY_NIC;
	pRD->m_rd1RD1.wReqCount = cpu_to_le16(pDevice->rx_buf_sz);
	pRD->buff_addr = cpu_to_le32(pRDInfo->skb_dma);

	return true;
}

bool device_alloc_frag_buf(struct vnt_private *pDevice,
			   PSDeFragControlBlock pDeF)
{
	pDeF->skb = dev_alloc_skb((int)pDevice->rx_buf_sz);
	if (pDeF->skb == NULL)
		return false;
	ASSERT(pDeF->skb);
	pDeF->skb->dev = pDevice->dev;

	return true;
}

static int device_tx_srv(struct vnt_private *pDevice, unsigned int uIdx)
{
	PSTxDesc                 pTD;
	bool bFull = false;
	int                      works = 0;
	unsigned char byTsr0;
	unsigned char byTsr1;
	unsigned int	uFrameSize, uFIFOHeaderSize;
	PSTxBufHead              pTxBufHead;
	struct net_device_stats *pStats = &pDevice->dev->stats;
	struct sk_buff *skb;
	unsigned int	uNodeIndex;
	PSMgmtObject             pMgmt = pDevice->pMgmt;

	for (pTD = pDevice->apTailTD[uIdx]; pDevice->iTDUsed[uIdx] > 0; pTD = pTD->next) {
		if (pTD->m_td0TD0.f1Owner == OWNED_BY_NIC)
			break;
		if (works++ > 15)
			break;

		byTsr0 = pTD->m_td0TD0.byTSR0;
		byTsr1 = pTD->m_td0TD0.byTSR1;

		//Only the status of first TD in the chain is correct
		if (pTD->m_td1TD1.byTCR & TCR_STP) {
			if ((pTD->pTDInfo->byFlags & TD_FLAGS_NETIF_SKB) != 0) {
				uFIFOHeaderSize = pTD->pTDInfo->dwHeaderLength;
				uFrameSize = pTD->pTDInfo->dwReqCount - uFIFOHeaderSize;
				pTxBufHead = (PSTxBufHead) (pTD->pTDInfo->buf);
				// Update the statistics based on the Transmit status
				// now, we DONT check TSR0_CDH

				STAvUpdateTDStatCounter(&pDevice->scStatistic,
							byTsr0, byTsr1,
							(unsigned char *)(pTD->pTDInfo->buf + uFIFOHeaderSize),
							uFrameSize, uIdx);

				BSSvUpdateNodeTxCounter(pDevice,
							byTsr0, byTsr1,
							(unsigned char *)(pTD->pTDInfo->buf),
							uFIFOHeaderSize
					);

				if (!(byTsr1 & TSR1_TERR)) {
					if (byTsr0 != 0) {
						pr_debug(" Tx[%d] OK but has error. tsr1[%02X] tsr0[%02X]\n",
							 (int)uIdx, byTsr1,
							 byTsr0);
					}
					if ((pTxBufHead->wFragCtl & FRAGCTL_ENDFRAG) != FRAGCTL_NONFRAG)
						pDevice->s802_11Counter.TransmittedFragmentCount++;

					pStats->tx_packets++;
					pStats->tx_bytes += pTD->pTDInfo->skb->len;
				} else {
					pr_debug(" Tx[%d] dropped & tsr1[%02X] tsr0[%02X]\n",
						 (int)uIdx, byTsr1, byTsr0);
					pStats->tx_errors++;
					pStats->tx_dropped++;
				}
			}

			if ((pTD->pTDInfo->byFlags & TD_FLAGS_PRIV_SKB) != 0) {
				if (pDevice->bEnableHostapd) {
					pr_debug("tx call back netif..\n");
					skb = pTD->pTDInfo->skb;
					skb->dev = pDevice->apdev;
					skb_reset_mac_header(skb);
					skb->pkt_type = PACKET_OTHERHOST;
					memset(skb->cb, 0, sizeof(skb->cb));
					netif_rx(skb);
				}
			}

			if (byTsr1 & TSR1_TERR) {
				if ((pTD->pTDInfo->byFlags & TD_FLAGS_PRIV_SKB) != 0) {
					pr_debug(" Tx[%d] fail has error. tsr1[%02X] tsr0[%02X]\n",
						 (int)uIdx, byTsr1, byTsr0);
				}


				if ((pMgmt->eCurrMode == WMAC_MODE_ESS_AP) &&
				    (pTD->pTDInfo->byFlags & TD_FLAGS_NETIF_SKB)) {
					unsigned short wAID;
					unsigned char byMask[8] = {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80};

					skb = pTD->pTDInfo->skb;
					if (BSSDBbIsSTAInNodeDB(pMgmt, (unsigned char *)(skb->data), &uNodeIndex)) {
						if (pMgmt->sNodeDBTable[uNodeIndex].bPSEnable) {
							skb_queue_tail(&pMgmt->sNodeDBTable[uNodeIndex].sTxPSQueue, skb);
							pMgmt->sNodeDBTable[uNodeIndex].wEnQueueCnt++;
							// set tx map
							wAID = pMgmt->sNodeDBTable[uNodeIndex].wAID;
							pMgmt->abyPSTxMap[wAID >> 3] |=  byMask[wAID & 7];
							pTD->pTDInfo->byFlags &= ~(TD_FLAGS_NETIF_SKB);
							pr_debug("tx_srv:tx fail re-queue sta index= %d, QueCnt= %d\n",
								 (int)uNodeIndex,
								 pMgmt->sNodeDBTable[uNodeIndex].wEnQueueCnt);
							pStats->tx_errors--;
							pStats->tx_dropped--;
						}
					}
				}
			}
			device_free_tx_buf(pDevice, pTD);
			pDevice->iTDUsed[uIdx]--;
		}
	}

	if (uIdx == TYPE_AC0DMA) {
		// RESERV_AC0DMA reserved for relay

		if (AVAIL_TD(pDevice, uIdx) < RESERV_AC0DMA) {
			bFull = true;
			pr_debug(" AC0DMA is Full = %d\n",
				 pDevice->iTDUsed[uIdx]);
		}
		if (netif_queue_stopped(pDevice->dev) && !bFull)
			netif_wake_queue(pDevice->dev);

	}

	pDevice->apTailTD[uIdx] = pTD;

	return works;
}

static void device_error(struct vnt_private *pDevice, unsigned short status)
{
	if (status & ISR_FETALERR) {
		dev_err(&pDevice->pcid->dev, "Hardware fatal error\n");

		netif_stop_queue(pDevice->dev);
		del_timer(&pDevice->sTimerCommand);
		del_timer(&(pDevice->pMgmt->sTimerSecondCallback));
		pDevice->bCmdRunning = false;
		MACbShutdown(pDevice->PortOffset);
		return;
	}
}

static void device_free_tx_buf(struct vnt_private *pDevice, PSTxDesc pDesc)
{
	PDEVICE_TD_INFO  pTDInfo = pDesc->pTDInfo;
	struct sk_buff *skb = pTDInfo->skb;

	// pre-allocated buf_dma can't be unmapped.
	if (pTDInfo->skb_dma && (pTDInfo->skb_dma != pTDInfo->buf_dma)) {
		pci_unmap_single(pDevice->pcid, pTDInfo->skb_dma, skb->len,
				 PCI_DMA_TODEVICE);
	}

	if ((pTDInfo->byFlags & TD_FLAGS_NETIF_SKB) != 0)
		dev_kfree_skb_irq(skb);

	pTDInfo->skb_dma = 0;
	pTDInfo->skb = NULL;
	pTDInfo->byFlags = 0;
}

static int  device_open(struct net_device *dev)
{
	struct vnt_private *pDevice = netdev_priv(dev);
	int i;
#ifdef WPA_SM_Transtatus
	extern SWPAResult wpa_Result;
#endif

	pDevice->rx_buf_sz = PKT_BUF_SZ;
	if (!device_init_rings(pDevice))
		return -ENOMEM;

//2008-5-13 <add> by chester
	i = request_irq(pDevice->pcid->irq, &device_intr, IRQF_SHARED, dev->name, dev);
	if (i)
		return i;

#ifdef WPA_SM_Transtatus
	memset(wpa_Result.ifname, 0, sizeof(wpa_Result.ifname));
	wpa_Result.proto = 0;
	wpa_Result.key_mgmt = 0;
	wpa_Result.eap_type = 0;
	wpa_Result.authenticated = false;
	pDevice->fWPA_Authened = false;
#endif
	pr_debug("call device init rd0 ring\n");
	device_init_rd0_ring(pDevice);
	device_init_rd1_ring(pDevice);
	device_init_defrag_cb(pDevice);
	device_init_td0_ring(pDevice);
	device_init_td1_ring(pDevice);

	if (pDevice->bDiversityRegCtlON)
		device_init_diversity_timer(pDevice);

	vMgrObjectInit(pDevice);
	vMgrTimerInit(pDevice);

	pr_debug("call device_init_registers\n");
	device_init_registers(pDevice);

	MACvReadEtherAddress(pDevice->PortOffset, pDevice->abyCurrentNetAddr);
	memcpy(pDevice->pMgmt->abyMACAddr, pDevice->abyCurrentNetAddr, ETH_ALEN);
	device_set_multi(pDevice->dev);

	// Init for Key Management
	KeyvInitTable(&pDevice->sKey, pDevice->PortOffset);
	add_timer(&(pDevice->pMgmt->sTimerSecondCallback));

#ifdef WPA_SUPPLICANT_DRIVER_WEXT_SUPPORT
	pDevice->bwextcount = 0;
	pDevice->bWPASuppWextEnabled = false;
#endif
	pDevice->byReAssocCount = 0;
	pDevice->bWPADEVUp = false;
	// Patch: if WEP key already set by iwconfig but device not yet open
	if (pDevice->bEncryptionEnable && pDevice->bTransmitKey) {
		KeybSetDefaultKey(&(pDevice->sKey),
				  (unsigned long)(pDevice->byKeyIndex | (1 << 31)),
				  pDevice->uKeyLength,
				  NULL,
				  pDevice->abyKey,
				  KEY_CTL_WEP,
				  pDevice->PortOffset,
				  pDevice->byLocalID
			);
		pDevice->eEncryptionStatus = Ndis802_11Encryption1Enabled;
	}

	pr_debug("call MACvIntEnable\n");
	MACvIntEnable(pDevice->PortOffset, IMR_MASK_VALUE);

	if (pDevice->pMgmt->eConfigMode == WMAC_CONFIG_AP) {
		bScheduleCommand((void *)pDevice, WLAN_CMD_RUN_AP, NULL);
	} else {
		bScheduleCommand((void *)pDevice, WLAN_CMD_BSSID_SCAN, NULL);
		bScheduleCommand((void *)pDevice, WLAN_CMD_SSID, NULL);
	}
	pDevice->flags |= DEVICE_FLAGS_OPENED;

	pr_debug("device_open success..\n");
	return 0;
}

static int  device_close(struct net_device *dev)
{
	struct vnt_private *pDevice = netdev_priv(dev);
	PSMgmtObject     pMgmt = pDevice->pMgmt;
//2007-1121-02<Add>by EinsnLiu
	if (pDevice->bLinkPass) {
		bScheduleCommand((void *)pDevice, WLAN_CMD_DISASSOCIATE, NULL);
		mdelay(30);
	}

	del_timer(&pDevice->sTimerTxData);
	del_timer(&pDevice->sTimerCommand);
	del_timer(&pMgmt->sTimerSecondCallback);
	if (pDevice->bDiversityRegCtlON) {
		del_timer(&pDevice->TimerSQ3Tmax1);
		del_timer(&pDevice->TimerSQ3Tmax2);
		del_timer(&pDevice->TimerSQ3Tmax3);
	}

	netif_stop_queue(dev);
	pDevice->bCmdRunning = false;
	MACbShutdown(pDevice->PortOffset);
	MACbSoftwareReset(pDevice->PortOffset);
	CARDbRadioPowerOff(pDevice);

	pDevice->bLinkPass = false;
	memset(pMgmt->abyCurrBSSID, 0, 6);
	pMgmt->eCurrState = WMAC_STATE_IDLE;
	device_free_td0_ring(pDevice);
	device_free_td1_ring(pDevice);
	device_free_rd0_ring(pDevice);
	device_free_rd1_ring(pDevice);
	device_free_frag_buf(pDevice);
	device_free_rings(pDevice);
	BSSvClearNodeDBTable(pDevice, 0);
	free_irq(dev->irq, dev);
	pDevice->flags &= (~DEVICE_FLAGS_OPENED);
	//2008-0714-01<Add>by chester
	device_release_WPADEV(pDevice);

	pr_debug("device_close..\n");
	return 0;
}

static int device_dma0_tx_80211(struct sk_buff *skb, struct net_device *dev)
{
	struct vnt_private *pDevice = netdev_priv(dev);
	unsigned char *pbMPDU;
	unsigned int cbMPDULen = 0;

	pr_debug("device_dma0_tx_80211\n");
	spin_lock_irq(&pDevice->lock);

	if (AVAIL_TD(pDevice, TYPE_TXDMA0) <= 0) {
		pr_debug("device_dma0_tx_80211, td0 <=0\n");
		dev_kfree_skb_irq(skb);
		spin_unlock_irq(&pDevice->lock);
		return 0;
	}

	if (pDevice->bStopTx0Pkt) {
		dev_kfree_skb_irq(skb);
		spin_unlock_irq(&pDevice->lock);
		return 0;
	}

	cbMPDULen = skb->len;
	pbMPDU = skb->data;

	vDMA0_tx_80211(pDevice, skb, pbMPDU, cbMPDULen);

	spin_unlock_irq(&pDevice->lock);

	return 0;
}

bool device_dma0_xmit(struct vnt_private *pDevice,
		      struct sk_buff *skb, unsigned int uNodeIndex)
{
	PSMgmtObject    pMgmt = pDevice->pMgmt;
	PSTxDesc        pHeadTD, pLastTD;
	unsigned int cbFrameBodySize;
	unsigned int uMACfragNum;
	unsigned char byPktType;
	bool bNeedEncryption = false;
	PSKeyItem       pTransmitKey = NULL;
	unsigned int cbHeaderSize;
	unsigned int ii;
	SKeyItem        STempKey;

	if (pDevice->bStopTx0Pkt) {
		dev_kfree_skb_irq(skb);
		return false;
	}

	if (AVAIL_TD(pDevice, TYPE_TXDMA0) <= 0) {
		dev_kfree_skb_irq(skb);
		pr_debug("device_dma0_xmit, td0 <=0\n");
		return false;
	}

	if (pMgmt->eCurrMode == WMAC_MODE_ESS_AP) {
		if (pDevice->uAssocCount == 0) {
			dev_kfree_skb_irq(skb);
			pr_debug("device_dma0_xmit, assocCount = 0\n");
			return false;
		}
	}

	pHeadTD = pDevice->apCurrTD[TYPE_TXDMA0];

	pHeadTD->m_td1TD1.byTCR = (TCR_EDP|TCR_STP);

	memcpy(pDevice->sTxEthHeader.abyDstAddr, (unsigned char *)(skb->data), ETH_HLEN);
	cbFrameBodySize = skb->len - ETH_HLEN;

	// 802.1H
	if (ntohs(pDevice->sTxEthHeader.wType) > ETH_DATA_LEN)
		cbFrameBodySize += 8;

	uMACfragNum = cbGetFragCount(pDevice, pTransmitKey, cbFrameBodySize, &pDevice->sTxEthHeader);

	if (uMACfragNum > AVAIL_TD(pDevice, TYPE_TXDMA0)) {
		dev_kfree_skb_irq(skb);
		return false;
	}
	byPktType = (unsigned char)pDevice->byPacketType;

	if (pDevice->bFixRate) {
		if (pDevice->eCurrentPHYType == PHY_TYPE_11B) {
			if (pDevice->uConnectionRate >= RATE_11M)
				pDevice->wCurrentRate = RATE_11M;
			else
				pDevice->wCurrentRate = (unsigned short)pDevice->uConnectionRate;
		} else {
			if (pDevice->uConnectionRate >= RATE_54M)
				pDevice->wCurrentRate = RATE_54M;
			else
				pDevice->wCurrentRate = (unsigned short)pDevice->uConnectionRate;
		}
	} else {
		pDevice->wCurrentRate = pDevice->pMgmt->sNodeDBTable[uNodeIndex].wTxDataRate;
	}

	//preamble type
	if (pMgmt->sNodeDBTable[uNodeIndex].bShortPreamble)
		pDevice->byPreambleType = pDevice->byShortPreamble;
	else
		pDevice->byPreambleType = PREAMBLE_LONG;

	pr_debug("dma0: pDevice->wCurrentRate = %d\n", pDevice->wCurrentRate);

	if (pDevice->wCurrentRate <= RATE_11M) {
		byPktType = PK_TYPE_11B;
	} else if (pDevice->eCurrentPHYType == PHY_TYPE_11A) {
		byPktType = PK_TYPE_11A;
	} else {
		if (pDevice->bProtectMode)
			byPktType = PK_TYPE_11GB;
		else
			byPktType = PK_TYPE_11GA;
	}

	if (pDevice->bEncryptionEnable)
		bNeedEncryption = true;

	if (pDevice->bEnableHostWEP) {
		pTransmitKey = &STempKey;
		pTransmitKey->byCipherSuite = pMgmt->sNodeDBTable[uNodeIndex].byCipherSuite;
		pTransmitKey->dwKeyIndex = pMgmt->sNodeDBTable[uNodeIndex].dwKeyIndex;
		pTransmitKey->uKeyLength = pMgmt->sNodeDBTable[uNodeIndex].uWepKeyLength;
		pTransmitKey->dwTSC47_16 = pMgmt->sNodeDBTable[uNodeIndex].dwTSC47_16;
		pTransmitKey->wTSC15_0 = pMgmt->sNodeDBTable[uNodeIndex].wTSC15_0;
		memcpy(pTransmitKey->abyKey,
		       &pMgmt->sNodeDBTable[uNodeIndex].abyWepKey[0],
		       pTransmitKey->uKeyLength
			);
	}
	vGenerateFIFOHeader(pDevice, byPktType, pDevice->pbyTmpBuff, bNeedEncryption,
			    cbFrameBodySize, TYPE_TXDMA0, pHeadTD,
			    &pDevice->sTxEthHeader, (unsigned char *)skb->data, pTransmitKey, uNodeIndex,
			    &uMACfragNum,
			    &cbHeaderSize
		);

	if (MACbIsRegBitsOn(pDevice->PortOffset, MAC_REG_PSCTL, PSCTL_PS)) {
		// Disable PS
		MACbPSWakeup(pDevice->PortOffset);
	}

	pDevice->bPWBitOn = false;

	pLastTD = pHeadTD;
	for (ii = 0; ii < uMACfragNum; ii++) {
		// Poll Transmit the adapter
		wmb();
		pHeadTD->m_td0TD0.f1Owner = OWNED_BY_NIC;
		wmb();
		if (ii == (uMACfragNum - 1))
			pLastTD = pHeadTD;
		pHeadTD = pHeadTD->next;
	}

	// Save the information needed by the tx interrupt handler
	// to complete the Send request
	pLastTD->pTDInfo->skb = skb;
	pLastTD->pTDInfo->byFlags = 0;
	pLastTD->pTDInfo->byFlags |= TD_FLAGS_NETIF_SKB;

	pDevice->apCurrTD[TYPE_TXDMA0] = pHeadTD;

	MACvTransmit0(pDevice->PortOffset);

	return true;
}

//TYPE_AC0DMA data tx
static int  device_xmit(struct sk_buff *skb, struct net_device *dev) {
	struct vnt_private *pDevice = netdev_priv(dev);
	PSMgmtObject    pMgmt = pDevice->pMgmt;
	PSTxDesc        pHeadTD, pLastTD;
	unsigned int uNodeIndex = 0;
	unsigned char byMask[8] = {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80};
	unsigned short wAID;
	unsigned int uMACfragNum = 1;
	unsigned int cbFrameBodySize;
	unsigned char byPktType;
	unsigned int cbHeaderSize;
	bool bNeedEncryption = false;
	PSKeyItem       pTransmitKey = NULL;
	SKeyItem        STempKey;
	unsigned int ii;
	bool bTKIP_UseGTK = false;
	bool bNeedDeAuth = false;
	unsigned char *pbyBSSID;
	bool bNodeExist = false;

	spin_lock_irq(&pDevice->lock);
	if (!pDevice->bLinkPass) {
		dev_kfree_skb_irq(skb);
		spin_unlock_irq(&pDevice->lock);
		return 0;
	}

	if (pDevice->bStopDataPkt) {
		dev_kfree_skb_irq(skb);
		spin_unlock_irq(&pDevice->lock);
		return 0;
	}

	if (pMgmt->eCurrMode == WMAC_MODE_ESS_AP) {
		if (pDevice->uAssocCount == 0) {
			dev_kfree_skb_irq(skb);
			spin_unlock_irq(&pDevice->lock);
			return 0;
		}
		if (is_multicast_ether_addr((unsigned char *)(skb->data))) {
			uNodeIndex = 0;
			bNodeExist = true;
			if (pMgmt->sNodeDBTable[0].bPSEnable) {
				skb_queue_tail(&(pMgmt->sNodeDBTable[0].sTxPSQueue), skb);
				pMgmt->sNodeDBTable[0].wEnQueueCnt++;
				// set tx map
				pMgmt->abyPSTxMap[0] |= byMask[0];
				spin_unlock_irq(&pDevice->lock);
				return 0;
			}
		} else {
			if (BSSDBbIsSTAInNodeDB(pMgmt, (unsigned char *)(skb->data), &uNodeIndex)) {
				if (pMgmt->sNodeDBTable[uNodeIndex].bPSEnable) {
					skb_queue_tail(&pMgmt->sNodeDBTable[uNodeIndex].sTxPSQueue, skb);
					pMgmt->sNodeDBTable[uNodeIndex].wEnQueueCnt++;
					// set tx map
					wAID = pMgmt->sNodeDBTable[uNodeIndex].wAID;
					pMgmt->abyPSTxMap[wAID >> 3] |=  byMask[wAID & 7];
					pr_debug("Set:pMgmt->abyPSTxMap[%d]= %d\n",
						 (wAID >> 3),
						 pMgmt->abyPSTxMap[wAID >> 3]);
					spin_unlock_irq(&pDevice->lock);
					return 0;
				}

				if (pMgmt->sNodeDBTable[uNodeIndex].bShortPreamble)
					pDevice->byPreambleType = pDevice->byShortPreamble;
				else
					pDevice->byPreambleType = PREAMBLE_LONG;

				bNodeExist = true;

			}
		}

		if (!bNodeExist) {
			pr_debug("Unknown STA not found in node DB\n");
			dev_kfree_skb_irq(skb);
			spin_unlock_irq(&pDevice->lock);
			return 0;
		}
	}

	pHeadTD = pDevice->apCurrTD[TYPE_AC0DMA];

	pHeadTD->m_td1TD1.byTCR = (TCR_EDP|TCR_STP);

	memcpy(pDevice->sTxEthHeader.abyDstAddr, (unsigned char *)(skb->data), ETH_HLEN);
	cbFrameBodySize = skb->len - ETH_HLEN;
	// 802.1H
	if (ntohs(pDevice->sTxEthHeader.wType) > ETH_DATA_LEN)
		cbFrameBodySize += 8;

	if (pDevice->bEncryptionEnable) {
		bNeedEncryption = true;
		// get Transmit key
		do {
			if ((pDevice->pMgmt->eCurrMode == WMAC_MODE_ESS_STA) &&
			    (pDevice->pMgmt->eCurrState == WMAC_STATE_ASSOC)) {
				pbyBSSID = pDevice->abyBSSID;
				// get pairwise key
				if (KeybGetTransmitKey(&(pDevice->sKey), pbyBSSID, PAIRWISE_KEY, &pTransmitKey) == false) {
					// get group key
					if (KeybGetTransmitKey(&(pDevice->sKey), pbyBSSID, GROUP_KEY, &pTransmitKey) == true) {
						bTKIP_UseGTK = true;
						pr_debug("Get GTK\n");
						break;
					}
				} else {
					pr_debug("Get PTK\n");
					break;
				}
			} else if (pDevice->pMgmt->eCurrMode == WMAC_MODE_IBSS_STA) {
				pbyBSSID = pDevice->sTxEthHeader.abyDstAddr;  //TO_DS = 0 and FROM_DS = 0 --> 802.11 MAC Address1
				pr_debug("IBSS Serach Key:\n");
				for (ii = 0; ii < 6; ii++)
					pr_debug("%x\n", *(pbyBSSID+ii));
				pr_debug("\n");

				// get pairwise key
				if (KeybGetTransmitKey(&(pDevice->sKey), pbyBSSID, PAIRWISE_KEY, &pTransmitKey) == true)
					break;
			}
			// get group key
			pbyBSSID = pDevice->abyBroadcastAddr;
			if (KeybGetTransmitKey(&(pDevice->sKey), pbyBSSID, GROUP_KEY, &pTransmitKey) == false) {
				pTransmitKey = NULL;
				if (pDevice->pMgmt->eCurrMode == WMAC_MODE_IBSS_STA)
					pr_debug("IBSS and KEY is NULL. [%d]\n",
						 pDevice->pMgmt->eCurrMode);
				else
					pr_debug("NOT IBSS and KEY is NULL. [%d]\n",
						 pDevice->pMgmt->eCurrMode);
			} else {
				bTKIP_UseGTK = true;
				pr_debug("Get GTK\n");
			}
		} while (false);
	}

	if (pDevice->bEnableHostWEP) {
		pr_debug("acdma0: STA index %d\n", uNodeIndex);
		if (pDevice->bEncryptionEnable) {
			pTransmitKey = &STempKey;
			pTransmitKey->byCipherSuite = pMgmt->sNodeDBTable[uNodeIndex].byCipherSuite;
			pTransmitKey->dwKeyIndex = pMgmt->sNodeDBTable[uNodeIndex].dwKeyIndex;
			pTransmitKey->uKeyLength = pMgmt->sNodeDBTable[uNodeIndex].uWepKeyLength;
			pTransmitKey->dwTSC47_16 = pMgmt->sNodeDBTable[uNodeIndex].dwTSC47_16;
			pTransmitKey->wTSC15_0 = pMgmt->sNodeDBTable[uNodeIndex].wTSC15_0;
			memcpy(pTransmitKey->abyKey,
			       &pMgmt->sNodeDBTable[uNodeIndex].abyWepKey[0],
			       pTransmitKey->uKeyLength
				);
		}
	}

	uMACfragNum = cbGetFragCount(pDevice, pTransmitKey, cbFrameBodySize, &pDevice->sTxEthHeader);

	if (uMACfragNum > AVAIL_TD(pDevice, TYPE_AC0DMA)) {
		pr_debug("uMACfragNum > AVAIL_TD(TYPE_AC0DMA) = %d\n",
			 uMACfragNum);
		dev_kfree_skb_irq(skb);
		spin_unlock_irq(&pDevice->lock);
		return 0;
	}

	if (pTransmitKey != NULL) {
		if ((pTransmitKey->byCipherSuite == KEY_CTL_WEP) &&
		    (pTransmitKey->uKeyLength == WLAN_WEP232_KEYLEN)) {
			uMACfragNum = 1; //WEP256 doesn't support fragment
		}
	}

	byPktType = (unsigned char)pDevice->byPacketType;

	if (pDevice->bFixRate) {
		if (pDevice->eCurrentPHYType == PHY_TYPE_11B) {
			if (pDevice->uConnectionRate >= RATE_11M)
				pDevice->wCurrentRate = RATE_11M;
			else
				pDevice->wCurrentRate = (unsigned short)pDevice->uConnectionRate;
		} else {
			if ((pDevice->eCurrentPHYType == PHY_TYPE_11A) &&
			    (pDevice->uConnectionRate <= RATE_6M)) {
				pDevice->wCurrentRate = RATE_6M;
			} else {
				if (pDevice->uConnectionRate >= RATE_54M)
					pDevice->wCurrentRate = RATE_54M;
				else
					pDevice->wCurrentRate = (unsigned short)pDevice->uConnectionRate;

			}
		}
		pDevice->byACKRate = (unsigned char) pDevice->wCurrentRate;
		pDevice->byTopCCKBasicRate = RATE_1M;
		pDevice->byTopOFDMBasicRate = RATE_6M;
	} else {
		//auto rate
		if (pDevice->sTxEthHeader.wType == TYPE_PKT_802_1x) {
			if (pDevice->eCurrentPHYType != PHY_TYPE_11A) {
				pDevice->wCurrentRate = RATE_1M;
				pDevice->byACKRate = RATE_1M;
				pDevice->byTopCCKBasicRate = RATE_1M;
				pDevice->byTopOFDMBasicRate = RATE_6M;
			} else {
				pDevice->wCurrentRate = RATE_6M;
				pDevice->byACKRate = RATE_6M;
				pDevice->byTopCCKBasicRate = RATE_1M;
				pDevice->byTopOFDMBasicRate = RATE_6M;
			}
		} else {
			VNTWIFIvGetTxRate(pDevice->pMgmt,
					  pDevice->sTxEthHeader.abyDstAddr,
					  &(pDevice->wCurrentRate),
					  &(pDevice->byACKRate),
					  &(pDevice->byTopCCKBasicRate),
					  &(pDevice->byTopOFDMBasicRate));

		}
	}


	if (pDevice->wCurrentRate <= RATE_11M) {
		byPktType = PK_TYPE_11B;
	} else if (pDevice->eCurrentPHYType == PHY_TYPE_11A) {
		byPktType = PK_TYPE_11A;
	} else {
		if (pDevice->bProtectMode)
			byPktType = PK_TYPE_11GB;
		else
			byPktType = PK_TYPE_11GA;
	}

	if (bNeedEncryption) {
		pr_debug("ntohs Pkt Type=%04x\n",
			 ntohs(pDevice->sTxEthHeader.wType));
		if ((pDevice->sTxEthHeader.wType) == TYPE_PKT_802_1x) {
			bNeedEncryption = false;
			pr_debug("Pkt Type=%04x\n",
				 (pDevice->sTxEthHeader.wType));
			if ((pDevice->pMgmt->eCurrMode == WMAC_MODE_ESS_STA) && (pDevice->pMgmt->eCurrState == WMAC_STATE_ASSOC)) {
				if (pTransmitKey == NULL) {
					pr_debug("Don't Find TX KEY\n");
				} else {
					if (bTKIP_UseGTK) {
						pr_debug("error: KEY is GTK!!~~\n");
					} else {
						pr_debug("Find PTK [%lX]\n",
							 pTransmitKey->dwKeyIndex);
						bNeedEncryption = true;
					}
				}
			}

			if (pDevice->byCntMeasure == 2) {
				bNeedDeAuth = true;
				pDevice->s802_11Counter.TKIPCounterMeasuresInvoked++;
			}

			if (pDevice->bEnableHostWEP) {
				if ((uNodeIndex != 0) &&
				    (pMgmt->sNodeDBTable[uNodeIndex].dwKeyIndex & PAIRWISE_KEY)) {
					pr_debug("Find PTK [%lX]\n",
						 pTransmitKey->dwKeyIndex);
					bNeedEncryption = true;
				}
			}
		} else {
			if (pTransmitKey == NULL) {
				pr_debug("return no tx key\n");
				dev_kfree_skb_irq(skb);
				spin_unlock_irq(&pDevice->lock);
				return 0;
			}
		}
	}

	vGenerateFIFOHeader(pDevice, byPktType, pDevice->pbyTmpBuff, bNeedEncryption,
			    cbFrameBodySize, TYPE_AC0DMA, pHeadTD,
			    &pDevice->sTxEthHeader, (unsigned char *)skb->data, pTransmitKey, uNodeIndex,
			    &uMACfragNum,
			    &cbHeaderSize
		);

	if (MACbIsRegBitsOn(pDevice->PortOffset, MAC_REG_PSCTL, PSCTL_PS)) {
		// Disable PS
		MACbPSWakeup(pDevice->PortOffset);
	}
	pDevice->bPWBitOn = false;

	pLastTD = pHeadTD;
	for (ii = 0; ii < uMACfragNum; ii++) {
		// Poll Transmit the adapter
		wmb();
		pHeadTD->m_td0TD0.f1Owner = OWNED_BY_NIC;
		wmb();
		if (ii == uMACfragNum - 1)
			pLastTD = pHeadTD;
		pHeadTD = pHeadTD->next;
	}

	// Save the information needed by the tx interrupt handler
	// to complete the Send request
	pLastTD->pTDInfo->skb = skb;
	pLastTD->pTDInfo->byFlags = 0;
	pLastTD->pTDInfo->byFlags |= TD_FLAGS_NETIF_SKB;
	pDevice->nTxDataTimeCout = 0; //2008-8-21 chester <add> for send null packet

	if (AVAIL_TD(pDevice, TYPE_AC0DMA) <= 1)
		netif_stop_queue(dev);

	pDevice->apCurrTD[TYPE_AC0DMA] = pHeadTD;

	if (pDevice->bFixRate)
		pr_debug("FixRate:Rate is %d,TxPower is %d\n", pDevice->wCurrentRate, pDevice->byCurPwr);

	{
		unsigned char Protocol_Version;    //802.1x Authentication
		unsigned char Packet_Type;           //802.1x Authentication
		unsigned char Descriptor_type;
		unsigned short Key_info;
		bool bTxeapol_key = false;

		Protocol_Version = skb->data[ETH_HLEN];
		Packet_Type = skb->data[ETH_HLEN+1];
		Descriptor_type = skb->data[ETH_HLEN+1+1+2];
		Key_info = (skb->data[ETH_HLEN+1+1+2+1] << 8)|(skb->data[ETH_HLEN+1+1+2+2]);
		if (pDevice->sTxEthHeader.wType == TYPE_PKT_802_1x) {
			if (((Protocol_Version == 1) || (Protocol_Version == 2)) &&
			    (Packet_Type == 3)) {  //802.1x OR eapol-key challenge frame transfer
				bTxeapol_key = true;
				if ((Descriptor_type == 254) || (Descriptor_type == 2)) {       //WPA or RSN
					if (!(Key_info & BIT3) &&   //group-key challenge
					    (Key_info & BIT8) && (Key_info & BIT9)) {    //send 2/2 key
						pDevice->fWPA_Authened = true;
						if (Descriptor_type == 254)
							pr_debug("WPA ");
						else
							pr_debug("WPA2 ");
						pr_debug("Authentication completed!!\n");
					}
				}
			}
		}
	}

	MACvTransmitAC0(pDevice->PortOffset);

	dev->trans_start = jiffies;

	spin_unlock_irq(&pDevice->lock);
	return 0;
}

static  irqreturn_t  device_intr(int irq,  void *dev_instance)
{
	struct net_device *dev = dev_instance;
	struct vnt_private *pDevice = netdev_priv(dev);
	int             max_count = 0;
	unsigned long dwMIBCounter = 0;
	PSMgmtObject    pMgmt = pDevice->pMgmt;
	unsigned char byOrgPageSel = 0;
	int             handled = 0;
	unsigned char byData = 0;
	int             ii = 0;
	unsigned long flags;

	MACvReadISR(pDevice->PortOffset, &pDevice->dwIsr);

	if (pDevice->dwIsr == 0)
		return IRQ_RETVAL(handled);

	if (pDevice->dwIsr == 0xffffffff) {
		pr_debug("dwIsr = 0xffff\n");
		return IRQ_RETVAL(handled);
	}

	handled = 1;
	MACvIntDisable(pDevice->PortOffset);

	spin_lock_irqsave(&pDevice->lock, flags);

	//Make sure current page is 0
	VNSvInPortB(pDevice->PortOffset + MAC_REG_PAGE1SEL, &byOrgPageSel);
	if (byOrgPageSel == 1)
		MACvSelectPage0(pDevice->PortOffset);
	else
		byOrgPageSel = 0;

	MACvReadMIBCounter(pDevice->PortOffset, &dwMIBCounter);
	// TBD....
	// Must do this after doing rx/tx, cause ISR bit is slow
	// than RD/TD write back
	// update ISR counter
	STAvUpdate802_11Counter(&pDevice->s802_11Counter, &pDevice->scStatistic , dwMIBCounter);
	while (pDevice->dwIsr != 0) {
		STAvUpdateIsrStatCounter(&pDevice->scStatistic, pDevice->dwIsr);
		MACvWriteISR(pDevice->PortOffset, pDevice->dwIsr);

		if (pDevice->dwIsr & ISR_FETALERR) {
			pr_debug(" ISR_FETALERR\n");
			VNSvOutPortB(pDevice->PortOffset + MAC_REG_SOFTPWRCTL, 0);
			VNSvOutPortW(pDevice->PortOffset + MAC_REG_SOFTPWRCTL, SOFTPWRCTL_SWPECTI);
			device_error(pDevice, pDevice->dwIsr);
		}

		if (pDevice->byLocalID > REV_ID_VT3253_B1) {
			if (pDevice->dwIsr & ISR_MEASURESTART) {
				// 802.11h measure start
				pDevice->byOrgChannel = pDevice->byCurrentCh;
				VNSvInPortB(pDevice->PortOffset + MAC_REG_RCR, &(pDevice->byOrgRCR));
				VNSvOutPortB(pDevice->PortOffset + MAC_REG_RCR, (RCR_RXALLTYPE | RCR_UNICAST | RCR_BROADCAST | RCR_MULTICAST | RCR_WPAERR));
				MACvSelectPage1(pDevice->PortOffset);
				VNSvInPortD(pDevice->PortOffset + MAC_REG_MAR0, &(pDevice->dwOrgMAR0));
				VNSvInPortD(pDevice->PortOffset + MAC_REG_MAR4, &(pDevice->dwOrgMAR4));
				MACvSelectPage0(pDevice->PortOffset);
				//xxxx
				if (set_channel(pDevice, pDevice->pCurrMeasureEID->sReq.byChannel)) {
					pDevice->bMeasureInProgress = true;
					MACvSelectPage1(pDevice->PortOffset);
					MACvRegBitsOn(pDevice->PortOffset, MAC_REG_MSRCTL, MSRCTL_READY);
					MACvSelectPage0(pDevice->PortOffset);
					pDevice->byBasicMap = 0;
					pDevice->byCCAFraction = 0;
					for (ii = 0; ii < 8; ii++)
						pDevice->dwRPIs[ii] = 0;

				} else {
					// can not measure because set channel fail
					// clear measure control
					MACvRegBitsOff(pDevice->PortOffset, MAC_REG_MSRCTL, MSRCTL_EN);
					s_vCompleteCurrentMeasure(pDevice, MEASURE_MODE_INCAPABLE);
					MACvSelectPage1(pDevice->PortOffset);
					MACvRegBitsOn(pDevice->PortOffset, MAC_REG_MSRCTL+1, MSRCTL1_TXPAUSE);
					MACvSelectPage0(pDevice->PortOffset);
				}
			}
			if (pDevice->dwIsr & ISR_MEASUREEND) {
				// 802.11h measure end
				pDevice->bMeasureInProgress = false;
				VNSvOutPortB(pDevice->PortOffset + MAC_REG_RCR, pDevice->byOrgRCR);
				MACvSelectPage1(pDevice->PortOffset);
				VNSvOutPortD(pDevice->PortOffset + MAC_REG_MAR0, pDevice->dwOrgMAR0);
				VNSvOutPortD(pDevice->PortOffset + MAC_REG_MAR4, pDevice->dwOrgMAR4);
				VNSvInPortB(pDevice->PortOffset + MAC_REG_MSRBBSTS, &byData);
				pDevice->byBasicMap |= (byData >> 4);
				VNSvInPortB(pDevice->PortOffset + MAC_REG_CCAFRACTION, &pDevice->byCCAFraction);
				VNSvInPortB(pDevice->PortOffset + MAC_REG_MSRCTL, &byData);
				// clear measure control
				MACvRegBitsOff(pDevice->PortOffset, MAC_REG_MSRCTL, MSRCTL_EN);
				MACvSelectPage0(pDevice->PortOffset);
				set_channel(pDevice, pDevice->byOrgChannel);
				MACvSelectPage1(pDevice->PortOffset);
				MACvRegBitsOn(pDevice->PortOffset, MAC_REG_MSRCTL+1, MSRCTL1_TXPAUSE);
				MACvSelectPage0(pDevice->PortOffset);
				if (byData & MSRCTL_FINISH) {
					// measure success
					s_vCompleteCurrentMeasure(pDevice, 0);
				} else {
					// can not measure because not ready before end of measure time
					s_vCompleteCurrentMeasure(pDevice, MEASURE_MODE_LATE);
				}
			}
			if (pDevice->dwIsr & ISR_QUIETSTART) {
				do {
					;
				} while (!CARDbStartQuiet(pDevice));
			}
		}

		if (pDevice->dwIsr & ISR_TBTT) {
			if (pDevice->bEnableFirstQuiet) {
				pDevice->byQuietStartCount--;
				if (pDevice->byQuietStartCount == 0) {
					pDevice->bEnableFirstQuiet = false;
					MACvSelectPage1(pDevice->PortOffset);
					MACvRegBitsOn(pDevice->PortOffset, MAC_REG_MSRCTL, (MSRCTL_QUIETTXCHK | MSRCTL_QUIETEN));
					MACvSelectPage0(pDevice->PortOffset);
				}
			}
			if (pDevice->bChannelSwitch &&
			    (pDevice->op_mode == NL80211_IFTYPE_STATION)) {
				pDevice->byChannelSwitchCount--;
				if (pDevice->byChannelSwitchCount == 0) {
					pDevice->bChannelSwitch = false;
					set_channel(pDevice, pDevice->byNewChannel);
					VNTWIFIbChannelSwitch(pDevice->pMgmt, pDevice->byNewChannel);
					MACvSelectPage1(pDevice->PortOffset);
					MACvRegBitsOn(pDevice->PortOffset, MAC_REG_MSRCTL+1, MSRCTL1_TXPAUSE);
					MACvSelectPage0(pDevice->PortOffset);
					CARDbStartTxPacket(pDevice, PKT_TYPE_802_11_ALL);

				}
			}
			if (pDevice->op_mode != NL80211_IFTYPE_ADHOC) {
				if ((pDevice->bUpdateBBVGA) && pDevice->bLinkPass && (pDevice->uCurrRSSI != 0)) {
					long            ldBm;

					RFvRSSITodBm(pDevice, (unsigned char) pDevice->uCurrRSSI, &ldBm);
					for (ii = 0; ii < BB_VGA_LEVEL; ii++) {
						if (ldBm < pDevice->ldBmThreshold[ii]) {
							pDevice->byBBVGANew = pDevice->abyBBVGA[ii];
							break;
						}
					}
					if (pDevice->byBBVGANew != pDevice->byBBVGACurrent) {
						pDevice->uBBVGADiffCount++;
						if (pDevice->uBBVGADiffCount == 1) {
							// first VGA diff gain
							BBvSetVGAGainOffset(pDevice, pDevice->byBBVGANew);
							pr_debug("First RSSI[%d] NewGain[%d] OldGain[%d] Count[%d]\n",
								 (int)ldBm,
								 pDevice->byBBVGANew,
								 pDevice->byBBVGACurrent,
								 (int)pDevice->uBBVGADiffCount);
						}
						if (pDevice->uBBVGADiffCount >= BB_VGA_CHANGE_THRESHOLD) {
							pr_debug("RSSI[%d] NewGain[%d] OldGain[%d] Count[%d]\n",
								 (int)ldBm,
								 pDevice->byBBVGANew,
								 pDevice->byBBVGACurrent,
								 (int)pDevice->uBBVGADiffCount);
							BBvSetVGAGainOffset(pDevice, pDevice->byBBVGANew);
						}
					} else {
						pDevice->uBBVGADiffCount = 1;
					}
				}
			}

			pDevice->bBeaconSent = false;
			if (pDevice->bEnablePSMode)
				PSbIsNextTBTTWakeUp((void *)pDevice);

			if ((pDevice->op_mode == NL80211_IFTYPE_AP) ||
			    (pDevice->op_mode == NL80211_IFTYPE_ADHOC)) {
				MACvOneShotTimer1MicroSec(pDevice->PortOffset,
							  (pMgmt->wIBSSBeaconPeriod - MAKE_BEACON_RESERVED) << 10);
			}

			/* TODO: adhoc PS mode */

		}

		if (pDevice->dwIsr & ISR_BNTX) {
			if (pDevice->op_mode == NL80211_IFTYPE_ADHOC) {
				pDevice->bIsBeaconBufReadySet = false;
				pDevice->cbBeaconBufReadySetCnt = 0;
			}

			if (pDevice->op_mode == NL80211_IFTYPE_AP) {
				if (pMgmt->byDTIMCount > 0) {
					pMgmt->byDTIMCount--;
					pMgmt->sNodeDBTable[0].bRxPSPoll = false;
				} else {
					if (pMgmt->byDTIMCount == 0) {
						// check if mutltcast tx bufferring
						pMgmt->byDTIMCount = pMgmt->byDTIMPeriod - 1;
						pMgmt->sNodeDBTable[0].bRxPSPoll = true;
						bScheduleCommand((void *)pDevice, WLAN_CMD_RX_PSPOLL, NULL);
					}
				}
			}
			pDevice->bBeaconSent = true;

			if (pDevice->bChannelSwitch) {
				pDevice->byChannelSwitchCount--;
				if (pDevice->byChannelSwitchCount == 0) {
					pDevice->bChannelSwitch = false;
					set_channel(pDevice, pDevice->byNewChannel);
					VNTWIFIbChannelSwitch(pDevice->pMgmt, pDevice->byNewChannel);
					MACvSelectPage1(pDevice->PortOffset);
					MACvRegBitsOn(pDevice->PortOffset, MAC_REG_MSRCTL+1, MSRCTL1_TXPAUSE);
					MACvSelectPage0(pDevice->PortOffset);
					CARDbStartTxPacket(pDevice, PKT_TYPE_802_11_ALL);
				}
			}

		}

		if (pDevice->dwIsr & ISR_RXDMA0)
			max_count += device_rx_srv(pDevice, TYPE_RXDMA0);

		if (pDevice->dwIsr & ISR_RXDMA1)
			max_count += device_rx_srv(pDevice, TYPE_RXDMA1);

		if (pDevice->dwIsr & ISR_TXDMA0)
			max_count += device_tx_srv(pDevice, TYPE_TXDMA0);

		if (pDevice->dwIsr & ISR_AC0DMA)
			max_count += device_tx_srv(pDevice, TYPE_AC0DMA);

		if (pDevice->dwIsr & ISR_SOFTTIMER1) {
			if (pDevice->op_mode == NL80211_IFTYPE_AP) {
				if (pDevice->bShortSlotTime)
					pMgmt->wCurrCapInfo |= WLAN_SET_CAP_INFO_SHORTSLOTTIME(1);
				else
					pMgmt->wCurrCapInfo &= ~(WLAN_SET_CAP_INFO_SHORTSLOTTIME(1));
			}
			bMgrPrepareBeaconToSend(pDevice, pMgmt);
			pDevice->byCntMeasure = 0;
		}

		MACvReadISR(pDevice->PortOffset, &pDevice->dwIsr);

		MACvReceive0(pDevice->PortOffset);
		MACvReceive1(pDevice->PortOffset);

		if (max_count > pDevice->sOpts.int_works)
			break;
	}

	if (byOrgPageSel == 1)
		MACvSelectPage1(pDevice->PortOffset);

	spin_unlock_irqrestore(&pDevice->lock, flags);

	MACvIntEnable(pDevice->PortOffset, IMR_MASK_VALUE);

	return IRQ_RETVAL(handled);
}

//2008-8-4 <add> by chester
static int Config_FileGetParameter(unsigned char *string,
				   unsigned char *dest, unsigned char *source)
{
	unsigned char buf1[100];
	int source_len = strlen(source);

	memset(buf1, 0, 100);
	strcat(buf1, string);
	strcat(buf1, "=");
	source += strlen(buf1);

	memcpy(dest, source, source_len - strlen(buf1));
	return true;
}

int Config_FileOperation(struct vnt_private *pDevice,
			 bool fwrite, unsigned char *Parameter)
{
	unsigned char *buffer = kmalloc(1024, GFP_KERNEL);
	unsigned char tmpbuffer[20];
	struct file *file;
	int result = 0;

	if (!buffer) {
		pr_err("allocate mem for file fail?\n");
		return -1;
	}
	file = filp_open(CONFIG_PATH, O_RDONLY, 0);
	if (IS_ERR(file)) {
		kfree(buffer);
		pr_err("Config_FileOperation:open file fail?\n");
		return -1;
	}

	if (kernel_read(file, 0, buffer, 1024) < 0) {
		pr_err("read file error?\n");
		result = -1;
		goto error1;
	}

	if (Config_FileGetParameter("ZONETYPE", tmpbuffer, buffer) != true) {
		pr_err("get parameter error?\n");
		result = -1;
		goto error1;
	}

	if (memcmp(tmpbuffer, "USA", 3) == 0) {
		result = ZoneType_USA;
	} else if (memcmp(tmpbuffer, "JAPAN", 5) == 0) {
		result = ZoneType_Japan;
	} else if (memcmp(tmpbuffer, "EUROPE", 5) == 0) {
		result = ZoneType_Europe;
	} else {
		result = -1;
		pr_err("Unknown Zonetype[%s]?\n", tmpbuffer);
	}

error1:
	kfree(buffer);
	fput(file);
	return result;
}

static void device_set_multi(struct net_device *dev) {
	struct vnt_private *pDevice = netdev_priv(dev);
	PSMgmtObject     pMgmt = pDevice->pMgmt;
	u32              mc_filter[2];
	struct netdev_hw_addr *ha;

	VNSvInPortB(pDevice->PortOffset + MAC_REG_RCR, &(pDevice->byRxMode));

	if (dev->flags & IFF_PROMISC) {         /* Set promiscuous. */
		pr_notice("%s: Promiscuous mode enabled\n", dev->name);
		/* Unconditionally log net taps. */
		pDevice->byRxMode |= (RCR_MULTICAST|RCR_BROADCAST|RCR_UNICAST);
	} else if ((netdev_mc_count(dev) > pDevice->multicast_limit)
		 ||  (dev->flags & IFF_ALLMULTI)) {
		MACvSelectPage1(pDevice->PortOffset);
		VNSvOutPortD(pDevice->PortOffset + MAC_REG_MAR0, 0xffffffff);
		VNSvOutPortD(pDevice->PortOffset + MAC_REG_MAR0 + 4, 0xffffffff);
		MACvSelectPage0(pDevice->PortOffset);
		pDevice->byRxMode |= (RCR_MULTICAST|RCR_BROADCAST);
	} else {
		memset(mc_filter, 0, sizeof(mc_filter));
		netdev_for_each_mc_addr(ha, dev) {
			int bit_nr = ether_crc(ETH_ALEN, ha->addr) >> 26;

			mc_filter[bit_nr >> 5] |= cpu_to_le32(1 << (bit_nr & 31));
		}
		MACvSelectPage1(pDevice->PortOffset);
		VNSvOutPortD(pDevice->PortOffset + MAC_REG_MAR0, mc_filter[0]);
		VNSvOutPortD(pDevice->PortOffset + MAC_REG_MAR0 + 4, mc_filter[1]);
		MACvSelectPage0(pDevice->PortOffset);
		pDevice->byRxMode &= ~(RCR_UNICAST);
		pDevice->byRxMode |= (RCR_MULTICAST|RCR_BROADCAST);
	}

	if (pMgmt->eConfigMode == WMAC_CONFIG_AP) {
		// If AP mode, don't enable RCR_UNICAST. Since hw only compare addr1 with local mac.
		pDevice->byRxMode |= (RCR_MULTICAST|RCR_BROADCAST);
		pDevice->byRxMode &= ~(RCR_UNICAST);
	}

	VNSvOutPortB(pDevice->PortOffset + MAC_REG_RCR, pDevice->byRxMode);
	pr_debug("pDevice->byRxMode = %x\n", pDevice->byRxMode);
}

static int  device_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct vnt_private *pDevice = netdev_priv(dev);
	struct iwreq *wrq = (struct iwreq *)rq;
	int rc = 0;
	PSMgmtObject pMgmt = pDevice->pMgmt;
	PSCmdRequest pReq;

	if (pMgmt == NULL) {
		rc = -EFAULT;
		return rc;
	}

	switch (cmd) {
	case SIOCGIWNAME:
		rc = iwctl_giwname(dev, NULL, (char *)&(wrq->u.name), NULL);
		break;

	case SIOCGIWNWID:     //0x8b03  support
		rc = -EOPNOTSUPP;
		break;

		// Set frequency/channel
	case SIOCSIWFREQ:
		rc = iwctl_siwfreq(dev, NULL, &(wrq->u.freq), NULL);
		break;

		// Get frequency/channel
	case SIOCGIWFREQ:
		rc = iwctl_giwfreq(dev, NULL, &(wrq->u.freq), NULL);
		break;

		// Set desired network name (ESSID)
	case SIOCSIWESSID:

	{
		char essid[IW_ESSID_MAX_SIZE+1];

		if (wrq->u.essid.length > IW_ESSID_MAX_SIZE) {
			rc = -E2BIG;
			break;
		}
		if (copy_from_user(essid, wrq->u.essid.pointer,
				   wrq->u.essid.length)) {
			rc = -EFAULT;
			break;
		}
		rc = iwctl_siwessid(dev, NULL,
				    &(wrq->u.essid), essid);
	}
	break;

	// Get current network name (ESSID)
	case SIOCGIWESSID:

	{
		char essid[IW_ESSID_MAX_SIZE+1];

		if (wrq->u.essid.pointer)
			rc = iwctl_giwessid(dev, NULL,
					    &(wrq->u.essid), essid);
		if (copy_to_user(wrq->u.essid.pointer,
				 essid,
				 wrq->u.essid.length))
			rc = -EFAULT;
	}
	break;

	case SIOCSIWAP:

		rc = iwctl_siwap(dev, NULL, &(wrq->u.ap_addr), NULL);
		break;

		// Get current Access Point (BSSID)
	case SIOCGIWAP:
		rc = iwctl_giwap(dev, NULL, &(wrq->u.ap_addr), NULL);
		break;

		// Set desired station name
	case SIOCSIWNICKN:
		pr_debug(" SIOCSIWNICKN\n");
		rc = -EOPNOTSUPP;
		break;

		// Get current station name
	case SIOCGIWNICKN:
		pr_debug(" SIOCGIWNICKN\n");
		rc = -EOPNOTSUPP;
		break;

		// Set the desired bit-rate
	case SIOCSIWRATE:
		rc = iwctl_siwrate(dev, NULL, &(wrq->u.bitrate), NULL);
		break;

		// Get the current bit-rate
	case SIOCGIWRATE:

		rc = iwctl_giwrate(dev, NULL, &(wrq->u.bitrate), NULL);
		break;

		// Set the desired RTS threshold
	case SIOCSIWRTS:

		rc = iwctl_siwrts(dev, NULL, &(wrq->u.rts), NULL);
		break;

		// Get the current RTS threshold
	case SIOCGIWRTS:

		rc = iwctl_giwrts(dev, NULL, &(wrq->u.rts), NULL);
		break;

		// Set the desired fragmentation threshold
	case SIOCSIWFRAG:

		rc = iwctl_siwfrag(dev, NULL, &(wrq->u.frag), NULL);
		break;

		// Get the current fragmentation threshold
	case SIOCGIWFRAG:

		rc = iwctl_giwfrag(dev, NULL, &(wrq->u.frag), NULL);
		break;

		// Set mode of operation
	case SIOCSIWMODE:
		rc = iwctl_siwmode(dev, NULL, &(wrq->u.mode), NULL);
		break;

		// Get mode of operation
	case SIOCGIWMODE:
		rc = iwctl_giwmode(dev, NULL, &(wrq->u.mode), NULL);
		break;

		// Set WEP keys and mode
	case SIOCSIWENCODE: {
		char abyKey[WLAN_WEP232_KEYLEN];

		if (wrq->u.encoding.pointer) {
			if (wrq->u.encoding.length > WLAN_WEP232_KEYLEN) {
				rc = -E2BIG;
				break;
			}
			memset(abyKey, 0, WLAN_WEP232_KEYLEN);
			if (copy_from_user(abyKey,
					   wrq->u.encoding.pointer,
					   wrq->u.encoding.length)) {
				rc = -EFAULT;
				break;
			}
		} else if (wrq->u.encoding.length != 0) {
			rc = -EINVAL;
			break;
		}
		rc = iwctl_siwencode(dev, NULL, &(wrq->u.encoding), abyKey);
	}
	break;

	// Get the WEP keys and mode
	case SIOCGIWENCODE:

		if (!capable(CAP_NET_ADMIN)) {
			rc = -EPERM;
			break;
		}
		{
			char abyKey[WLAN_WEP232_KEYLEN];

			rc = iwctl_giwencode(dev, NULL, &(wrq->u.encoding), abyKey);
			if (rc != 0)
				break;
			if (wrq->u.encoding.pointer) {
				if (copy_to_user(wrq->u.encoding.pointer,
						 abyKey,
						 wrq->u.encoding.length))
					rc = -EFAULT;
			}
		}
		break;

		// Get the current Tx-Power
	case SIOCGIWTXPOW:
		pr_debug(" SIOCGIWTXPOW\n");
		rc = -EOPNOTSUPP;
		break;

	case SIOCSIWTXPOW:
		pr_debug(" SIOCSIWTXPOW\n");
		rc = -EOPNOTSUPP;
		break;

	case SIOCSIWRETRY:

		rc = iwctl_siwretry(dev, NULL, &(wrq->u.retry), NULL);
		break;

	case SIOCGIWRETRY:

		rc = iwctl_giwretry(dev, NULL, &(wrq->u.retry), NULL);
		break;

		// Get range of parameters
	case SIOCGIWRANGE:

	{
		struct iw_range range;

		rc = iwctl_giwrange(dev, NULL, &(wrq->u.data), (char *)&range);
		if (copy_to_user(wrq->u.data.pointer, &range, sizeof(struct iw_range)))
			rc = -EFAULT;
	}

	break;

	case SIOCGIWPOWER:

		rc = iwctl_giwpower(dev, NULL, &(wrq->u.power), NULL);
		break;

	case SIOCSIWPOWER:

		rc = iwctl_siwpower(dev, NULL, &(wrq->u.power), NULL);
		break;

	case SIOCGIWSENS:

		rc = iwctl_giwsens(dev, NULL, &(wrq->u.sens), NULL);
		break;

	case SIOCSIWSENS:
		pr_debug(" SIOCSIWSENS\n");
		rc = -EOPNOTSUPP;
		break;

	case SIOCGIWAPLIST: {
		char buffer[IW_MAX_AP * (sizeof(struct sockaddr) + sizeof(struct iw_quality))];

		if (wrq->u.data.pointer) {
			rc = iwctl_giwaplist(dev, NULL, &(wrq->u.data), buffer);
			if (rc == 0) {
				if (copy_to_user(wrq->u.data.pointer,
						 buffer,
						 (wrq->u.data.length * (sizeof(struct sockaddr) +  sizeof(struct iw_quality)))
					    ))
					rc = -EFAULT;
			}
		}
	}
	break;

#ifdef WIRELESS_SPY
	// Set the spy list
	case SIOCSIWSPY:

		pr_debug(" SIOCSIWSPY\n");
		rc = -EOPNOTSUPP;
		break;

		// Get the spy list
	case SIOCGIWSPY:

		pr_debug(" SIOCGIWSPY\n");
		rc = -EOPNOTSUPP;
		break;

#endif // WIRELESS_SPY

	case SIOCGIWPRIV:
		pr_debug(" SIOCGIWPRIV\n");
		rc = -EOPNOTSUPP;
		break;

//2008-0409-07, <Add> by Einsn Liu
#ifdef WPA_SUPPLICANT_DRIVER_WEXT_SUPPORT
	case SIOCSIWAUTH:
		pr_debug(" SIOCSIWAUTH\n");
		rc = iwctl_siwauth(dev, NULL, &(wrq->u.param), NULL);
		break;

	case SIOCGIWAUTH:
		pr_debug(" SIOCGIWAUTH\n");
		rc = iwctl_giwauth(dev, NULL, &(wrq->u.param), NULL);
		break;

	case SIOCSIWGENIE:
		pr_debug(" SIOCSIWGENIE\n");
		rc = iwctl_siwgenie(dev, NULL, &(wrq->u.data), wrq->u.data.pointer);
		break;

	case SIOCGIWGENIE:
		pr_debug(" SIOCGIWGENIE\n");
		rc = iwctl_giwgenie(dev, NULL, &(wrq->u.data), wrq->u.data.pointer);
		break;

	case SIOCSIWENCODEEXT: {
		char extra[sizeof(struct iw_encode_ext)+MAX_KEY_LEN+1];

		pr_debug(" SIOCSIWENCODEEXT\n");
		if (wrq->u.encoding.pointer) {
			memset(extra, 0, sizeof(struct iw_encode_ext)+MAX_KEY_LEN + 1);
			if (wrq->u.encoding.length > (sizeof(struct iw_encode_ext) + MAX_KEY_LEN)) {
				rc = -E2BIG;
				break;
			}
			if (copy_from_user(extra, wrq->u.encoding.pointer, wrq->u.encoding.length)) {
				rc = -EFAULT;
				break;
			}
		} else if (wrq->u.encoding.length != 0) {
			rc = -EINVAL;
			break;
		}
		rc = iwctl_siwencodeext(dev, NULL, &(wrq->u.encoding), extra);
	}
	break;

	case SIOCGIWENCODEEXT:
		pr_debug(" SIOCGIWENCODEEXT\n");
		rc = iwctl_giwencodeext(dev, NULL, &(wrq->u.encoding), NULL);
		break;

	case SIOCSIWMLME:
		pr_debug(" SIOCSIWMLME\n");
		rc = iwctl_siwmlme(dev, NULL, &(wrq->u.data), wrq->u.data.pointer);
		break;

#endif // #ifdef WPA_SUPPLICANT_DRIVER_WEXT_SUPPORT
//End Add -- //2008-0409-07, <Add> by Einsn Liu

	case IOCTL_CMD_TEST:

		if (!(pDevice->flags & DEVICE_FLAGS_OPENED)) {
			rc = -EFAULT;
			break;
		} else {
			rc = 0;
		}
		pReq = (PSCmdRequest)rq;
		pReq->wResult = MAGIC_CODE;
		break;

	case IOCTL_CMD_SET:

#ifdef SndEvt_ToAPI
		if ((((PSCmdRequest)rq)->wCmdCode != WLAN_CMD_SET_EVT) &&
		    !(pDevice->flags & DEVICE_FLAGS_OPENED))
#else
			if (!(pDevice->flags & DEVICE_FLAGS_OPENED) &&
			    (((PSCmdRequest)rq)->wCmdCode != WLAN_CMD_SET_WPA))
#endif
			{
				rc = -EFAULT;
				break;
			} else {
				rc = 0;
			}

		if (test_and_set_bit(0, (void *)&(pMgmt->uCmdBusy)))
			return -EBUSY;

		rc = private_ioctl(pDevice, rq);
		clear_bit(0, (void *)&(pMgmt->uCmdBusy));
		break;

	case IOCTL_CMD_HOSTAPD:

		rc = vt6655_hostap_ioctl(pDevice, &wrq->u.data);
		break;

	case IOCTL_CMD_WPA:

		rc = wpa_ioctl(pDevice, &wrq->u.data);
		break;

	case SIOCETHTOOL:
		return ethtool_ioctl(dev, rq->ifr_data);
		// All other calls are currently unsupported

	default:
		rc = -EOPNOTSUPP;
		pr_debug("Ioctl command not support..%x\n", cmd);

	}

	if (pDevice->bCommit) {
		if (pMgmt->eConfigMode == WMAC_CONFIG_AP) {
			netif_stop_queue(pDevice->dev);
			spin_lock_irq(&pDevice->lock);
			bScheduleCommand((void *)pDevice, WLAN_CMD_RUN_AP, NULL);
			spin_unlock_irq(&pDevice->lock);
		} else {
			pr_debug("Commit the settings\n");
			spin_lock_irq(&pDevice->lock);
			pDevice->bLinkPass = false;
			memset(pMgmt->abyCurrBSSID, 0, 6);
			pMgmt->eCurrState = WMAC_STATE_IDLE;
			netif_stop_queue(pDevice->dev);
#ifdef WPA_SUPPLICANT_DRIVER_WEXT_SUPPORT
			pMgmt->eScanType = WMAC_SCAN_ACTIVE;
			if (!pDevice->bWPASuppWextEnabled)
#endif
				bScheduleCommand((void *)pDevice, WLAN_CMD_BSSID_SCAN, pMgmt->abyDesireSSID);
			bScheduleCommand((void *)pDevice, WLAN_CMD_SSID, NULL);
			spin_unlock_irq(&pDevice->lock);
		}
		pDevice->bCommit = false;
	}

	return rc;
}

static int ethtool_ioctl(struct net_device *dev, void __user *useraddr)
{
	u32 ethcmd;

	if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
		return -EFAULT;

	switch (ethcmd) {
	case ETHTOOL_GDRVINFO: {
		struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};

		strncpy(info.driver, DEVICE_NAME, sizeof(info.driver)-1);
		strncpy(info.version, DEVICE_VERSION, sizeof(info.version)-1);
		if (copy_to_user(useraddr, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}

	}

	return -EOPNOTSUPP;
}

/*------------------------------------------------------------------*/

MODULE_DEVICE_TABLE(pci, vt6655_pci_id_table);

static struct pci_driver device_driver = {
	.name = DEVICE_NAME,
	.id_table = vt6655_pci_id_table,
	.probe = vt6655_probe,
	.remove = vt6655_remove,
#ifdef CONFIG_PM
	.suspend = viawget_suspend,
	.resume = viawget_resume,
#endif
};

static int __init vt6655_init_module(void)
{
	int ret;

	ret = pci_register_driver(&device_driver);
#ifdef CONFIG_PM
	if (ret >= 0)
		register_reboot_notifier(&device_notifier);
#endif

	return ret;
}

static void __exit vt6655_cleanup_module(void)
{
#ifdef CONFIG_PM
	unregister_reboot_notifier(&device_notifier);
#endif
	pci_unregister_driver(&device_driver);
}

module_init(vt6655_init_module);
module_exit(vt6655_cleanup_module);

#ifdef CONFIG_PM
static int
device_notify_reboot(struct notifier_block *nb, unsigned long event, void *p)
{
	struct pci_dev *pdev = NULL;

	switch (event) {
	case SYS_DOWN:
	case SYS_HALT:
	case SYS_POWER_OFF:
		for_each_pci_dev(pdev) {
			if (pci_dev_driver(pdev) == &device_driver) {
				if (pci_get_drvdata(pdev))
					viawget_suspend(pdev, PMSG_HIBERNATE);
			}
		}
	}
	return NOTIFY_DONE;
}

static int
viawget_suspend(struct pci_dev *pcid, pm_message_t state)
{
	int power_status;   // to silence the compiler

	struct vnt_private *pDevice = pci_get_drvdata(pcid);
	PSMgmtObject  pMgmt = pDevice->pMgmt;

	netif_stop_queue(pDevice->dev);
	spin_lock_irq(&pDevice->lock);
	pci_save_state(pcid);
	del_timer(&pDevice->sTimerCommand);
	del_timer(&pMgmt->sTimerSecondCallback);
	pDevice->cbFreeCmdQueue = CMD_Q_SIZE;
	pDevice->uCmdDequeueIdx = 0;
	pDevice->uCmdEnqueueIdx = 0;
	pDevice->bCmdRunning = false;
	MACbShutdown(pDevice->PortOffset);
	MACvSaveContext(pDevice->PortOffset, pDevice->abyMacContext);
	pDevice->bLinkPass = false;
	memset(pMgmt->abyCurrBSSID, 0, 6);
	pMgmt->eCurrState = WMAC_STATE_IDLE;
	pci_disable_device(pcid);
	power_status = pci_set_power_state(pcid, pci_choose_state(pcid, state));
	spin_unlock_irq(&pDevice->lock);
	return 0;
}

static int
viawget_resume(struct pci_dev *pcid)
{
	struct vnt_private *pDevice = pci_get_drvdata(pcid);
	PSMgmtObject  pMgmt = pDevice->pMgmt;
	int power_status;   // to silence the compiler

	power_status = pci_set_power_state(pcid, PCI_D0);
	power_status = pci_enable_wake(pcid, PCI_D0, 0);
	pci_restore_state(pcid);
	if (netif_running(pDevice->dev)) {
		spin_lock_irq(&pDevice->lock);
		MACvRestoreContext(pDevice->PortOffset, pDevice->abyMacContext);
		device_init_registers(pDevice);
		if (pMgmt->sNodeDBTable[0].bActive) { // Assoc with BSS
			pMgmt->sNodeDBTable[0].bActive = false;
			pDevice->bLinkPass = false;
			if (pMgmt->eCurrMode == WMAC_MODE_IBSS_STA) {
				// In Adhoc, BSS state set back to started.
				pMgmt->eCurrState = WMAC_STATE_STARTED;
			} else {
				pMgmt->eCurrMode = WMAC_MODE_STANDBY;
				pMgmt->eCurrState = WMAC_STATE_IDLE;
			}
		}
		init_timer(&pMgmt->sTimerSecondCallback);
		init_timer(&pDevice->sTimerCommand);
		MACvIntEnable(pDevice->PortOffset, IMR_MASK_VALUE);
		BSSvClearBSSList((void *)pDevice, pDevice->bLinkPass);
		bScheduleCommand((void *)pDevice, WLAN_CMD_BSSID_SCAN, NULL);
		bScheduleCommand((void *)pDevice, WLAN_CMD_SSID, NULL);
		spin_unlock_irq(&pDevice->lock);
	}
	return 0;
}

#endif
