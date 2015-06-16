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

void puts(char* txt) {
	int len = 0;
	while (txt[len])
		len++;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), txt, len, 0, 0);
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, 0, 0);
}

unsigned long net_resolve(char* host) {
	// hints
	addrinfo hints = { 0, AF_INET, SOCK_STREAM, 0, 0, 0, 0, 0 };

	addrinfo* result = 0;

	// i cri everytiem
	getaddrinfo(host, 0, &hints, &result);

	unsigned long ip = htonl(*(unsigned long*)((char*)&result->ai_addr->sa_data + 2));

	return ip;
}

const int MAX_FIBERS = 0x1000;

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
	// net
	WSADATA windat;
	WSAStartup(MAKEWORD(2, 2), &windat);

	// coros
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

void rest() {
	dead[numdead++] = current;
	SwitchToFiber(main);
}

void die() {
	puts("trimming fat...");
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
	fiber f;
	void* arg;
};

void fiberproc(info* i) {
	info i2 = *i;
	SwitchToFiber(i2.f);
	i2.p(i2.arg);
	rest();
}

void run(proc p, void* arg) {
	info i;
	i.p = p;
	i.f = current;
	i.arg = arg;
	fiber f = CreateFiber(0x10000, (LPFIBER_START_ROUTINE)fiberproc, &i);
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

PCHAR*
CommandLineToArgvA(
PCHAR CmdLine,
int* _argc
)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len + 2) / 2)*sizeof(PVOID)+sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len + 2)*sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv) + i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while (a = CmdLine[i]) {
		if (in_QM) {
			if (a == '\"') {
				in_QM = FALSE;
			}
			else {
				_argv[j] = a;
				j++;
			}
		}
		else {
			switch (a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if (in_SPACE) {
					argv[argc] = _argv + j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if (in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if (in_SPACE) {
					argv[argc] = _argv + j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}