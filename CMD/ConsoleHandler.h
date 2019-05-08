#pragma once
#include <windows.h>
#include <stdio.h>
#include "Package.h"


class ConsoleHandler {
public:
	ConsoleHandler();
	Package readConsole();
	int writeConsole(Package pack);
private:
	HANDLE hStdin, hStdout;
	DWORD cNumRead = 0, lpNumberOfCharsWritten = 0;
	char lpBuffer[DEFAULT_BUFLEN];
	Package pack;
	VOID ErrorExit(LPSTR);
	_CONSOLE_READCONSOLE_CONTROL control;
};


