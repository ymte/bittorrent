#pragma once
#define ioctl ioctlsocket
#define close closesocket

typedef unsigned long long uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef int native_socket;
namespace net {
	struct socket {
		int id;

		socket();
		socket(native_socket sid);
		~socket();

		void connect(unsigned long ip, unsigned short port);
		int sendto(unsigned long ip, unsigned short port, void* buf, int len);
		int send(void* buf, int len);
		int recv(void* buf, int len);
		void bind(unsigned short port);
		void listen();
		native_socket accept(unsigned long* ip = 0, unsigned short* port = 0);

		void reg();
	};

	struct udpsocket : socket {
		udpsocket();
		udpsocket(native_socket sid);
	};
}

#undef close
#undef ioctl