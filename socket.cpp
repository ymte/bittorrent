#include "socket.h"
#include "coro.h"
#include <WinSock2.h>

#define ioctl ioctlsocket
#define close closesocket
typedef sockaddr_in address;

namespace net {
	udpsocket::udpsocket() {
		id = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		reg();
		u_long nonblocking = true;
		ioctl(id, FIONBIO, &nonblocking);
	}

	udpsocket::udpsocket(native_socket sid) {
		id = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		reg();
		u_long nonblocking = true;
		ioctl(id, FIONBIO, &nonblocking);
	}

	socket::socket() {
		id = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		reg();
		u_long nonblocking = true;
		ioctl(id, FIONBIO, &nonblocking);
	}

	socket::socket(native_socket sid) {
		id = sid;
		reg();
		u_long nbio = true;
		ioctl(id, FIONBIO, &nbio);
	}

	socket::~socket() {
		//if (id) {
		//	close(id);
		//	id = 0;
		//}
	}

	void socket::connect(unsigned long ip, unsigned short port) {
		address ad;
		ad.sin_addr.s_addr = ip;
		ad.sin_port = htons(port);
		::connect(id, (sockaddr*)&ad, sizeof(ad));
		wait_socket_write(id);
	}

	int socket::send(void* buf, int len) {
		wait_socket_write(id);
		int sent = ::send(id, (char*)buf, len, 0);
		if (sent != len)
			wait_socket_write(id);
		return sent;
	}

	int socket::sendto(uint32 ip, uint16 port, void* buf, int len) {
		wait_socket_write(id);
		sockaddr_in to;
		to.sin_family = AF_INET;
		to.sin_addr.s_addr = htonl(ip);
		to.sin_port = htons(port);
		int sent = ::sendto(id, (char*)buf, len, 0, (sockaddr*)&to, sizeof(sockaddr_in));
		int i = GetLastError();
		return sent;
	}

	int socket::recv(void* buf, int len) {
		//unsigned long am;
		//ioctl(id, FIONREAD, &am);
		//if (!am)
			wait_socket_read(id);
		return ::recv(id, (char*)buf, len, 0);
	}
	/*	while (len) {
			read(*this);
			int res = ::recv(id, (char*)buf, len, 0);
			if (res == -1 || res == 0) {
			printf("disconnect\n");
			coros::die();
			}
			len -= res;
			*(char**)buf += res;
			}
			}*/

	void socket::bind(unsigned short port) {
		address ad;
		ad.sin_addr.s_addr = 0;
		ad.sin_port = htons(port);
		ad.sin_family = AF_INET;

		byte* b = (byte*)&ad;

		::bind(id, (sockaddr*)&ad, sizeof(ad));
	}

	void socket::listen() {
		::listen(id, 99);
	}

	native_socket socket::accept(unsigned long* ip, unsigned short* port) {
		wait_socket_read(id);

		int len = sizeof(sockaddr);

		while (1) {
			address other;
			native_socket sid = ::accept(id, (sockaddr*)&other, (int*)&len);
			if (ip)
				*ip = other.sin_addr.s_addr;
			if (port)
				*port = htons(other.sin_port);

			if (sid == 0)
				return 0;

			if (sid == -1)
				wait_socket_read(id);

			return sid;
		}
	}

#ifdef _WIN32
	void __stdcall cb(void* data) {
		close((native_socket)data);
	}

	void socket::reg() {
		FlsSetValue(FlsAlloc(cb), (void*)id);
	}
#else
	void socket::reg() {
		printf("TODO close socket after coro shutdown");
	}
#endif

	// windows hack
#ifdef _WIN32
	class _WIN_NET {
	public:
		_WIN_NET() {
			WSADATA windat;
			WSAStartup(MAKEWORD(2, 2), &windat);
		}
	} _win_net;
#endif

#undef close
#undef ioctl
}