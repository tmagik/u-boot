// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 * Troy Benjegerdes, SiFive Inc <troy.benjegerdes@sifive.com>
 */

#include <common.h>
#include <command.h>
#include <asm/csr.h>

void do_go_elf(ulong hart, ulong fdt, ulong (*entry)(int, void *))
{
	#warning hardcoded fooo!!
	//csr_write(MTVEC, 0xfff7fa20);
	asm volatile ( "li t4, 0xfff7fa20\n\t"
			"csrw mtvec, t4\n\t");
	printf("do_go_smp: hart: %d, fdt 0x%x, entry 0x%x\n", hart, fdt, entry);
	return entry(hart, fdt);
}

unsigned long do_bootelf_exec(ulong (*entry)(int, char * const[]),
	/* TODO go to device tree */
	int argc, char * const fdt[])
{
	printf("do_bootelf_exec( argc: %d, fdt 0x%x)\n", argc, fdt);

        smp_call_function(do_go_elf, fdt, entry);

	/* ugly delay so other harts finish */
                int64_t a, b;
                for (int64_t i=0; i < 100000000; i++){
                        asm volatile("nop");
                }

        //return entry(argc, argv);
        printf("do_go_exec: calling entry(argc: %d, fdt 0x%x)\n", argc, fdt);
        return entry(argc, fdt);

}
