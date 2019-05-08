#pragma once
#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Package.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "27015"


class Client {
private:
	WSADATA wsaData;
	SOCKET ConnectSocket;
	struct addrinfo *result, *ptr, hints;
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen;
	Package data;

public:
	Client();
	int client_initialize(int argc, char **argv);
	int client_connect();
	int client_data_send(Package& pack);
	Package& client_recv();
	int client_close_connection();
};
