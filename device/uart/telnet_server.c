/*
 * ww 2014-0120 created
 * This is taken from OpenOCD.
 */

#include <winsock2.h>
#include <process.h>

#include "skyeye_uart_telnet.h"

#include <stdio.h>
#include <ctype.h>

#define TELNET_BUFFER_SIZE (32)

enum telnet_states {
	TELNET_STATE_DATA,
	TELNET_STATE_IAC,
	TELNET_STATE_SB,
	TELNET_STATE_SE,
	TELNET_STATE_WILL,
	TELNET_STATE_WONT,
	TELNET_STATE_DO,
	TELNET_STATE_DONT,
	TELNET_STATE_ESCAPE,
};

typedef struct telnet_connection_s {
	enum telnet_states state;
	char last_escape;
	int closed;
} telnet_connection_t;

/* generic socket functions for Windows and Posix */
static __inline int write_socket(int handle, const void *buffer, unsigned int count) {
	return send(handle, buffer, count, 0);
}

static __inline int read_socket(int handle, void *buffer, unsigned int count) {
	return recv(handle, buffer, count, 0);
}

static char *negotiate = "\xFF\xFB\x03" /* IAC WILL Suppress Go Ahead */
		"\xFF\xFB\x01" /* IAC WILL Echo */
		"\xFF\xFD\x03" /* IAC DO Suppress Go Ahead */
		"\xFF\xFE\x01"; /* IAC DON'T Echo */

#define CTRL(c) (c - '@')

/* The only way we can detect that the socket is closed is the first time
 * we write to it, we will fail. Subsequent write operations will
 * succeed. Shudder!
 */
int telnet_write(uart_telnet_device *netdev, const void *data, int len) {
	telnet_connection_t *t_con = netdev->priv;
	if (t_con->closed) return 0;

	if (write_socket(netdev->remote_desc, data, len) == len) {
		return 1;
	}
	t_con->closed = 1;
	return 0;
}

int telnet_new_connection(uart_telnet_device *netdev) {
	telnet_connection_t *telnet_connection = malloc(sizeof(telnet_connection_t));
//	telnet_service_t *telnet_service = connection->service->priv;

	netdev->priv = telnet_connection;

	/* initialize telnet connection information */
	telnet_connection->closed = 0;
	telnet_connection->state = TELNET_STATE_DATA;

	/* output goes through telnet connection */

	/* negotiate telnet options */
	telnet_write(netdev, negotiate, strlen(negotiate));

	/* print connection banner */
	if (netdev->banner) {
		telnet_write(netdev, netdev->banner, strlen(netdev->banner));
		telnet_write(netdev, "\r\n\0", 3);
	}

	return 0;
}

int telnet_input(uart_telnet_device *netdev) {
	int bytes_read;
	char buffer[TELNET_BUFFER_SIZE];
	char *buf_p;
	telnet_connection_t *t_con = netdev->priv;

	bytes_read = read_socket(netdev->remote_desc, buffer, TELNET_BUFFER_SIZE);

	if (bytes_read <= 0) return bytes_read;

	buf_p = buffer;
	while (bytes_read) {
		switch (t_con->state) {
		case TELNET_STATE_DATA:
			if (*buf_p == '\xff') {
				t_con->state = TELNET_STATE_IAC;
			}
			else {
				if (*buf_p == '\r') {	// return key generates "\r\n", eat up \n
					if ((bytes_read > 1) && (*(buf_p + 1) == '\n')) {
						buf_p++;
						*buf_p = '\r';
						bytes_read--;

					}
				}
				else if (*buf_p == '\n') {	// return key generates "\n\r", eat up \r
					if ((bytes_read > 1) && (*(buf_p + 1) == '\r')) {
						buf_p++;
						bytes_read--;

					}
				}

				if (!uart_telnet_putch(netdev, buf_p)) {
					/* output audible bell if buffer is full */
					telnet_write(netdev, "\x07", 1); /* "\a" does not work, at least on windows */
				}
			}
			break;
		case TELNET_STATE_IAC:
			switch (*buf_p) {
			case '\xfe':
				t_con->state = TELNET_STATE_DONT;
				break;
			case '\xfd':
				t_con->state = TELNET_STATE_DO;
				break;
			case '\xfc':
				t_con->state = TELNET_STATE_WONT;
				break;
			case '\xfb':
				t_con->state = TELNET_STATE_WILL;
				break;
			}
			break;
		case TELNET_STATE_SB:
			break;
		case TELNET_STATE_SE:
			break;
		case TELNET_STATE_WILL:
		case TELNET_STATE_WONT:
		case TELNET_STATE_DO:
		case TELNET_STATE_DONT:
			t_con->state = TELNET_STATE_DATA;
			break;
		default:
			fprintf(stderr, "unknown telnet state\n");
			exit(-1);
		}

		bytes_read--;
		buf_p++;
	}

	return 1;
}

int telnet_connection_closed(uart_telnet_device *netdev) {
	/* if this connection registered a debug-message receiver delete it */

	if (netdev->priv) {
		free(netdev->priv);
		netdev->priv = NULL;
	}
	else {
		fprintf(stderr, "BUG: priv pointer is NULL\n");
	}

	return 0;
}

