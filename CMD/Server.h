#pragma once
#include "pch.h"
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Package.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "27015"



class Server {
private:
	WSADATA wsaData;
	int iResult;
	int iSendResult;

	SOCKET ListenSocket;
	SOCKET ClientSocket;

	struct addrinfo *result;
	struct addrinfo hints;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen;
public:
	Server();
	int set_up_socket();
	int listen_socket();
	int accept_client_socket();
	int recv_data();
	int close_connection();
};