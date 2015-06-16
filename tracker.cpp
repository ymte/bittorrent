#include "text.h"
#include "coro.h"
#include "socket.h"
#include "tracker-packets.h"

unsigned long net_resolve(char* host);

int parse_port(char* str) {
	if (*str == 0)
		die(); // TRACKER_PORT_NOT_SPECIFIED

	int res = 0;

	while (1) {
		int digit = *str - '0';
		if (digit < 0 || digit > 9)
			die(); // TRACKER_PORT_INVALID

		res = 10 * res + digit;

		if (res > 65535)
			die(); // TRACKER_PORT_INVALID

		str++;

		// break
		if (*str == 0 || *str == '/')
			break;
	}
	return res;
}

void puts(char* txt);

void tracker_start(char* link) {
	puts(link);
	// PARSING
	if (!memcmp(link, "http://", 7))
		die(); // HTTP_TRACKER_NOT_SUPPORTED
	if (memcmp(link, "udp://", 6))
		die(); // INVALID_TRACKER_URL

	link += 6;
	char* host = link;
	while (*link != ':') {
		if (*link == 0 || *link == '/')
			die(); // TRACKER_PORT_NOT_SPECIFIED
		link++;
	}

	*link = 0;
	link++;

	// port
	int port = parse_port(link);

	// CONNECT
	unsigned long ip = net_resolve(host);

	net::udpsocket sock;

	connectrequest req;
	req.transaction = 0x19483923;
	req.hton();
	int len1 = sock.sendto(ip, port, &req, sizeof(req));
	int i = WSAGetLastError();

	connectresponse resp;
	int len2 = sock.recv(&resp, sizeof(resp));
	int j = WSAGetLastError();
	resp.hton();
	resp.action++;
}