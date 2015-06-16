// implementation for windows functions
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include "file.h"
#include "coro.h"

bool exists(const char* path) {
	DWORD dwAttrib = GetFileAttributes(path);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


unsigned long net_resolve(char* host) {
	// hints
	addrinfo hints = { 0, AF_INET, SOCK_STREAM, 0, 0, 0, 0, 0 };

	addrinfo* result;

	// i cri everytiem
	getaddrinfo(host, 0, &hints, &result);

	unsigned long ip = htonl(*(unsigned long*)((char*)&result->ai_addr->sa_data + 2));

	return ip;
}

const int MAX_FIBERS = 0x100;

// fiber stuff
typedef void* fiber;

fiber current;
fiber main;

fiber dead[MAX_FIBERS];
int numdead;

fiber reading[MAX_FIBERS];
fiber writing[MAX_FIBERS];
int reading_socks[MAX_FIBERS];
int writing_socks[MAX_FIBERS];
int numreading;
int numwriting;

fiber pending[MAX_FIBERS];
int numpending;

void init() {
	main = ConvertThreadToFiber(0);
	current = main;
	numdead = 0;
	numreading = 0;
	numwriting = 0;
	numpending = 0;
}

void cleanup() {
	for (int i = 0; i < numdead; i++)
		DeleteFiber(dead[i]);
	numdead = 0;
	numreading = 0;
	numwriting = 0;
	numpending = 0;
}

void die() {
	dead[numdead++] = current;
	SwitchToFiber(main);
}

void wait_socket_read(int sid) {
	reading[numreading] = current;
	reading_socks[numreading] = sid;
	numreading++;
	current = main;
	SwitchToFiber(main);
}

void wait_socket_write(int sid) {
	writing[numwriting] = current;
	writing_socks[numwriting] = sid;
	numwriting++;
	current = main;
	SwitchToFiber(main);
}

void remove_socket(int sid) {
	for (int i = 0; i < numreading; i++) {
		if (reading_socks[i] == sid) {
			// match! remove it
			if (i != numreading - 1) {
				reading_socks[i] = reading_socks[numreading - 1];
				reading[i] = reading[numreading - 1];
			}
			else
				numreading--;
		}
	}
}

bool runpending() {
	if (!numpending)
		return false;
	fiber fresh = pending[--numpending];
	current = fresh;
	SwitchToFiber(fresh);
	int i = GetLastError();
	return true;
}


struct info {
	proc p;
	void* arg;
};

void fiberproc(info* i) {
	info i2 = *i;
	SwitchToFiber(main);
	i2.p(i2.arg);
	die();
}

void run(proc p, void* arg) {
	info i;
	i.p = p;
	i.arg = arg;
	fiber f = CreateFiber(0x1000, (LPFIBER_START_ROUTINE)fiberproc, &i);
	SwitchToFiber(f); // it will return immediately
	pending[numpending++] = f;
}

void poll() {
	// variables
	fd_set r, w;
	FD_ZERO(&r);
	FD_ZERO(&w);

	// input
	for (int i = 0; i < numreading; i++)
		FD_SET(reading_socks[i], &r);
	for (int i = 0; i < numwriting; i++)
		FD_SET(writing_socks[i], &w);

	timeval time;
	time.tv_sec = 10000;
	time.tv_usec = 0;
	
	// call
	select(99, &r, &w, 0, &time);

	// output
	for (int i = 0; i < numreading; i++) {
		if (FD_ISSET(reading_socks[i], &r)) {
			pending[numpending++] = reading[i];

			// remove from list
			if (i == numreading - 1) {
				numreading--;
			}
			else {
				reading[i] = reading[numreading - 1];
				reading_socks[i] = reading_socks[numreading - 1];
				numreading--;
			}
		}
	}
	for (int i = 0; i < numwriting; i++) {
		if (FD_ISSET(writing_socks[i], &w)) {
			pending[numpending++] = writing[i];

			// remove from list
			if (i == numwriting - 1) {
				numwriting--;
			}
			else {
				writing[i] = writing[numwriting - 1];
				writing_socks[i] = writing_socks[numwriting - 1];
				numwriting--;
			}
		}
	}
}