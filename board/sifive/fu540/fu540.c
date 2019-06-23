// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 * 
 * Copyright (C) 2018 Microsemi Corporation.
 * Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
 * 
 * Copyright (C) 2018 Joey Hewitt <joey@joeyhewitt.com>
 * Copyright (C) 2019 Troy Benjegerdes <troy.benjegerdes@sifive.com>
 */

#include <common.h>
#include <dm.h>
#if defined(CONFIG_SIFIVE_LEGACY_MEMORY_INIT)
#include "ddrregs.h"

DECLARE_GLOBAL_DATA_PTR;

struct hifive_gpio_regs
{
    volatile uint32_t  INPUT_VAL;   /* 0x0000 */
    volatile uint32_t  INPUT_EN;    /* 0x0004 */
    volatile uint32_t  OUTPUT_VAL;  /* 0x0008 */
    volatile uint32_t  OUTPUT_EN;   /* 0x000C */
    volatile uint32_t  PUE;	    /* 0x0010 */
    volatile uint32_t  DS;	    /* 0x0014 */
    volatile uint32_t  RISE_IE;     /* 0x0018 */
    volatile uint32_t  RISE_IP;     /* 0x001C */
    volatile uint32_t  FALL_IE;     /* 0x0020 */
    volatile uint32_t  FALL_IP;     /* 0x0024 */
    volatile uint32_t  HIGH_IE;     /* 0x0028 */
    volatile uint32_t  HIGH_IP;     /* 0x002C */
    volatile uint32_t  LOW_IE;	    /* 0x0030 */
    volatile uint32_t  LOW_IP;	    /* 0x0034 */
    volatile uint32_t  reserved0;   /* 0x0038 */
    volatile uint32_t  reserved1;   /* 0x003C */
    volatile uint32_t  OUT_XOR;     /* 0x0040 */
};

struct hifive_gpio_regs *g_aloe_gpio = (struct hifive_gpio_regs *)HIFIVE_BASE_GPIO;
#endif

#include <linux/delay.h>
#include <linux/io.h>

#ifdef CONFIG_MISC_INIT_R

#define FU540_OTP_BASE_ADDR			0x10070000

struct fu540_otp_regs {
	u32 pa;     /* Address input */
	u32 paio;   /* Program address input */
	u32 pas;    /* Program redundancy cell selection input */
	u32 pce;    /* OTP Macro enable input */
	u32 pclk;   /* Clock input */
	u32 pdin;   /* Write data input */
	u32 pdout;  /* Read data output */
	u32 pdstb;  /* Deep standby mode enable input (active low) */
	u32 pprog;  /* Program mode enable input */
	u32 ptc;    /* Test column enable input */
	u32 ptm;    /* Test mode enable input */
	u32 ptm_rep;/* Repair function test mode enable input */
	u32 ptr;    /* Test row enable input */
	u32 ptrim;  /* Repair function enable input */
	u32 pwe;    /* Write enable input (defines program cycle) */
} __packed;

#define BYTES_PER_FUSE				4
#define NUM_FUSES				0x1000

static int fu540_otp_read(int offset, void *buf, int size)
{
	struct fu540_otp_regs *regs = (void __iomem *)FU540_OTP_BASE_ADDR;
	unsigned int i;
	int fuseidx = offset / BYTES_PER_FUSE;
	int fusecount = size / BYTES_PER_FUSE;
	u32 fusebuf[fusecount];

	/* check bounds */
	if (offset < 0 || size < 0)
		return -EINVAL;
	if (fuseidx >= NUM_FUSES)
		return -EINVAL;
	if ((fuseidx + fusecount) > NUM_FUSES)
		return -EINVAL;

	/* init OTP */
	writel(0x01, &regs->pdstb); /* wake up from stand-by */
	writel(0x01, &regs->ptrim); /* enable repair function */
	writel(0x01, &regs->pce);   /* enable input */

	/* read all requested fuses */
	for (i = 0; i < fusecount; i++, fuseidx++) {
		writel(fuseidx, &regs->pa);

		/* cycle clock to read */
		writel(0x01, &regs->pclk);
		mdelay(1);
		writel(0x00, &regs->pclk);
		mdelay(1);

		/* read the value */
		fusebuf[i] = readl(&regs->pdout);
	}

	/* shut down */
	writel(0, &regs->pce);
	writel(0, &regs->ptrim);
	writel(0, &regs->pdstb);

	/* copy out */
	memcpy(buf, fusebuf, size);

	return 0;
}

static u32 fu540_read_serialnum(void)
{
	int ret;
	u32 serial[2] = {0};

	for (int i = 0xfe * 4; i > 0; i -= 8) {
		ret = fu540_otp_read(i, serial, sizeof(serial));
		if (ret) {
			printf("%s: error reading from OTP\n", __func__);
			break;
		}
		if (serial[0] == ~serial[1]){
			printf("Board serial #%d\n", serial[0]);
			return serial[0];
		} else {
			printf("%s: error, serial number corrupted in otp, 0x%x	0x%x\n",
				serial[0], ~serial[1]);
		}
	}

	return 0;
}

static void fu540_setup_macaddr(u32 serialnum)
{
	/* Default MAC address */
	unsigned char mac[6] = { 0x70, 0xb3, 0xd5, 0x92, 0xf0, 0x00 };

	/*
	 * We derive our board MAC address by ORing last three bytes
	 * of board serial number to above default MAC address.
	 *
	 * This logic of deriving board MAC address is taken from
	 * SiFive FSBL and is kept unchanged.
	 */
	mac[5] |= (serialnum >>  0) & 0xff;
	mac[4] |= (serialnum >>  8) & 0xff;
	mac[3] |= (serialnum >> 16) & 0xff;

	/* Update environment variable */
	eth_env_set_enetaddr("ethaddr", mac);
}

int misc_init_r(void)
{

	/* Set ethaddr environment variable if not set */
	if (!env_get("ethaddr"))
		fu540_setup_macaddr(fu540_read_serialnum());

	return 0;
}

#endif

int board_init(void)
{
	/* For now nothing to do here. */

	return 0;
}

void reset_phy(void)
{
    volatile uint32_t loop;

    printf("  enter reset_phy..");
/*
 * Init includes toggling the reset line which is connected to GPIO 0 pin 12.
 * This is the only pin I can see on the 16 GPIO which is currently set as an.
 * output. We will hard code the setup here to avoid having to have a GPIO
 * driver as well...
 *
 * The Aloe board is strapped for unmanaged mode and needs two pulses of the
 * reset line to configure the device properly.
 *
 * The RX_CLK, TX_CLK and RXD7 pins are strapped high and the remainder low.
 * This selects GMII mode with auto 10/100/1000 and 125MHz clkout.
 */
    g_aloe_gpio->OUTPUT_EN  |= 0x00001000ul;  /* Configure pin 12 as an output */
    g_aloe_gpio->OUTPUT_VAL &= 0x0000EFFFul;  /* Clear pin 12 to reset PHY */
    for(loop = 0; loop != 1000; loop++)     /* Short delay, I'm not sure how much is needed... */
    {
	;
    }
    g_aloe_gpio->OUTPUT_VAL  |= 0x00001000ul; /* Take PHY^ out of reset */
    for(loop = 0; loop != 1000; loop++)     /* Short delay, I'm not sure how much is needed... */
    {
	;
    }
    g_aloe_gpio->OUTPUT_VAL &= 0x0000EFFFul;  /* Second reset pulse */
    for(loop = 0; loop != 1000; loop++)     /* Short delay, I'm not sure how much is needed... */
    {
	;
    }
    g_aloe_gpio->OUTPUT_VAL  |= 0x00001000ul; /* Out of reset once more */

    /* Need at least 15mS delay before accessing PHY after reset... */
    for(loop = 0; loop != 100000; loop++)     /* Long delay, I'm not sure how much is needed... */
    {
	;
    }
    printf(" exit reset_phy\n");
}


#if defined(CONFIG_MACB) && !defined(CONFIG_DM_ETH)
#warning "TODO: remove me"
int board_eth_init(bd_t *bd)
{	
	int rc = 0;
	
	rc = macb_eth_initialize(0, (void *)HIFIVE_BASE_ETHERNET, 0x00);
	
	return rc;
}
#endif
