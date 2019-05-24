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

int board_init(void)
{
	/* For now nothing to do here. */

	return 0;
}

void reset_phy(void)
{
    volatile uint32_t loop;

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
    for(loop = 0; loop != 10000; loop++)     /* Long delay, I'm not sure how much is needed... */
    {
	;
    }

}

/* This define is a value used for error/unknown serial. If we really care about distinguishing errors and 0 is valid, we'll need a different one. */
#define ERROR_READING_SERIAL_NUMBER	   0

#ifdef CONFIG_MISC_INIT_R
static u32 setup_serialnum(void);
static void setup_macaddr(u32 serialnum);

int misc_init_r(void)
{	
	if (!env_get("serial#")) {
		u32 serialnum = setup_serialnum();
		setup_macaddr(serialnum);
	}
	return 0;
}
#endif

#if defined(CONFIG_MACB) && !defined(CONFIG_DM_ETH)
int board_eth_init(bd_t *bd)
{	
	int rc = 0;
	
	rc = macb_eth_initialize(0, (void *)HIFIVE_BASE_ETHERNET, 0x00);
	
	return rc;
}
#endif

#if CONFIG_IS_ENABLED(HIFIVE_OTP)
static u32 otp_read_serialnum(struct udevice *dev)
{
	u32 serial[2] = {0};
	int ret;
	for (int i = 0xfe * 4; i > 0; i -= 8) {
		ret = misc_read(dev, i, serial, sizeof(serial));
		if (ret) {
			printf("%s: error reading serial from OTP\n", __func__);
			break;
		}
		if (serial[0] == ~serial[1])
			return serial[0];
	}
	return ERROR_READING_SERIAL_NUMBER;
}
#endif

static u32 setup_serialnum(void)
{
	u32 serial = ERROR_READING_SERIAL_NUMBER;
	char serial_str[6];

#if CONFIG_IS_ENABLED(HIFIVE_OTP)
	struct udevice *dev;
	int ret;

	// init OTP
	ret = uclass_get_device_by_driver(UCLASS_MISC, DM_GET_DRIVER(hifive_otp), &dev);
	if (ret) {
		debug("%s: could not find otp device\n", __func__);
		return serial;
	}

	// read serial from OTP and set env var
	serial = otp_read_serialnum(dev);
	snprintf(serial_str, sizeof(serial_str), "%05"PRIu32, serial);
	env_set("serial#", serial_str);
#endif

	return serial;
}

static void setup_macaddr(u32 serialnum) {
	// OR the serial into the MAC -- see SiFive FSBL
	unsigned char mac[6] = { 0x70, 0xb3, 0xd5, 0x92, 0xf0, 0x00 };
	mac[5] |= (serialnum >>  0) & 0xff;
	mac[4] |= (serialnum >>  8) & 0xff;
	mac[3] |= (serialnum >> 16) & 0xff;
	eth_env_set_enetaddr("ethaddr", mac);
}
