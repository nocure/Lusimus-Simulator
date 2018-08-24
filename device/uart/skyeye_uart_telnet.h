/*
 * skyeye_uart_telnet.h
 *
 *  Created on: 2014/1/20
 *      Author: ww
 */

#ifndef SKYEYE_UART_TELNET_H_
#define SKYEYE_UART_TELNET_H_

// ww 2018-0530 added include
#include <stdint.h>

#define TELNET_BANNER_TEXT "[UART on TELNET ww 2014-0120]"
#define DEFAULT_TELNET_PORT 10025

#define BUF_SIZE	16

typedef struct {
	uintptr_t threadid;
	int remote_desc;
	int local_desc;
	int port;
	int exit_thread_now;

	char buf[BUF_SIZE];
	int bptr;
	int cnt;

	const char *banner;
	void *priv;
} uart_telnet_device;

int uart_telnet_putch(uart_telnet_device *netdev, char *p);

int telnet_new_connection(uart_telnet_device *netdev);
int telnet_input(uart_telnet_device *netdev);
int telnet_connection_closed(uart_telnet_device *netdev);
int telnet_write(uart_telnet_device *netdev, const void *data, int len);

#endif /* SKYEYE_UART_TELNET_H_ */
