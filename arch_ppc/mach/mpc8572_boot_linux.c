/* 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/*
 * 30/10/2007   Michael.Kang  <blackfin.kang@gmail.com>
 */

/*
 * A simple boot function for linux happy
 */

#include <stdio.h>
#include <string.h>

#include "skyeye.h"
#include "ppc_cpu.h"
#include "ppc_boot.h"
#include "ppc_mmu.h"
#include "sysendian.h"

extern byte * ddr_ram; /* 64M DDR SDRAM */

static const int initrd_start = 32 * 1024 * 1024, initrd_size = 1 * 1024 * 1024;
static const char * initrd_filename = "initrd.img";
static const int bd_start = 8 * 1024 * 1024;
//static const int boot_param_start = 8 * 1024 * 1024;
static const int bootcmd_start = 9 * 1024 * 1024;
//static char * bootcmd = "root=/dev/ram0 console=ttyCPM0 mem=64M";
static char * bootcmd = "root=/dev/ram0 console=ttyS0 mem=64M";

//const int OFF_DT_STRUCT = 0x200000;
const int DT_STRUCT_SIZE = 8 * 1024;
const char * dtb_filename = "mpc8572ds.dtb";

static void load_initrd() {

	FILE *f;

	if ((f = fopen(initrd_filename, "rb"))) {
//		void * t= &ddr_ram[initrd_start];
		if (fread(&ddr_ram[initrd_start], 1, initrd_size, f))
			printf("Load %s to 0x%x...\n", initrd_filename, initrd_start);
		else
			printf("Can not load %s to 0x%x\n", initrd_filename, initrd_start);
		fclose(f);
	}
	else {
		fprintf(stderr, "Can not open initrd file %s.\n", initrd_filename);
		skyeye_exit(-1);
	}

}

struct boot_param_header {
	uint32_t magic; /* magic word OF_DT_HEADER */
	uint32_t totalsize; /* total size of DT block */
	uint32_t off_dt_struct; /* offset to structure */
	uint32_t off_dt_strings; /* offset to strings */
	uint32_t off_mem_rsvmap; /* offset to memory reserve map*/
	uint32_t version; /* format version */
	uint32_t last_comp_version; /* last compatible version */
	/* version 2 fields below */
	uint32_t boot_cpuid_phys; /* Physical CPU id we're booting on */
	/* version 3 fields below */
	uint32_t dt_strings_size; /* size of the DT strings block*/
};

/**
 * 
 */
static void setup_boot_param() {
//	struct boot_param_header * initial_boot_param = &ddr_ram[bd_start];
	//initial_boot_param->off_dt_struct = ppc_word_from_BE(OFF_DT_STRUCT);
	FILE *f;
	if ((f = fopen(dtb_filename, "rb"))) {
		//void * t = &ddr_ram[OFF_DT_STRUCT];
		if (fread(&ddr_ram[bd_start], 1, DT_STRUCT_SIZE, f))
			printf("Load %s to 0x%x...\n", dtb_filename, bd_start);
		else
			printf("Can not load %s to 0x%x\n", dtb_filename, bd_start);
		fclose(f);
	}
	else {
		fprintf(stderr, "Can not open dtb file%s.\n", dtb_filename);
		skyeye_exit(-1);
	}
	memcpy(&ddr_ram[bootcmd_start], bootcmd, (strlen(bootcmd) + 1));
}

#if 0
static void set_bootcmd() {
	bd_t * t = (bd_t*)&ddr_ram[bd_start];
	t->bi_immr_base = ppc_word_to_BE(0xe0000000);
	t->bi_busfreq = ppc_word_to_BE(100 * 1024 * 1024);
	t->bi_intfreq = ppc_word_to_BE(500 * 1024 * 1024);
	t->bi_baudrate = ppc_word_to_BE(9600);
	t->bi_memsize = ppc_word_to_BE(64 * 1024 * 1024);

	memcpy(&ddr_ram[bootcmd_start], bootcmd, (strlen(bootcmd) + 1));
}
#endif

static void set_boot_param(e500_core_t * core) {
	core->gpr[3] = bd_start;

	/*
	 *   r4 - Starting address of the init RAM disk
	 *   r5 - Ending address of the init RAM disk
	 */

	core->gpr[4] = initrd_start;
	core->gpr[5] = initrd_start + initrd_size;

	core->gpr[6] = bootcmd_start;
	core->gpr[7] = bootcmd_start + strlen(bootcmd) + 1;
}

static void setup_boot_map(e500_mmu_t * mmu) {
	/* setup initial tlb map for linux, that should be done by bootloader */
	ppc_tlb_entry_t * entry = &mmu->l2_tlb1_vsp[0];
	entry->v = 1; /* entry is valid */
	entry->ts = 0; /* address space 0 */
	entry->tid = 0; /* TID value for shared(global) page */
	entry->epn = 0xC0000; /* Virtual address of DDR ram in address space*/
	entry->rpn = 0x0; /* Physical address of DDR ram in address space*/
	entry->size = 0x7; /* 16M byte page size */
	/* usxrw should be initialized to 010101 */
	entry->usxrw |= 0x15; /* Full supervisor mode access allowed */
	entry->usxrw &= 0x15; /* No user mode access allowed */
	entry->wimge = 0x8; /* Caching-inhibited, non-coherent,big-endian*/
	entry->x = 0; /* Reserved system attributes */
	entry->u = 0; /* User attribute bits */
	entry->iprot = 1; /* Page is protected from invalidation */
}

void mpc8572_boot_linux() {
	int i;
	load_initrd();
	//set_bootcmd(); /* MPC8560 */
	setup_boot_param(); /* MPC8572 */
	for (i = 0; i < gCPU.core_num; i++) {
		e500_core_t * core = &gCPU.core[i];
		/* FIXME, will move it to skyeye.conf */
		set_boot_param(core);
		/* just for linux boot, so we need to do some map */
		setup_boot_map(&core->mmu);
	}
	//gCPU.ccsr.ccsr = 0xE0000; /* Only for boot linux MPC8560 */
	gCPU.ccsr = 0xFFE00; /* Only for boot MPC8572 linux */
}
