/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 2004 Alexander Guy <alexander.guy@andern.org>
 * Copyright (c) 2020 Musatov Danila <danilarumus2000@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
 //extern "C"{
#include <winsock2.h>
#include <Ws2tcpip.h>
//};
#include <iostream>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
//#include <minwinbase.h>
#include "ntp_server.h"
#define SA_LEN(x) ((x)->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)


typedef LONG_PTR ssize_t;

using namespace std;

int
ntp_getmsg(struct sockaddr *sa, char *p, ssize_t len, struct ntp_msg *msg)
{
	if (len != NTP_MSGSIZE_NOAUTH && len != NTP_MSGSIZE) { 
		cerr << "malformed packet received from %s";
		return (-1);
	}

	memcpy(msg, p, sizeof(*msg));

	return (0);
}

int
ntp_sendmsg(int fd, struct sockaddr *sa, struct ntp_msg *msg)
{
	socklen_t       sa_len;
	ssize_t         n;

	if (sa != NULL)
		sa_len = SA_LEN(sa); // sa_len 
	else
		sa_len = 0;

	n = sendto(fd, (const char*)msg, sizeof(*msg), 0, sa, sa_len); 
	if (n == -1) {
		if (errno == ENOBUFS || errno == EHOSTUNREACH ||
			errno == ENETDOWN) {
			/* logging is futile */
			return (-1);
		}
		cerr << "sendto"; // cout?
		return (-1);
	}

	if (n != sizeof(*msg)) {
		cerr << "ntp_sendmsg: only part of bytes sent";
		return (-1);
	}

	return (0);
}

const char *
log_sockaddr(struct sockaddr *sa)
{
	static char     buf[NI_MAXHOST];

	if (getnameinfo(sa, SA_LEN(sa), buf, sizeof(buf), NULL, 0,
		NI_NUMERICHOST))
		return ("(unknown)");
	else
		return (buf);
}

struct s_fixedpt
	d_to_sfp(double d) 
{
	struct s_fixedpt        sfp;

	sfp.int_parts = htons((uint16_t)d);
	sfp.fractions = htons((uint16_t)((d - (uint16_t)d) * USHRT_MAX));

	return (sfp);
}

struct l_fixedpt
	d_to_lfp(double d)
{
	struct l_fixedpt        lfp;

	while (d > SECS_IN_ERA)
		d -= SECS_IN_ERA;
	lfp.int_partl = htonl((uint32_t)d);
	lfp.fractionl = htonl((uint32_t)((d - (uint32_t)d) * UINT_MAX));

	return (lfp);
}

struct l_fixedpt 
	d_to_lfp(SYSTEMTIME time)
{
	PFILETIME ft=(PFILETIME)malloc(sizeof(FILETIME));
	SystemTimeToFileTime(&time, ft);
	ULARGE_INTEGER helper;
	helper.HighPart = ft->dwHighDateTime;
	helper.LowPart = ft->dwLowDateTime;
	struct l_fixedpt        lfp;
	double p = helper.QuadPart;
	p /= pow(10, 7);
	p -= 9435484800; //seconds from 01.01.1601
	while (p > SECS_IN_ERA) //maybe you don't need a tc I think just before 1900
		p -= SECS_IN_ERA;
	lfp.int_partl = htonl((uint32_t)p);
	lfp.fractionl = htonl((uint32_t)((p - (uint32_t)p) * UINT_MAX));
	free(ft); 
	return (lfp);
}

/*
double
lfp_to_d(struct l_fixedpt lfp)
{
	double  base, ret;

	lfp.int_partl = ntohl(lfp.int_partl);
	lfp.fractionl = ntohl(lfp.fractionl);

	base = NTP_ERA;
	if (lfp.int_partl <= INT32_MAX)
		base++;
	ret = base * SECS_IN_ERA;
	ret += (double)(lfp.int_partl) + ((double)lfp.fractionl / UINT_MAX);

	return (ret);
}
*/
