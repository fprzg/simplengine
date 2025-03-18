#pragma once

#include <errhandlingapi.h>
#include <processthreadsapi.h>



//
// FUNCTION DECLARATIONS
//

#define profiler_Begin() _profiler_Begin((char *)__FUNCTION__)
#define profiler_End() _profiler_End()

DWORD WINAPI
profiler_LogWriterThread(LPVOID Param);

b8
profiler_Init();

void
profiler_QueueLogEntry(char *Message);

void
_profiler_Begin(char *Name);

void
_profiler_End();
void
profiler_Shutdown();

double
GetTime();



//
// IMPLEMENTATION
//

#if defined(_PLATFORM_WIN32)
#include "win32_simplengine_profiler.cpp"
#endif