/*
 * wsa_perror.c
 *
 *  Created on: 2014/1/20
 *      Author: ww
 */

#include <winsock2.h>
#include <stdio.h>

int wsa_perror(const char* str) {
	int err;
	char buf[256];
	err=WSAGetLastError();
	FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM,
			0,			// lpSource
			err,		// dwMessageId
			0,			// dwLanguageId
			buf,		// lpBuffer
			256,		// nSize
			0			// Arguments
	);
	fprintf(stderr,"%s: %s\n",str,buf);
	return err;
}


