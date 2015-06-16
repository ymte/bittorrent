#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#pragma pack(push, 1)

typedef unsigned long long uint64;
typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

// peer
struct BTPeer {
	uint32 ip;
	uint16 port;
};

// connect
struct connectrequest {
	uint64 connection;
	uint32 action; // {connect, announce, scrape}
	uint32 transaction;

	inline connectrequest() {
		//connection = htonll(0x41727101980ULL);
		connection = 0x801910271704ULL;
		action = 0;
	}

	inline void hton() {
		//connection = htonll(connection);
		action = htonl(action);
		transaction = htonl(transaction);
	}
};

struct connectresponse {
	uint32 action;
	uint32 transaction;
	uint64 connection;

	inline void hton() {
		action = htonl(action);
		transaction = htonl(transaction);
		//connection = htonll(connection);
	}
};

// announce
struct announcerequest {
	uint64 connection;
	uint32 action;
	uint32 transaction;
	uint8 infohash[20];
	uint8 peerid[20];
	uint64 downloaded;
	uint64 left;
	uint64 uploaded;
	uint32 event; // {none, completed, started, stopped}
	uint32 ip;
	uint32 key;
	uint32 numwant;
	uint16 port;
};

struct announceresponse {
	uint32 action; // (event)
	uint32 transaction;
	uint32 interval;
	uint32 leechers;
	uint32 seeders;
	BTPeer peers[200];
};

struct P3a {
	uint64 connection;
	uint32 action; // 2
	uint32 transaction;
	//Infohash infohashes[];
};

struct Response {
	uint32 complete;
	uint32 downloaded;
	uint32 incomplete;
};

struct P3b {
	uint32 action; // 2
	uint32 transaction;
	//Response responses[];
};

// all
enum code { CONNECT, ANNOUNCE, SCRAPE, FAILURE };
union response {
	code code;
	//P1b connect;
	//P2b announce;
	P3b scrape;
	int error;
};

#pragma pack(pop)