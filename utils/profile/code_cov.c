/*
        cov_prof.c - used to record the WRX action for memory address
        Copyright (C) 2008 Skyeye Develop Group
        for help please send mail to <skyeye-developer@lists.gro.clinux.org>

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
 * 03/08/2008   Michael.Kang  <blackfin.kang@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "skyeye.h"
#include "skyeye_types.h"
/**
 *  pointer of memory allocated for code coverage
 */
static uint8_t * prof_mem;
static int prof_start;
static int prof_end;
/* The header format of code coverage data file */
#define MAX_DESC_STR 32
typedef struct prof_header_s{
	/* the version of header file */
	int ver;
	/* The length of header */
	int header_length;
	int prof_start;
	int prof_end;
	/* The description info for profiling file */
	char desc[MAX_DESC_STR]; 
}prof_header_t;
static void header_init(prof_header_t* header){
	const char* desc = "raw data of code coverage";
	header->ver = 0x1;
	header->header_length = (sizeof(prof_header_t));
	header->prof_start = prof_start;
	header->prof_end = prof_end;
	if(sizeof(desc) <= MAX_DESC_STR)
		strcpy(header->desc, desc);
}
void cov_init(int start_addr, int end_addr){
	int prof_size = end_addr - start_addr;
	prof_start = start_addr;
	prof_end = end_addr;
	/* we use four bits to record the WRX action for a 32 bit word */
	int mem_alloc = sizeof(prof_header_t) + (prof_size / 4) / 2 ;
	prof_mem = malloc(mem_alloc);
	prof_header_t * header = (prof_header_t *)prof_mem;
	header_init(header);
	if(!prof_mem)
		SKYEYE_ERR("Can not alloc memory for code coverage, profiling is disabled.\n");
	else
		SKYEYE_INFO("Begin do code coverage between 0x%x and 0x%x .\n", prof_start, prof_end);
}
/**
 * flags: 4 means read, 2 means write, 1 means eXecute
 *
 */
void cov_prof(int flags,WORD addr){
	if(addr < prof_start || addr >= prof_end)
		return;	
	int offset = (addr - prof_start) / 8 + sizeof(prof_header_t);
	uint8_t * prof_addr  = &prof_mem[offset] ;
	*prof_addr |= flags << ((addr - prof_start) % 8);
	//SKYEYE_INFO("addr=0x%x, flags=0x%x, offset=0x%x,(addr - prof_start)%8=0x%x\n", addr, flags, offset, (addr - prof_start)%8);
	//SKYEYE_INFO("prof_addr=0x%x, *prof_addr=0x%x\n", (unsigned long)prof_addr, *prof_addr);
	return;
}
#if 0
/* for debug purpose of profiling */
static void dump_prof_mem(){
	int i = 0;
	for(;i < 8; i++)
		SKYEYE_INFO("prof_mem[%d] = 0x%x\n", i , prof_mem[i]);
}
#endif
/**
 * deinitialization function
 */
void cov_fini(char * filename){
	FILE * fp = fopen(filename, "w+");
	if(!fp){
		SKYEYE_ERR("Warning: can not open file %s for code coverage\n", filename);
		return;
	}
	//dump_prof_mem();
	//int count = fwrite(prof_mem, (prof_end - prof_start), 1, fp);
	/* one byte can recode the state of two 32 bit words */
	size_t to_write = (prof_end - prof_start) / 8 + sizeof(prof_header_t);
	size_t count = fwrite(prof_mem, 1, to_write, fp);
	if(count < to_write)
		SKYEYE_INFO("Write %d bytes for code coverage .\n", count);	
	fclose(fp);
	if(prof_mem)
		free(prof_mem);
	return;
}

