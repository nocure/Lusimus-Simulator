#ifndef __LEON2_IO_H__
#define __LEON2_IO_H__

#ifndef __SPARC_TYPES_H_
#error "sparc_types.h header file must be included before including mach_leon2_regmap.h"
#endif

#if 0
#define leon2_declare_device(name) \
void name##_write(uint32 size, uint32 addr, uint32 v); \
uint32 name##_read(uint32 size, uint32 addr);    \
void name##_cycle(void *arg);   \
void name##_init(void *state)
#endif

#define TIMER1_ADDR     0x80000040
#define TIMER_IRQ       8

#define UART1_ADDR      0x80000070
#define UART1_IRQ        3

#define MCFG_ADDR       0x80000000
#define CFG_ADDR        0x80000024
#define IRQCTRL_ADDR    0x80000090

/*-----------------------------------------------------------------------------
 *  TIMERS REGISTERS
 *-----------------------------------------------------------------------------*/

typedef  uint32 (*io_read_callback)(void *opaque, uint32 addr);
typedef  uint32 (*io_write_callback)(void *opaque, uint32 addr, uint32 val);

int leon2_register_io_memory (
        uint32 address,     /*  I/O address */
        uint32 size,        /*  I/O area size (bytes)   */
		io_write_callback fwrite,   /*  read functin callback   */
		io_read_callback fread,  /*  write function callback */
        void *opaque);

void leon2_timer_core_init(void *state, uint32 address, int freq);
void leon2_timer_core_cycle(void *pstate);

void leon2_mcfg_init(uint32 address);
void leon2_cfg_init(uint32 address);

void leon2_uart_init(void *opaque, uint32 address, uint32 freq);
void leon2_uart_cycle(void *pstate);

#endif
