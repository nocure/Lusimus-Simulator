/*
 * ww 2014-0120 created
 */

#include <winsock2.h>
#include <process.h>
#include <skyeye.h>
#include "skyeye_uart.h"
#include "skyeye_uart_telnet.h"

static int uart_telnet_getch(uart_telnet_device *netdev, char *p) {
	if (!netdev->cnt) return 0;
	netdev->cnt--;
	*p = netdev->buf[netdev->bptr++];
	if (netdev->bptr >= BUF_SIZE) netdev->bptr -= BUF_SIZE;
	return 1;
}

int uart_telnet_putch(uart_telnet_device *netdev, char *p) {
	int w;
	if (netdev->cnt >= BUF_SIZE) return 0;
	w = netdev->bptr + netdev->cnt;
	if (w >= BUF_SIZE) w -= BUF_SIZE;
	netdev->buf[w] = *p;
	netdev->cnt++;
	return 1;
}

static void uart_telnet_thread(void *arglist) {
	int tmp;
	struct sockaddr_in sockaddr;
	struct uart_device *uart_dev;
	uart_telnet_device *netdev;
	int ret;

	uart_dev = arglist;
	netdev = uart_dev->priv;

	while (!netdev->exit_thread_now) {
		if ((netdev->local_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) wsa_perror("[uart_telnet]Can't open socket");
		sockaddr.sin_family = PF_INET;
		sockaddr.sin_port = htons(netdev->port);
		sockaddr.sin_addr.s_addr = INADDR_ANY;

		if (bind(netdev->local_desc, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) || listen(netdev->local_desc, 1)) {
			wsa_perror("[uart_telnet]Can't bind address");
			netdev->exit_thread_now=1;
			continue;
		}

		tmp = sizeof(sockaddr);
		ret = accept(netdev->local_desc, (struct sockaddr *)&sockaddr, &tmp);
		if (ret == SOCKET_ERROR) {
			wsa_perror("[uart_telnet]Accept failed");
			closesocket(netdev->local_desc); /* No longer need this */
			netdev->exit_thread_now=1;
			continue;
		}
		netdev->remote_desc = ret;

		/* Enable TCP keep alive process. */
		tmp = 1;
		setsockopt(netdev->local_desc, SOL_SOCKET, SO_KEEPALIVE, (char *)&tmp, sizeof(tmp));

		/* Tell TCP not to delay small packets.  This greatly speeds up interactive response. */
		tmp = 1;
		setsockopt(netdev->remote_desc, 6 /* PROTO_TCP */, TCP_NODELAY, (char *)&tmp, sizeof(tmp));

		closesocket(netdev->local_desc); /* No longer need this */
		netdev->local_desc = 0;

		printf("UART connected on TELNET using port:%d\n", netdev->port);

		telnet_new_connection(netdev);

		while (1) {
			ret = telnet_input(netdev);
			if (ret == 0) break;	// connection closed
			if (ret < 0) {
				wsa_perror("[uart_telnet]Recv failed");
				break;
			}
		}
		telnet_connection_closed(netdev);
		netdev->remote_desc = 0;
		printf("UART disconnected\n");
	}
	_endthread();
}

int uart_telnet_open(struct uart_device *uart_dev) {
	uart_telnet_device *p;

	p = malloc(sizeof(uart_telnet_device));
	if (!p) return -1;
	memset(p, 0, sizeof(uart_telnet_device));
	uart_dev->priv = p;

	if (uart_dev->desc_in) {
		p->port = atoi(uart_dev->desc_in);
	}

	if (!p->port) {
		p->port = DEFAULT_TELNET_PORT;
	}
	printf("uart_mod:Using TCP port %d\n", p->port);

	p->banner = TELNET_BANNER_TEXT;
	p->threadid = _beginthread(uart_telnet_thread, 0, uart_dev);

	if (p->threadid <= 0) {
		free(p);
		return -1;
	}
	return 0;
}

int uart_telnet_close(struct uart_device *uart_dev) {
	uart_telnet_device *netdev = uart_dev->priv;
	netdev->exit_thread_now = 1;
	closesocket(netdev->local_desc);	// close local socket to break waiting for accept.
	closesocket(netdev->remote_desc);	// close local socket to break waiting for accept.
	WaitForSingleObject((HANDLE)netdev->threadid, INFINITE);
	free(netdev);
	uart_dev->priv = 0;
	return 0;
}

int uart_telnet_read(struct uart_device *uart_dev, void *buf, size_t count, struct timeval *timeout) {
	int i;
	uart_telnet_device *netdev = uart_dev->priv;
	if (!netdev->remote_desc) return 0;	// if not connected return 0
	for (i = 0; i < count; i++) {
		if (!uart_telnet_getch(netdev, buf)) break;
		buf++;
	}
	return i;
}

int uart_telnet_write(struct uart_device *uart_dev, void *buf, size_t count) {
	uart_telnet_device *netdev = uart_dev->priv;
	if (!netdev->remote_desc) return 0;	// if not connected return 0
	return telnet_write(netdev, buf, count);
}

