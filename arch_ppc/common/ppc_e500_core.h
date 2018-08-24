/*
        ppc_e500_core.h - difinition for powerpc e500 core
        Copyright (C) 2003 Skyeye Develop Group
        for help please send mail to <skyeye-developer@lists.sf.linuxforum.net>

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
/*
 * 07/21/2008   Michael.Kang  <blackfin.kang@gmail.com>
 */

#ifndef __PPC_E500_CORE_H__
#define __PPC_E500_CORE_H__
#include <types.h>
/* tlb entry */
typedef struct ppc_tlb_entry_s{
	uint v;
	uint ts;
	uint tid;
	uint epn;
	uint rpn;
	uint size;
	uint usxrw;
	uint wimge;
	uint x;
	uint u;
	uint iprot;	
}ppc_tlb_entry_t;


#define L2_TLB0_SIZE 256
#define L2_TLB1_SIZE 16

typedef struct e500_mmu_s{
	uint64 pid[3];	
	uint64 mmucsr0;
	uint64 mmucfg;
	uint64 tlbcfg[2];
	uint64 mas[8];
	uint32 tlb0_nv;

	ppc_tlb_entry_t l2_tlb0_4k[L2_TLB0_SIZE]; /* unified, filled by tlbwe instruction */
	ppc_tlb_entry_t l2_tlb1_vsp[L2_TLB1_SIZE]; /* filled by tlbwe insructions */

}e500_mmu_t;

#define TLBSEL(e) ((e >> 28) & 0x1)
#define ESEL(e) ((e >> 16) & 0xf)
#define EPN(e) (e >> 12)
#define TLBSELD(e) ((e & 0x10000000) >> 28)

typedef union Vector_t {
	uint64 d[2];
	sint64 sd[2];
	float f[4];
	uint32 w[4];
	sint32 sw[4];
	uint16 h[8];
	sint16 sh[8];
	uint8 b[16];
	sint8 sb[16];
} Vector_t;

/**
 * description for e500 core, refer to E500CORERM manual
 */
typedef struct e500_core_s{
	// * uisa
	uint32 gpr[32];
	uint64 fpr[32];
	uint32 cr;
	uint32 fpscr;
	uint32 xer;	// spr 1
	uint32 xer_ca;  // carry from xer
	uint32 lr;	// spr 8
	uint32 ctr;	// spr 9
	// * oea
	uint32 msr;
	uint32 pvr;	// spr 287

	//    * memory managment
	uint32 ibatu[4];	// spr 528, 530, 532, 534
	uint32 ibatl[4];	// spr 529, 531, 533, 535
	uint32 ibat_bl17[4];	// for internal use

	uint32 dbatu[4];	// spr 536, 538, 540, 542
	uint32 dbatl[4];	// spr 537, 539, 541, 543
	uint32 dbat_bl17[4];	// for internal use

	uint32 sdr1;	// spr 25       (page table base address)

	uint32 sr[16];
	
	//    * exception handling
	uint32 dar;	// spr 19
	uint32 dsisr;	// spr 18
	uint32 sprg[8]; // spr 272-275
	//uint32 sprg[4];
	uint32 srr[2];	// spr 26-27
	
	//    * misc
	uint32 dec;	// spr 22
	uint32 ear;	// spr 282 .101
	uint32 pir;	// spr 1032
	uint64 tb;	// .75 spr 284(l)/285(u)
	
	uint32 hid[16];
	// * internal
	
	uint32 pc;
	uint32 npc;
	uint32 current_opc;
	bool   exception_pending;
	bool   dec_exception;
	bool   ext_exception;
	bool   stop_exception;
	bool   singlestep_ignore;

	uint32 pagetable_base;
	int    pagetable_hashmask;
	uint32 reserve;
	bool   have_reservation;
	
	// for generic cpu core
	uint32 effective_code_page;
	byte  *physical_code_page;
	uint64 pdec;	// more precise version of dec
	uint64 ptb;	// more precise version of tb

	// for altivec
	uint32 vscr;
	uint32 vrsave;	// spr 256
	Vector_t vr[36];		// <--- this MUST be 16-byte alligned
	uint32 vtemp;
	/* e500 specific register */
	uint32 l1csr[2]; /* L1 cache constrol and status */
	uint32 csrr[2]; /* Critical save/restore register */
	uint32 mcsrr[2]; /* Machine check save/restore register */
	uint32 esr; /* Exception syndrome register */
	uint32 mcsr; /* Machine check syndrome register */
	uint32 dear; /* Data exception address register */
	uint32 dbcr[3]; /* Debug control register */
	uint32 dbsr; /* Debug status register */ 
	uint32 tcr; /* Timer control register */
	uint32 tsr; /* Timer status register */
	uint32 dac[2]; /* Data address compare */
	uint32 ivpr; /* Interrupt vector */
	uint32 ivor[16]; /* 0 = Critical input */
	uint32 iac[2]; /* Instruction address compare */

	uint32 tbl;
	uint32 tbu;

	uint32 spefscr;
	e500_mmu_t mmu;

	uint32 ipr;
	uint32 iack;

	uint32 ipi_flag;
}e500_core_t;

#define E500

#define IPI0 (1 >> 0)

void ppc_core_init(e500_core_t * core, int core_id);
void dec_io_do_cycle(e500_core_t * core);


#endif
