/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * Copyright (C) 2018 Microsemi Corporation.
 * Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
 * Copyright (C) 2018 SiFive Inc
 * Troy Benjegerdes, <troy.benjegerdes@sifive.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_SDRAM_BASE		0x80000000

///* start from L2 cache sideband */
//#define CONFIG_SYS_TEXT_BASE		0x08000000
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + SZ_1M)

#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + SZ_1M)

#define CONFIG_SYS_MALLOC_LEN		SZ_8M

#define CONFIG_SYS_BOOTM_LEN		SZ_64M

/* load part way up SDram for FIT images */
#define CONFIG_STANDALONE_LOAD_ADDR	0x90000000

/* Environment options */
#define CONFIG_ENV_SIZE			SZ_128K

#define BOOT_TARGET_DEVICES(func) \
	func(DHCP, dhcp, na)


#if 0
/* Someday I will understand config_distro_bootcmd. Today is not that day */
#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_high=0xffffffffffffffff\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"kernel_addr_r=0x84000000\0" \
	"fdt_addr_r=0x88000000\0" \
	"scriptaddr=0x88100000\0" \
	"pxefile_addr_r=0x88200000\0" \
	"ramdisk_addr_r=0x88300000\0" \
	BOOTENV

#define CONFIG_PREBOOT \
	"setenv fdt_addr ${fdtcontroladdr};" \
	"fdt addr ${fdtcontroladdr};"
#else
/* less complicated, works for now */

#define CONFIG_BOOTFILE		"hifiveu.fit"
#define HIFIVE_BASE_GPIO	0x10060000

#define CONFIG_EXTRA_ENV_SETTINGS       \
		"loadaddr=0x88000000\0" \
                "ip_dyn=yes\0" \
                "uboot_version=" __stringify(PLAIN_VERSION) "\0" \
                "mmcsetup=mmc_spi 1 20000000 0; mmc part\0" \
                "fdt_high=0xffffffffffffffff\0" \
                "fdtsetup=fdt addr ${fdtcontroladdr}; fdt chosen;" \
                        "fdt set /firmware sifive,uboot " __stringify(PLAIN_VERSION) ";" \
                        "fdt set /chosen bootargs console=ttySIF0,${baudrate}\0" \
                "fatenv=setenv fileaddr a0000000; fatload mmc 0:1 ${fileaddr} uEnv.txt;" \
                        "env import -t ${fileaddr} ${filesize};\0" \
		"distro_bootcmd=dhcp"

#endif

/* macb needs the phy reset until we have gpio drivers */
#define CONFIG_RESET_PHY_R

/* 1MHz RTC clock, SiFive FU540-C000 Manual, section 7 */
#define CONFIG_SYS_HZ_CLOCK     1000000

#endif /* __CONFIG_H */
