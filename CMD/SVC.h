#pragma once
#include <tchar.h>
#include <strsafe.h>
#include "pch.h"
#include "Server.h"

#define SVCNAME (char *)TEXT("SvcName")



VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR *);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR *);
VOID SvcReportEvent(LPTSTR);
void __cdecl _ttmain(int argc, TCHAR *argv[]);


