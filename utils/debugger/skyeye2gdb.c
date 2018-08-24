/*
        skyeye2gdb.c - necessary definition for skyeye debugger
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
 * 12/04/2005   ksh  <blacfin.kang@gmail.com>
 * */

#include "armdefs.h"

#include "skyeye_types.h"
#include "memory.h"
#include "armemu.h"
#include "skyeye2gdb.h"
#include "skyeye_defs.h"

#include <stdint.h>

int remote_interrupt();

extern struct ARMul_State * state;
extern struct _memory_core memory_core;
extern register_defs_t * current_reg_type;
extern generic_arch_t * arch_instance;

int bigendSig;
int stop_simulator = 0;
int debugmode = 0;

int
frommem (char *memory)
{
	bigendSig = current_reg_type->endian_flag;
	if (bigendSig == HIGH) {
		return (memory[0] << 24)
			| (memory[1] << 16) | (memory[2] << 8) | (memory[3] <<
								  0);
	}
	else {
		return (memory[3] << 24)
			| (memory[2] << 16) | (memory[1] << 8) | (memory[0] <<
								  0);
	}
}


void
tomem (char *memory, int val)
{
	bigendSig = current_reg_type->endian_flag;
	if (bigendSig == HIGH) {
		memory[0] = val >> 24;
		memory[1] = val >> 16;
		memory[2] = val >> 8;
		memory[3] = val >> 0;
	}
	else {
		memory[3] = val >> 24;
		memory[2] = val >> 16;
		memory[1] = val >> 8;
		memory[0] = val >> 0;
	}
}

#if 0
ARMword
ARMul_Debug (ARMul_State * state, ARMword pc ATTRIBUTE_UNUSED,
	     ARMword instr ATTRIBUTE_UNUSED)
{
	state->Emulate = STOP;
	stop_simulator = 1;
	return 1;
}
#endif
//chy 2006-04-12
int sim_ice_breakpoint_insert(unsigned int addr)
{
  int i;
  for(i = 0; i < skyeye_ice.num_bps; i++) {
        if (skyeye_ice.bps[i] == addr)
	        return 1;
	}
  if (skyeye_ice.num_bps >= MAX_BREAKPOINTS)
	 return -1;
  skyeye_ice.bps[skyeye_ice.num_bps++] = addr;
	       
  return 0;
}

int sim_ice_breakpoint_remove(unsigned int addr)
{
  int i;
  for(i = 0; i < skyeye_ice.num_bps; i++) {
        if (skyeye_ice.bps[i] == addr)
	       goto found;
  }
  return 0;
found:
  skyeye_ice.num_bps--;
  if (i < skyeye_ice.num_bps)
          skyeye_ice.bps[i] = skyeye_ice.bps[skyeye_ice.num_bps];
  return 1;
}

int
sim_write (unsigned int addr, char *buffer, int size)
{
	int i;
	int fault=0;
	for (i = 0; i < size; i++) {
		fault=arch_instance->ICE_write_byte(addr + i, buffer[i]); 
		if(fault) return -1; 
	}
	return size;
}

int
sim_read (unsigned int addr, char *buffer, int size)
{
	int i;
	int fault = 0;
	unsigned char v;
	for (i = 0; i < size; i++) {
		fault = arch_instance->ICE_read_byte(addr + i, &v);
		if(fault) 
			return -1; 
		buffer[i]=v;
	}
	return size;
}

/**
 *  step run for the simulator
 */
void
sim_resume (int step)
{
	/* workaround here: we have different run mode on arm */
	if(!strcmp(skyeye_config.arch->arch_name, "arm")){
		state->EndCondition = 0;
		stop_simulator = 0;

		if (step) {
			state->Reg[15] = ARMul_DoInstr (state);

			if (state->EndCondition == 0) {
				//chy 20050729 ????
				printf ("error in sim_resume for state->EndCondition");
				skyeye_exit (-1);
			}
		}
		else {
			state->NextInstr = RESUME;	/* treat as PC change */
			state->Reg[15] = ARMul_DoProg (state);
		}
		FLUSHPIPE;
	}
	/* other target simulator step run */
	else {
		do {
			/* if we are in debugmode we will check ctrl+c and breakpoint */
			if(debugmode){
				int i;
				WORD addr;

				/* to detect if Ctrl+c is pressed.. */
				if(remote_interrupt())
					return;
				addr = arch_instance->get_pc();
				for (i = 0;i < skyeye_ice.num_bps;i++){
					if(skyeye_ice.bps[i] == addr)
            					return;
				} /* for */
                                if (skyeye_ice.tps_status==TRACE_STARTED)
                                {
                                    for (i=0;i<skyeye_ice.num_tps;i++)
                                    {
                                        if (((skyeye_ice.tps[i].tp_address==addr)&&             (skyeye_ice.tps[i].status==TRACEPOINT_ENABLED))||(skyeye_ice.tps[i].status==TRACEPOINT_STEPPING))
                                        {
                                            handle_tracepoint(i);
                                        }
                               	    }
                                }
			} /* if(debugmode) */
			if (skyeye_config.log.logon >= 1) {
				WORD pc = arch_instance->get_pc();
				if (pc >= skyeye_config.log.start &&
					    pc <= skyeye_config.log.end) {
#if defined(HAVE_LIBBFD) && !defined(__MINGW32__)
					char * func_name = get_sym(pc);
					if(func_name)
						fprintf (skyeye_logfd,"\n in %s\n", func_name);
#endif
				/*
					if (skyeye_config.log.logon >= 2)
						fprintf (skyeye_logfd,
                                                         "pc=0x%x", pc);

						SKYEYE_OUTREGS (skyeye_logfd);
					if (skyeye_config.log.logon >= 3)
						SKYEYE_OUTMOREREGS
							(skyeye_logfd);
				*/
				}
			}/* if (skyeye_config.log.logon >= 1) */

			arch_instance->step_once ();
		}while(!step);
	}
}

void gdbserver_cont(){
         if(!strcmp(skyeye_config.arch->arch_name,"arm")){
                //chy 2006-04-12
                state->NextInstr = RESUME;      /* treat as PC change */
                state->Reg[15]=ARMul_DoProg (state);
        }
	else
		sim_resume(0);
}
void gdbserver_step(){
         if(!strcmp(skyeye_config.arch->arch_name,"arm")){
                //chy 2006004-12
                state->NextInstr = RESUME;
                state->Reg[15]=ARMul_DoInstr (state);
        }
	else
		sim_resume(1);
}
