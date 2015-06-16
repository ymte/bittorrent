#include "file.h"
#include "text.h"
#include "tracker.h"
#include "coro.h"

void puts(char* txt);

void bt_from_torrent(char* str) {

}

char* url_decode_inplace(char* str) {
	char* ptr = str;
	while (*str != '&' && *str != 0) {
		if (*str == '+') {
			*ptr++ = ' ';
			str++;
		}
		else if (*str == '%') {
			str++;
			int d1 = hexdigit(*str++);
			int d2 = hexdigit(*str++);
			*ptr++ = (d1 << 4) | d2;
		}
		else
			*ptr++ = *str++;
	}
	return str;
}

void bt_from_magnet(char* str) {
	//magnet:?
	//xt=urn:btih:736b3dc769e32d91cfd3b6b70fac7f1bfe0ed7c7&
	//dn=Game+of+Thrones+-+Season+1+-+720p+BluRay+-+x264+-+ShAaNiG&
	//tr=udp%3A%2F%2Ftracker.openbittorrent.com%3A80&
	//tr=udp%3A%2F%2Fopen.demonii.com%3A1337&
	//tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969&
	//tr=udp%3A%2F%2Fexodus.desync.com%3A6969

	char* name = 0;
	char ih[20];

	// check if magnet
	if (memcmp(str, "magnet:?", 8))
		die(); // INVALID_MAGNET_LINK
	str += 8;

	// parse arguments
	while (1) {
		char* key = str;
		while (*str != '=' && *str != 0)
			str++;

		// only argument found!
		if (!*str)
			die(); // INVALID_MAGNET_LINK
		str++;

		// (start, str-start) is now "dn="

		// DISPLAY NAME
		if (!memcmp(key, "dn=", 3) || !memcmp(key, "dn.1=", 5)) {
			// decode inplace
			name = str;
			str = url_decode_inplace(str);

			// common code
			if (*str == 0)
				break;
			if (*str == '&')
				*str++ = 0;

			// print
			puts(name);
		}

		// EXACT TOPIC
		else if (!memcmp(key, "xt=", 3) || !memcmp(key, "xt.1=", 5)) {
			// check prestring
			if (memcmp(str, "urn:btih:", 9))
				die(); // ONLY_BTIH_SUPPORTED

			str += 9;

			if (!name)
				name = str;

			// 736b3dc769e32d91cfd3b6b70fac7f1bfe0ed7c7
			for (int i = 0; i < 20; i++) {
				int d1 = hexdigit(*str++);
				int d2 = hexdigit(*str++);

				ih[i] = (d1 << 4) | d2;
			}

			// common code
			if (*str == 0)
				break;
			if (*str == '&')
				*str++ = 0;
		}

		// DISPLAY NAME
		else if (!memcmp(key, "tr=", 3) || !memcmp(key, "tr.1=", 5)) {
			// decode inplace
			char* tracker = str;
			str = url_decode_inplace(str);

			run((proc)tracker_start, tracker);

			// common code
			if (*str == 0)
				break;
			if (*str == '&')
				*str++ = 0;
		}
	}
}

#include <Windows.h>

PCHAR*
CommandLineToArgvA(
PCHAR CmdLine,
int* _argc
);

int main() {
	int argc;
	char** argv;

	char* cmd = GetCommandLine();
	argv = CommandLineToArgvA(cmd, &argc);

	init();
	if (argc == 1) {
		puts("Please provide a torrent or magnet link as argument.");
		return 0;
	}

	for (int i = 1; i < argc && argv[i]; i++) {
		if (exists(argv[i]))
			run((proc)bt_from_torrent, argv[i]);
		else
			run((proc)bt_from_magnet, argv[i]);
	}

	while (1) {
		poll();
		while (runpending()) {}
	}

	return 0;
}