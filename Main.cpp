/*
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


#include<iostream>
#include<cmath>
#include<ctime>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include <winsock2.h>
#include "ntp_server.h"


#define PFD_PIPE_MAIN   0
#define PFD_PIPE_DNS    1
#define PFD_SOCK_CTL    2
#define PFD_MAX         3

int main()
{
	unsigned int listener_cnt = 0;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	setup_listeners(&listener_cnt);
	pollfd* pd = (pollfd*)calloc(sizeof(pollfd), listener_cnt);
	struct listen_addr* v;
	unsigned int counter=0;
	
	TAILQ_FOREACH(v, &listen_addrs, entry){
		pd[counter].fd = v->fd;
		pd[counter].events = POLLIN;
		pd[counter].revents = 0;
		counter++;
	}
	struct listen_addr* a;
	while (1) {
		WSAPoll(pd, listener_cnt, -1);
		counter = 0;
		TAILQ_FOREACH(a, &listen_addrs, entry) {
			if(pd[counter].revents & POLLIN)	server_dispatch(a);
			counter++;
		}

	}
	WSACleanup();
	
	return 0;
}