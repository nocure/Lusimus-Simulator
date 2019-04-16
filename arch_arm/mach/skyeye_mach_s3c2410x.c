/*
	skyeye_mach_s3c2410x.c - define machine S3C2410X for skyeye
	Copyright (C) 2005 Skyeye Develop Group
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

#include "armdefs.h"
#include "s3c2410x.h"
//zzc:2005-1-1
#ifdef __CYGWIN__
//chy 2005-07-28
#include <time.h>
//teawater add DBCT_TEST_SPEED 2005.10.04---------------------------------------
/*struct timeval
{
  int tv_sec;
  int tv_usec;
};*/
//AJ2D--------------------------------------------------------------------------
#endif

/* 2007-01-18 added by Anthony Lee : for new uart device frame */
#include "skyeye_uart.h"
typedef struct s3c2410_memctl_s{
	uint32_t bwscon;
	uint32_t bankcon[8];
	uint32_t refresh;
	uint32_t banksize;
	uint32_t mrsrb6;
	uint32_t mrsrb7;
}s3c2410_memctl_t;
typedef struct s3c2410_wd_timer_s{
	uint32_t wtcon;
	uint32_t wtdat;
	uint32_t wtcnt;
}s3c2410_wd_timer_t;
typedef struct s3c2410_rtc_s{
	/* RTC Registers */
	uint32_t rtccon;
	uint32_t rtcalm;
	uint32_t rtcrst;
	uint32_t ticint;
	uint32_t tick_count;
	struct tm rtc_alarm;
	long int rtc_offset;
}s3c2410_rtc_t;
typedef struct s3c2410x_io
{
	u32 srcpnd;		/* Indicate the interrupt request status */
	u32 intmod;		/* Interrupt mode register */
	u32 intmsk;		/* Determine which interrupt source is masked */
	u32 priority;		/* IRQ priority control register */
	u32 intpnd;		/* Indicate the interrupt request status */
	u32 intoffset;		/* Indicate the IRQ interrupt request source */
	u32 subsrcpnd;		/* Indicate the interrupt request status */
	u32 intsubmsk;		/* Determin which interrupt source is masked */

	u32 eintmask;		/* Interrupt pending register for 20 external interrupts (EINT[23:4]) */
	u32 eintpend;		/* Interrupt mask register for 20 external interrupts (EINT[23:4]). */
	struct s3c2410x_timer_io timer;	/* Timer */
	struct s3c2410x_uart_io uart[3]; /* uart */
	struct s3c2410x_clkpower clkpower;	/* clock and power management */
	s3c2410_memctl_t memctl;
	s3c2410_wd_timer_t wd_timer;
	s3c2410_rtc_t rtc;
	uint32_t gpio_ctl[GPIO_CTL_SIZE]; /* GPIO control register */

} s3c2410x_io_t;
static s3c2410x_io_t s3c2410x_io;
#define io s3c2410x_io

static inline void
s3c2410x_set_subsrcint (unsigned int irq)
{
	io.subsrcpnd |= irq;
}
static inline void
s3c2410x_update_subsrcint ()
{
	u32 requests;
	s3c2410x_set_subsrcint (UART_INT_TXD << (0 * 3));
	s3c2410x_set_subsrcint (UART_INT_TXD << (1 * 3));
	s3c2410x_set_subsrcint (UART_INT_TXD << (2 * 3));
	requests = ((io.subsrcpnd & (~io.intsubmsk)) & 0x7fff);
	if (requests & 0x7)
		io.srcpnd |= INT_UART0;
	if (requests & 0x38)
		io.srcpnd |= INT_UART1;
	if (requests & 0x1c0)
		io.srcpnd |= INT_UART2;
	if (requests & 0x600)
		io.srcpnd |= INT_ADC;
}

static inline void
s3c2410x_update_extint ()
{
	u32 requests = ((io.eintpend & (~io.eintmask)));
	if (requests & 0xF0)
		io.srcpnd |= INT_EINT4_7;
	if (requests & 0xFFFF00)
		io.srcpnd |= INT_EINT8_23;

}

static void
s3c2410x_update_int (void * state)
{
	ARMword requests;
	s3c2410x_update_subsrcint ();
	s3c2410x_update_extint ();
	requests = io.srcpnd & (~io.intmsk & INT_MASK_INIT);
	((ARMul_State*)state)->NfiqSig = (requests & io.intmod) ? LOW : HIGH;
	((ARMul_State*)state)->NirqSig = (requests & ~io.intmod) ? LOW : HIGH;
}

static void
s3c2410x_set_ext_intr (u32 interrupt)
{
	io.eintpend |= (1 << interrupt);
}

static int
s3c2410x_pending_ext_intr (u32 interrupt)
{
	return ((io.eintpend & (1 << interrupt)));
}

static void
s3c2410x_update_intr (void *mach)
{
	struct machine_config *mc = (struct machine_config *) mach;
	ARMul_State *state = (ARMul_State *) mc->state;
	s3c2410x_update_int (state);
}

static void
s3c2410x_io_reset (void * state)
{
	int i;

	memset (&s3c2410x_io, 0, sizeof (s3c2410x_io));

	for (i = 0; i < 3; i++) {
		io.uart[i].ulcon = UART_ULCON_INIT;
		io.uart[i].utrstat = UART_UTRSTAT_INIT;
	}

	//io.timer.tcnt[4] = 25350;

	io.clkpower.locktime = 0x00FFFFFF;
	//io.clkpower.mpllcon = 0x00070022; /* That is a value mizi required */
	io.clkpower.mpllcon = 0x0002c080; /* workaround for linux-2.6.10 by ksh */
	io.clkpower.upllcon = 0x00028080;
	io.clkpower.clkcon = 0x7FFF0;
	io.clkpower.clkslow = 0x4;
	io.intmsk = INT_MASK_INIT;
	io.intpnd = 0x0;

	io.eintmask = 0x00FFFFF0;

	/* ARM920T uses LOW */
	((ARMul_State*)state)->lateabtSig = LOW;

	((ARMul_State*)state)->Reg[1] = 193;	//for SMDK2410
	//state->Reg[1] = 395;  //for SMDK2410TK
	//state->Reg[1] = 241;    //ARCH_S3C2440

}


/* s3c2410x io_do_cycle */
static void
s3c2410x_io_do_cycle (void * state)
{
	int i;

	if ((io.timer.tcon & 0x100000) != 0) {

		io.timer.tcnt[4]--;
		if (io.timer.tcnt[4] < 0) {
			io.timer.tcnt[4] = io.timer.tcntb[4];
			/*timer 4 hasn't tcmp */
			//io.timer.tcmp[4] = io.timer.tcmpb[4];
//			io.timer.tcnto[4] = io.timer.tcntb[4];
			io.srcpnd |= INT_TIMER4;
//			s3c2410x_update_int (state);
//			return;
		}
	}

	for (i = 0; i < 3; i++) {
		if (((io.uart[i].utrstat & 0x1) == 0x0) && ((io.uart[i].ucon & 0x3) == 0x1)) {
			struct timeval tv;
			unsigned char buf;

			tv.tv_sec = 0;
			tv.tv_usec = 0;

			if (skyeye_uart_read(i, &buf, 1, &tv, NULL) > 0) {
				/* convert ctrl+c to ctrl+a. */
				if (buf == 1) buf = 3;
				io.uart[i].urxh = buf;
				/* Receiver Ready
				 * */
				io.uart[i].utrstat |= (0x1);
				io.uart[i].ufstat |= (0x1); /* 2007-02-09 by Anthony Lee : for 1 bytes */
				/* pending usart0 interrupt
				 * */
				s3c2410x_set_subsrcint (UART_INT_RXD << (i * 3));
				//io.srcpnd |= INT_UART0;
//				s3c2410x_update_int (state);
			}
		}
	}
	s3c2410x_update_int (state);
}


static void
s3c2410x_uart_read (u32 offset, u32 * data, int index)
{
	switch (offset) {
	case ULCON:
		*data = io.uart[index].ulcon;
		break;
	case UCON:
		*data = io.uart[index].ucon;
		break;
	case UFCON:
		*data = io.uart[index].ufcon;
		break;
	case UMCON:
		*data = io.uart[index].umcon;
		break;
	case UTRSTAT:
		*data = io.uart[index].utrstat;
		break;
	case UERSTAT:
		*data = io.uart[index].uerstat;
		break;
	case UFSTAT:
		*data = io.uart[index].ufstat;
		break;
	case UMSTAT:
		*data = io.uart[index].umstat;
		break;
	case URXH:
		/* receive char
		 * */
		*data = io.uart[index].urxh;
		io.uart[index].utrstat &= (~0x1);	/* clear strstat register bit[0] */
		io.uart[index].ufstat &= ~(0x1); /* 2007-02-09 by Anthony Lee : for 0 bytes */
		break;
	case UBRDIV:
		*data = io.uart[index].ubrdiv;
		break;
	default:
		break;
	}
	SKYEYE_DBG ("%s(UART%d: 0x%x, 0x%x)\n", __FUNCTION__, index, offset, data);
}

static void
s3c2410x_uart_write (ARMul_State * state, u32 offset, u32 data, int index)
{

	SKYEYE_DBG ("%s(UART%d: 0x%x, 0x%x)\n", __FUNCTION__, index, offset, data);
	switch (offset) {
	case ULCON:
		io.uart[index].ulcon = data;
		break;
	case UCON:
		io.uart[index].ucon = data;
		break;
	case UFCON:
		io.uart[index].ufcon = data;
		break;
	case UMCON:
		io.uart[index].umcon = data;
		break;
	case UTRSTAT:
		io.uart[index].utrstat = data;
		break;
	case UERSTAT:
		io.uart[index].uerstat = data;
		break;
	case UFSTAT:
		io.uart[index].ufstat = data;
		break;
	case UMSTAT:
		io.uart[index].umstat = data;
		break;
	case UTXH:
		{
			char c = data;

			/* 2007-01-18 modified by Anthony Lee : for new uart device frame */
			skyeye_uart_write(index, &c, 1, NULL);

			io.uart[index].utrstat |= 0x6;	//set strstat register bit[0]
			if ((io.uart[index].ucon & 0xc) == 0x4) {
				s3c2410x_set_subsrcint (UART_INT_TXD << (index * 3));
				extern ARMul_State * state;
				s3c2410x_update_int (state);
			}
		}
		break;
	case UBRDIV:
		io.uart[index].ubrdiv = data;
		break;
	default:
		break;
	}
}

static void
s3c2410x_timer_read (u32 offset, u32 * data)
{
	switch (offset) {
	case TCFG0:
		*data = io.timer.tcfg0;
		break;
	case TCFG1:
		*data = io.timer.tcfg1;
		break;
	case TCON:
		*data = io.timer.tcon;
		break;
	case TCNTB0:
	case TCNTB1:
	case TCNTB2:
	case TCNTB3:
	case TCNTB4:
		{
			int n = (offset - 0xC) / 0xC;
			*data = io.timer.tcntb[n];
		}
		break;
	case TCMPB0:
	case TCMPB1:
	case TCMPB2:
	case TCMPB3:
		{
			int n = (offset - 0x10) / 0xC;
			*data = io.timer.tcmpb[n];
		}
		break;
	case TCNTO0:
	case TCNTO1:
	case TCNTO2:
	case TCNTO3:
	case TCNTO4:
		{
			int n = (offset - 0x10) / 0xC;
			*data = io.timer.tcnt[n];
		}
		break;
	default:
		break;
	}
}

static void
s3c2410x_timer_write (ARMul_State * state, u32 offset, u32 data)
{
	switch (offset) {
	case TCFG0:
		io.timer.tcfg0 = data;
		break;
	case TCFG1:
		io.timer.tcfg1 = data;
		break;
	case TCON:
		{
			io.timer.tcon = data;
			if (io.timer.tcon) {
			}
		}
		break;
	case TCNTB0:
	case TCNTB1:
	case TCNTB2:
	case TCNTB3:
	case TCNTB4:
		{
			int n = (offset - 0xC) / 0xC;
			io.timer.tcntb[n] = data;
			/* temp data taken from linux source */
//			io.timer.tcntb[n] = 25350 / 20;
		}
		break;
	case TCMPB0:
	case TCMPB1:
	case TCMPB2:
	case TCMPB3:
		{
			int n = (offset - 0x10) / 0xC;
			io.timer.tcmpb[n] = data;
		}
		break;
	default:
		break;
	}
}


#define CYCLE_TIMES_PER_SECOND 1
#define BCD_TO_BIN(a)			(((a) >> 4) * 10 + ((a) & 0xf))
#define BIN_TO_BCD(a)			((((a) / 10) << 4) | ((a) % 10))
static void s3c2410x_rtc_read(ARMword addr, ARMword *data)
{
	struct timeval curr_time;
	struct tm *curr_tm;
	time_t curr_timer;

	if (addr == RTCCON) {
		*data = io.rtc.rtccon;
		goto exit;
	} else if ((io.rtc.rtccon & 0x1) == 0) {
		goto exit;
	}

	switch (addr) {
		case RTCALM:
			*data = io.rtc.rtcalm;
			goto exit;

		case RTCRST:
			*data = io.rtc.rtcrst;
			goto exit;

		case TICNT:
			*data = io.rtc.ticint;
			goto exit;

		case ALMSEC:
			*data = BIN_TO_BCD(io.rtc.rtc_alarm.tm_sec);
			goto exit;

		case ALMMIN:
			*data = BIN_TO_BCD(io.rtc.rtc_alarm.tm_min);
			goto exit;

		case ALMHOUR:
			*data = BIN_TO_BCD(io.rtc.rtc_alarm.tm_hour);
			goto exit;

		case ALMDATE:
			*data = BIN_TO_BCD(io.rtc.rtc_alarm.tm_mday);
			goto exit;

		case ALMMON:
			*data = BIN_TO_BCD(io.rtc.rtc_alarm.tm_mon + 1);
			goto exit;

		case ALMYEAR:
			*data = io.rtc.rtc_alarm.tm_year - 100;
			goto exit;

		default:
			break;
	}

	if (gettimeofday(&curr_time, NULL) != 0 ||
	    ((curr_timer = (time_t)((long int)curr_time.tv_sec + io.rtc.rtc_offset)),
	     (curr_tm = gmtime(&curr_timer))) == NULL) goto exit;

	switch (addr) {
		case BCDSEC:
			*data = BIN_TO_BCD(curr_tm->tm_sec);
			break;

		case BCDMIN:
			*data = BIN_TO_BCD(curr_tm->tm_min);
			break;

		case BCDHOUR:
			*data = BIN_TO_BCD(curr_tm->tm_hour);
			break;

		case BCDDAY:
			*data = BIN_TO_BCD(curr_tm->tm_mday);
			break;

		case BCDDATE:
			*data = (curr_tm->tm_wday == 0 ? 7 : curr_tm->tm_wday);
			break;

		case BCDMON:
			*data = BIN_TO_BCD(curr_tm->tm_mon + 1);
			break;

		case BCDYEAR:
			*data = curr_tm->tm_year - 100;
			break;

		default:
			goto exit;
	}

exit:
SKYEYE_DBG("%s(addr:0x%x, data:0x%x)\n", __FUNCTION__, addr, *data);
	return;
}


static void s3c2410x_rtc_write(ARMul_State *state, ARMword addr, ARMword data)
{
	struct timeval curr_time;
	struct tm *curr_tm;
	time_t curr_timer;

	SKYEYE_DBG("%s(addr:0x%x, data:0x%x)\n", __FUNCTION__, addr, data);

	if (addr == RTCCON) {
		io.rtc.rtccon = data;
		goto exit;
	} else if ((io.rtc.rtccon & 0x1) == 0) {
		goto exit;
	}

	switch (addr) {
		case RTCALM:
			io.rtc.rtcalm = data;
			goto exit;

		case RTCRST:
			io.rtc.rtcrst = data;
			goto exit;

		case TICNT:
			io.rtc.ticint = data;
			io.rtc.tick_count = (data & 0x7f);
			io.rtc.tick_count *= (CYCLE_TIMES_PER_SECOND / 128); /* 1/128 sec per count down */
			goto exit;

		case ALMSEC:
			io.rtc.rtc_alarm.tm_sec = BCD_TO_BIN(data);
			goto exit;

		case ALMMIN:
			io.rtc.rtc_alarm.tm_min = BCD_TO_BIN(data);
			goto exit;

		case ALMHOUR:
			io.rtc.rtc_alarm.tm_hour = BCD_TO_BIN(data);
			goto exit;

		case ALMDATE:
			io.rtc.rtc_alarm.tm_mday = BCD_TO_BIN(data);
			goto exit;

		case ALMMON:
			io.rtc.rtc_alarm.tm_mon = BCD_TO_BIN(data) - 1;
			goto exit;

		case ALMYEAR:
			io.rtc.rtc_alarm.tm_year = data + 100;
			goto exit;

		default:
			break;
	}

	if (gettimeofday(&curr_time, NULL) != 0 ||
	    ((curr_timer = (time_t)((long int)curr_time.tv_sec + io.rtc.rtc_offset)),
	     (curr_tm = gmtime(&curr_timer))) == NULL) goto exit;

	switch (addr) {
		case BCDSEC:
			curr_tm->tm_sec = BCD_TO_BIN(data);
			break;

		case BCDMIN:
			curr_tm->tm_min = BCD_TO_BIN(data);
			break;

		case BCDHOUR:
			curr_tm->tm_hour = BCD_TO_BIN(data);
			break;

		case BCDDAY:
			curr_tm->tm_mday = BCD_TO_BIN(data);
			break;

		case BCDDATE:
			break;

		case BCDMON:
			curr_tm->tm_mon = BCD_TO_BIN(data) - 1;
			break;

		case BCDYEAR:
			curr_tm->tm_mon = data + 100;
			break;

		default:
			goto exit;
	}

	io.rtc.rtc_offset = (long int)mktime(curr_tm) - (long int)curr_time.tv_sec;

exit:
	return;
}



static ARMword
s3c2410x_io_read_word (void * state, ARMword addr)
{
	ARMword data = -1;
//	int i;
	/* uart */
	if ((addr >= UART_CTL_BASE0)
	    && (addr < (UART_CTL_BASE0 + UART_CTL_SIZE))) {
		s3c2410x_uart_read ((u32) ((addr - UART_CTL_BASE0) % 0x4000),
				    (u32 *) & data,
				    (addr - UART_CTL_BASE0) / 0x4000);
		return data;
	}
	if ((addr >= PWM_CTL_BASE) && (addr < (PWM_CTL_BASE + PWM_CTL_SIZE))) {
		s3c2410x_timer_read ((u32) (addr - PWM_CTL_BASE),
				     (u32 *) & data);
		return data;
	}

	/*
	 * 2007-02-09 by Anthony Lee
	 * changed 0xC0 to 0xA4 for running linux-2.6.20,
	 * because GSTATUS1 is 0xB0, the "0xC0" make it like S3C2400
	 */
	if((addr >= GPIO_CTL_BASE) && (addr < (GPIO_CTL_BASE + GPIO_CTL_SIZE))){
		int offset = addr - GPIO_CTL_BASE;
		return io.gpio_ctl[offset];
	}

	/* RTC */
	if (addr >= RTC_CTL_BASE && addr < (RTC_CTL_BASE+RTC_CTL_SIZE)) {
		s3c2410x_rtc_read(addr, &data);
		return data;
	}


	switch (addr) {
	case SRCPND:
		data = io.srcpnd;
		break;
	case INTMOD:
		data = io.intmod;
		break;
	case INTMSK:
		data = io.intmsk;
		break;
	case PRIORITY:
		data = io.priority;
		break;
	case INTPND:
	case INTOFFSET:
		{
			/*find which interrupt is pending */
			int i;
			for (i = 0; i < 32; i++) {
				if (io.srcpnd & (1 << i))
					break;
			}
			if (i < 32) {
				io.intoffset = i;
				io.intpnd = (1 << i);
				if (addr == INTPND)
					data = (1 << i);
				else
					data = i;
			}
			else
				data = 0;

		}
		io.intpnd = (1 << io.intoffset);
		//printf ("io.intoffset:%x, io.intpnd:%x (0x%08x) = 0x%08x\n", io.intoffset, io.intpnd, addr, data);
		break;
	case SUBSRCPND:
		data = io.subsrcpnd;
		break;
	case INTSUBMSK:
		data = io.intsubmsk;
		break;

	case EINTMASK:
		data = io.eintmask;
		break;
	case EINTPEND:
		data = io.eintpend;
		break;
		/* GPIO Register */
	case GSTATUS1:
		data = 0x32410000;
		break;
		/* Clock and Power Management Registers */
	case LOCKTIME:
		data = io.clkpower.locktime;
		break;
	case MPLLCON:
		data = io.clkpower.mpllcon;
		break;
	case UPLLCON:
		data = io.clkpower.upllcon;
		break;
	case CLKCON:
		data = io.clkpower.clkcon;
		break;
	case CLKSLOW:
		data = io.clkpower.clkslow;
		break;
	case CLKDIVN:
		data = io.clkpower.clkdivn;
		break;
	case BWSCON:
		data = io.memctl.bwscon;
		break;
	case BANKCON0:
		data = io.memctl.bankcon[0];
		break;
	case BANKCON1:
		data = io.memctl.bankcon[1];
		break;
	case BANKCON2:
		data = io.memctl.bankcon[2];
		break;
	case BANKCON3:
		data = io.memctl.bankcon[3];
		break;
	case BANKCON4:
		data = io.memctl.bankcon[4];
		break;
	case BANKCON5:
		data = io.memctl.bankcon[5];
		break;
	case BANKCON6:
		data = io.memctl.bankcon[6];
		break;
	case BANKCON7:
		data = io.memctl.bankcon[7];
		break;
	case REFRESH:
		data = io.memctl.refresh;
		break;
	case BANKSIZE:
		data = io.memctl.banksize;
		break;
	case MRSRB6:
		data = io.memctl.mrsrb6;
		break;
	case MRSRB7:
		data = io.memctl.mrsrb7;
		break;
	case WDCON:
		data = io.wd_timer.wtcon;
		break;
	case WDDAT:
		data = io.wd_timer.wtdat;
		break;
	case WDCNT:
		data = io.wd_timer.wtcnt;
		break;

	default:
		//fprintf(stderr, "ERROR: %s(0x%08x) \n", __FUNCTION__, addr);
		break;
	}
	return data;
}

static ARMword
s3c2410x_io_read_byte (void * state, ARMword addr)
{
	// 2018-0606 ww added return statement, there was no value returned
	return s3c2410x_io_read_word (state, addr);
}

static ARMword
s3c2410x_io_read_halfword (void * state, ARMword addr)
{
	// 2018-0606 ww added return statement, there was no value returned
	return s3c2410x_io_read_word (state, addr);
}

static void
s3c2410x_io_write_word (void * state, ARMword addr, ARMword data)
{
	if ((addr >= UART_CTL_BASE0)
	    && (addr < UART_CTL_BASE0 + UART_CTL_SIZE)) {
		s3c2410x_uart_write (state, (addr - UART_CTL_BASE0) % 0x4000,
				     data, (addr - UART_CTL_BASE0) / 0x4000);
		return;
	}
	if ((addr >= PWM_CTL_BASE) && (addr < (PWM_CTL_BASE + PWM_CTL_SIZE))) {
		s3c2410x_timer_write (state, addr - PWM_CTL_BASE, data);
		return;
	}

	/*
	 * 2007-02-09 by Anthony Lee
	 * changed 0xC0 to 0xA4 for running linux-2.6.20,
	 * because GSTATUS1 is 0xB0, the "0xC0" make it like S3C2400
	 */
	if((addr >= GPIO_CTL_BASE) && (addr < (GPIO_CTL_BASE + GPIO_CTL_SIZE))){
                int offset = addr - GPIO_CTL_BASE;
                io.gpio_ctl[offset] = data;
		return;
        }

	/* RTC */
	if (addr >= RTC_CTL_BASE && addr < (RTC_CTL_BASE+RTC_CTL_SIZE)) {
		s3c2410x_rtc_write(state, addr, data);
		return;
	}


	switch (addr) {
	case SRCPND:
		io.srcpnd &= (~data & INT_MASK_INIT);
		//2006-04-04 chy, for eCos on s3c2410. SRCPND will change the INTPND, INTOFFSET, so when write SRCPND, the interrupt should be update
		extern ARMul_State * state;
		s3c2410x_update_int (state);
		break;
	case INTMOD:
		io.intmod = data;
		break;
	case INTMSK:
		io.intmsk = data;
		extern ARMul_State * state;
		s3c2410x_update_int (state);
		break;
	case PRIORITY:
		io.priority = data;
		break;
	case INTPND:
		io.intpnd &= (~data & INT_MASK_INIT);
		io.intoffset = 0;
		//printf ("io.intoffset:%x, io.intpnd:%x (0x%08x) = 0x%08x, pc:%x\n", io.intoffset, io.intpnd, addr, data, state->pc);
		break;
		/*read only */
		//case INTOFFSET:
		//      break;
	case SUBSRCPND:
		io.subsrcpnd &= (~data & INT_SUBMSK_INIT);
		break;
	case INTSUBMSK:
		io.intsubmsk = data;
		break;


		/* ext interrupt */
	case EINTMASK:
		io.eintmask = data;
		break;
	case EINTPEND:
		io.eintpend &= (~data & 0x00FFFFF0);
		break;
	case CLKCON:
                io.clkpower.clkcon = data;
                break;
        case CLKSLOW:
                io.clkpower.clkslow = data;
                break;
        case CLKDIVN:
                io.clkpower.clkdivn = data;
                break;
	case BWSCON:
		io.memctl.bwscon = data;
		break;
	case MPLLCON:
                io.clkpower.mpllcon = data;
                break;
	case BANKCON0:
		io.memctl.bankcon[0] = data;
		break;
	case BANKCON1:
		io.memctl.bankcon[1] = data;
		break;
	case BANKCON2:
		io.memctl.bankcon[2] = data;
		break;
	case BANKCON3:
		io.memctl.bankcon[3] = data;
		break;
	case BANKCON4:
		io.memctl.bankcon[4] = data;
		break;
	case BANKCON5:
		io.memctl.bankcon[5] = data;
		break;
	case BANKCON6:
		io.memctl.bankcon[6] = data;
		break;
	case BANKCON7:
		io.memctl.bankcon[7] = data;
		break;
	case REFRESH:
		io.memctl.refresh = data;
		break;
	case BANKSIZE:
		io.memctl.banksize = data;
		break;
	case MRSRB6:
		io.memctl.mrsrb6 = data;
		break;
	case MRSRB7:
		io.memctl.mrsrb7 = data;
		break;
	case WDCON:
		io.wd_timer.wtcon = data;
		break;
	case WDDAT:
		io.wd_timer.wtdat = data;
		break;
	case WDCNT:
		io.wd_timer.wtcnt = data;
		break;
	default:
		SKYEYE_DBG ("io_write_word(0x%08x) = 0x%08x\n", addr, data);
		fprintf(stderr, "ERROR: %s(0x%08x) = 0x%08x\n", __FUNCTION__, addr ,data);
		break;
	}
}

static void
s3c2410x_io_write_byte (void * state, ARMword addr, ARMword data)
{
	SKYEYE_DBG ("s3c2410x_io_write_byte error\n");
	// 2018-0606 ww added return statement, there was no value returned
	return s3c2410x_io_write_word (state, addr, data);
}

static void
s3c2410x_io_write_halfword (void * state, ARMword addr, ARMword data)
{
	SKYEYE_DBG ("s3c2410x_io_write_halfword error\n");
	// 2018-0606 ww added return statement, there was no value returned
	return s3c2410x_io_write_word (state, addr, data);
}


void
s3c2410x_mach_init (ARMul_State * state, machine_config_t * this_mach)
{
	ARMul_SelectProcessor (state, ARM_v4_Prop);
	this_mach->mach_io_do_cycle = s3c2410x_io_do_cycle;
	this_mach->mach_io_reset = s3c2410x_io_reset;
	this_mach->mach_io_read_byte = s3c2410x_io_read_byte;
	this_mach->mach_io_write_byte = s3c2410x_io_write_byte;
	this_mach->mach_io_read_halfword = s3c2410x_io_read_halfword;
	this_mach->mach_io_write_halfword = s3c2410x_io_write_halfword;
	this_mach->mach_io_read_word = s3c2410x_io_read_word;
	this_mach->mach_io_write_word = s3c2410x_io_write_word;

	this_mach->mach_update_int = s3c2410x_update_int;


	this_mach->mach_set_intr = s3c2410x_set_ext_intr;
	this_mach->mach_pending_intr = s3c2410x_pending_ext_intr;
	this_mach->mach_update_intr = s3c2410x_update_intr;
	this_mach->state = (void *) state;

}
