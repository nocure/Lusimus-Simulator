
/*
        debugger.h - necessary definition for skyeye debugger
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
 * 12/04/2005   ksh  <blackfin.kang@gmail.com>
 * */

#ifndef __SKYEYE2GDB_H__
#define __SKYEYE2GDB_H__

#include "gdb_tracepoint.h"

//chy 2006-04-12 add ICE Simulation
//chy 2006-04-12 define skyeye_ice struct
//#define MAX_BREAKPOINTS 16
#define MAX_BREAKPOINTS 64	// ww 2009-0921: increased bp to 64
#define MAX_TRACEPOINTS 64	// ww 2009-0923: increased tp to 64
#define MAX_ACTION_LENGTH 80

struct SkyEye_ICE{
	unsigned int bps[MAX_BREAKPOINTS];
	int num_bps;
	trace_status tps_status;		// is tracing enabled or disabled ?
	tracepoint_def tps[MAX_TRACEPOINTS];	// tracepoints array
	int num_tps;
	frame_buffer *fb;			// frames linked list
	int num_fb;
	frame_buffer *selected_fb;
	ro_region *ro_region_head;
};
extern struct SkyEye_ICE skyeye_ice;

struct register_defs{
	char * name;
	int (*register_raw_size)(int x);
	int register_bytes;
	int (*register_byte)(int x);
	int num_regs;
	int max_register_raw_size;
	int endian_flag;
	int pc_regnum;
	int sp_regnum;
	int fp_regnum;
	int (*store_register)(int rn, char * memory);
	int (*fetch_register)(int rn, char * memory);
};
typedef struct register_defs register_defs_t;

int frommem (char *memory);
void tomem (char *memory, int val);

void gdbserver_cont();
void gdbserver_step();

int sim_ice_breakpoint_remove(unsigned int addr);
int sim_ice_breakpoint_insert(unsigned int addr);
int sim_read (unsigned int addr, char *buffer, int size);
int sim_write (unsigned int addr, char *buffer, int size);

// in arch_regdefs.c
void register_reg_type(register_defs_t * reg_type);


#endif
