// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 */

#include <common.h>
#include <command.h>
#include <asm/csr.h>


void do_go_smp(ulong hart, ulong fdt, ulong (*entry)(int, void *))
{
	#warning hardcoded fooo!!
	//csr_write(MTVEC, 0xfff7fa20);
	asm volatile ( "li t4, 0xfff7fa20\n\t"
			"csrw mtvec, t4\n\t");
	printf("do_go_smp: hart: %d, fdt 0x%x, entry 0x%x\n", hart, fdt, entry);
	return entry(hart, fdt);
}


unsigned long do_go_exec(ulong (*entry)(int, char * const []),
//			 int argc, char * const argv[])
/*			overload this */
			int argc, char * const fdt[])
{
	printf("do_go_exec( argc: %d, fdt 0x%x)\n", argc, fdt);
	cleanup_before_linux();

#if 0
#define HIFIVE_HART0_MSIP       0x2000000
#define HIFIVE_HART1_MSIP       0x2000004
#define HIFIVE_HART2_MSIP       0x2000008
#define HIFIVE_HART3_MSIP       0x200000C
#define HIFIVE_HART4_MSIP       0x2000010
#define RAISE_SOFT_INT          0x1
#if 1
        *((volatile uint32_t *)(HIFIVE_HART1_MSIP)) = RAISE_SOFT_INT;
        *((volatile uint32_t *)(HIFIVE_HART2_MSIP)) = RAISE_SOFT_INT;
        *((volatile uint32_t *)(HIFIVE_HART3_MSIP)) = RAISE_SOFT_INT;
        *((volatile uint32_t *)(HIFIVE_HART4_MSIP)) = RAISE_SOFT_INT;
#endif

        asm volatile ("li a1, 0xF0000000\n\t"
                        "csrr a0, mhartid\n\t"
          "li t4, 0x80000000\n\t"
          "jr t4\n\t");

#endif
	smp_call_function(do_go_smp, fdt, entry);
	//return entry(argc, argv);
	printf("do_go_exec: calling entry(hart: %d, fdt 0x%x)\n", argc, fdt);
	return entry(argc, fdt);
	//return;
}
