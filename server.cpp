/*      $OpenBSD: server.c,v 1.44 2016/09/03 11:52:06 reyk Exp $ */

/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 2004 Alexander Guy <alexander@openbsd.org>
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


//#include <sys/ioctl.h>
//#include <sys/socket.h>
//#include <net/if.h>
//#include <unistd.h>
//#include <ifaddrs.h>
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
//extern "C" {
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
//};
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "ntp_server.h"

#define WORKING_BUFFER_SIZE 15360
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

#define SA_LEN(x) (((x)->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))

typedef LONG_PTR ssize_t; 

using namespace std; 

struct listen_addrs	listen_addrs = TAILQ_HEAD_INITIALIZER(listen_addrs);

int
setup_listeners(unsigned int* cnt)
{
	struct listen_addr      *la;
	struct sockaddr         *sa;
	uint8_t                 *a6;
	size_t                   sa6len = sizeof(struct in6_addr);
	unsigned int                   new_cnt = 0;
	int                      tos = IPTOS_LOWDELAY;
	PIP_ADAPTER_ADDRESSES AdapterAddresses;
	ULONG outBufLen = 15360;
	DWORD dwRetVal = 0;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG family = AF_UNSPEC;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	ULONG Iterations = 0;

	do {
			pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);
			if (pAddresses == NULL) {
				printf
				("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
				exit(1);
			}
				dwRetVal =
			GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

			if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
				FREE(pAddresses);
				pAddresses = NULL;
			}
			else {
				break;
			}

			Iterations++;

		} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));


	for (PIP_ADAPTER_ADDRESSES helper = pAddresses; helper != NULL; helper=helper->Next ) {
		for(PIP_ADAPTER_UNICAST_ADDRESS pUnicast = helper->FirstUnicastAddress; pUnicast != NULL; pUnicast = pUnicast->Next){
			sa = pUnicast->Address.lpSockaddr;
			if (sa == NULL || SA_LEN(sa) == 0)
				continue;
			if (sa->sa_family != AF_INET &&
				sa->sa_family != AF_INET6)
				continue;

			if (sa->sa_family == AF_INET &&
				((struct sockaddr_in *)sa)->sin_addr.s_addr ==
				INADDR_ANY)
				continue;

			if (sa->sa_family == AF_INET6) {
				a6 = ((struct sockaddr_in6 *)sa)->
					sin6_addr.s6_addr;
				if (memcmp(a6, &in6addr_any, sa6len) == 0)
					continue;
			}
				
			if ((la = (struct listen_addr*)calloc(1, sizeof(struct listen_addr))) ==
				NULL)
				cerr << "setup_listeners calloc";
		
			memcpy(&la->sa, sa, SA_LEN(sa));
			TAILQ_INSERT_TAIL(&listen_addrs, la , entry);
			new_cnt++;

			switch (la->sa.ss_family) {
			case AF_INET:
				if (((struct sockaddr_in *)&la->sa)->sin_port == 0)
					((struct sockaddr_in *)&la->sa)->sin_port =
					htons(123);
				break;
			case AF_INET6:
				if (((struct sockaddr_in6 *)&la->sa)->sin6_port == 0)
					((struct sockaddr_in6 *)&la->sa)->sin6_port =
					htons(123);
				break;
			default:
				cerr << "king bula sez: af borked";
			}

			if ((la->fd = socket(la->sa.ss_family, SOCK_DGRAM, 0)) == -1)
				cerr << "socket";
			/*if (la->sa.ss_family == AF_INET && setsockopt(la->fd,
				IPPROTO_IP, IP_TOS, (const char*)&tos, sizeof(tos)) == -1)
				cerr << "setsockopt IPTOS_LOWDELAY";*/

			if (bind(la->fd, (struct sockaddr *)&la->sa,
				SA_LEN((struct sockaddr *)&la->sa)) == -1) {
				cerr << "bind failed, skipping " << endl; cout << WSAGetLastError() << endl;
				closesocket(la->fd);
			}
		}
	}
	*cnt = new_cnt;
	if (pAddresses) {
		FREE(pAddresses);
	}
	return (0);
}

int
server_dispatch(struct listen_addr* listen_addr)
{
	ssize_t                  size;
	double                   rectime;
	struct sockaddr_storage  fsa;
	socklen_t                fsa_len;
	struct ntp_msg           query, reply;
	char                     buf[NTP_MSGSIZE];

	fsa_len = sizeof(fsa);
	if ((size = recvfrom(listen_addr->fd, buf, sizeof(buf), 0,
		(struct sockaddr *)&fsa, &fsa_len)) == -1) {
		if (errno == EHOSTUNREACH ||
			errno == ENETUNREACH || errno == ENETDOWN) {			
			cerr<< "recvfrom" ;
			return (0);
		}
		else				
			cerr << "recvfrom";
	}
	SYSTEMTIME st;
	GetSystemTime(&st);

	if (ntp_getmsg((struct sockaddr *)&fsa, buf, size, &query) == -1) 
		return (0);

	memset(&reply, 0, sizeof(reply));
	reply.status = (query.status & VERSIONMASK);
	if ((query.status & MODEMASK) == MODE_CLIENT)
		reply.status |= MODE_SERVER;
	else if ((query.status & MODEMASK) == MODE_SYM_ACT)
		reply.status |= MODE_SYM_PAS;
	else /* ignore packets of different type (e.g. bcast) */
		return (0);

	reply.stratum = 1;
	reply.ppoll = query.ppoll;
	reply.precision = 0.001;
	reply.rectime = d_to_lfp(st);
	reply.reftime = d_to_lfp(st);
	GetSystemTime(&st); 
	reply.xmttime = d_to_lfp(st);
	reply.orgtime = query.xmttime;
	reply.rootdelay = d_to_sfp(0);
	reply.refid = 0;

	ntp_sendmsg(listen_addr->fd, (struct sockaddr *)&fsa, &reply);
	return (0);
}