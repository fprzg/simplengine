#pragma once

#include <errhandlingapi.h>
#include <processthreadsapi.h>

double
GetTime()
{
    local LARGE_INTEGER frequency;
    local BOOL initialized = FALSE;
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = TRUE;
    }
    
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / frequency.QuadPart;
}

struct profile_data
{
    char Name[64];
    LARGE_INTEGER Start;
    LARGE_INTEGER End;
    LARGE_INTEGER Frequency;
    struct profile_data *Next;
};

#define LOG_BUFFER_SIZE 4096

// TODO: Use string arenas instead of fixed-size char buffers
// TODO: Separate the logging and profiling systems.

struct log_entry
{
    char Message[512];
    struct log_entry *Next;
};

struct profiler_ctx
{
    log_entry *QueueHead;
    log_entry *QueueTail;
    HANDLE LogFile;
    HANDLE Mutex;
    HANDLE Event;
    HANDLE Thread;
    DWORD LogThreadID;
    b8 Running;
};

profile_data *Stack;
FILE *LogFile;

global profiler_ctx GlobalProfilerCtx;
__declspec(thread) profile_data *ThreadProfilerStack;

#define profiler_Begin() _profiler_Begin((char *)__FUNCTION__)
#define profiler_End() _profiler_End()

DWORD WINAPI
profiler_LogWriterThread(LPVOID Param)
{
    while(GlobalProfilerCtx.Running)
    {
        WaitForSingleObject(GlobalProfilerCtx.Event, INFINITE);
        
        WaitForSingleObject(GlobalProfilerCtx.Mutex, INFINITE);
        
        while(GlobalProfilerCtx.QueueHead)
        {
            log_entry *Entry = GlobalProfilerCtx.QueueHead;
            GlobalProfilerCtx.QueueHead = GlobalProfilerCtx.QueueHead->Next;
            
            if(GlobalProfilerCtx.QueueHead == null)
            {
                GlobalProfilerCtx.QueueTail = null;
            }
            
            DWORD BytesWritten;
            WriteFile(GlobalProfilerCtx.LogFile, Entry->Message, (DWORD)strlen(Entry->Message), &BytesWritten, null);
            FlushFileBuffers(GlobalProfilerCtx.LogFile);
            free(Entry);
        }
        
        ReleaseMutex(GlobalProfilerCtx.Mutex);
    }
    
    return 0;
}

b8
profiler_Init()
{
    char FilePath[256];
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    snprintf(FilePath, sizeof(FilePath),
             "..\\logs\\flamegraph %04d-%02d-%02d %02d.%02d.%02d.txt",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond
             //, st.wMilliseconds
             );
    
    char Directory[256];
    strncpy(Directory, FilePath, sizeof(Directory));
    
    PathRemoveFileSpecA(Directory);
    
    if(!PathFileExistsA(Directory))
    {
        if(CreateDirectory(Directory, null) == 0)
        {
            printf("Error %lu creating directory '%s'\n", GetLastError(), Directory);
            
            return false;
        }
    }
    
    GlobalProfilerCtx.LogFile = CreateFileA(
                                            FilePath,
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            null,
                                            OPEN_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            null
                                            );
    
    if(GlobalProfilerCtx.LogFile == INVALID_HANDLE_VALUE)
    {
        printf("Error opening log file: %lu\n", GetLastError());
        return false;
    }
    
    GlobalProfilerCtx.Thread = CreateThread(
                                            null, 0,
                                            (LPTHREAD_START_ROUTINE)profiler_LogWriterThread,
                                            null, 0,
                                            &GlobalProfilerCtx.LogThreadID);
    
    GlobalProfilerCtx.Running = true;
    
    return true;
}

// Queue a log entry (non-blocking)
void
profiler_QueueLogEntry(char *Message)
{
    log_entry *Entry = (log_entry *)malloc(sizeof(log_entry));
    if(Entry == null)
    {
        return;
    }
    
    strncpy(Entry->Message, Message, sizeof(Entry->Message) - 1);
    Entry->Next = null;
    
    WaitForSingleObject(GlobalProfilerCtx.Mutex, INFINITE);
    if(GlobalProfilerCtx.QueueTail == null)
    {
        GlobalProfilerCtx.QueueHead = Entry;
    }
    else
    {
        GlobalProfilerCtx.QueueTail->Next = Entry;
    }
    
    GlobalProfilerCtx.QueueTail = Entry;
    ReleaseMutex(GlobalProfilerCtx.Mutex);
    
    SetEvent(GlobalProfilerCtx.Event);
}

void
_profiler_Begin(char *Name)
{
    profile_data *Data = (profile_data *)malloc(sizeof(profile_data));
    if(Data == null)
    {
        return;
    }
    
    strncpy(Data->Name, Name, sizeof(Data->Name) - 1);
    QueryPerformanceFrequency(&Data->Frequency);
    QueryPerformanceCounter(&Data->Start);
    
    Data->Next = ThreadProfilerStack;
    ThreadProfilerStack = Data;
}

void
_profiler_End()
{
    DWORD ThreadID = GetCurrentThreadId();
    
    if(ThreadProfilerStack == null)
    {
        printf("ThreadProfilerCtx stack is empty on thread %lu", ThreadID);
        return;
    }
    
    profile_data *Data = ThreadProfilerStack;
    QueryPerformanceCounter(&Data->End);
    
    // Elapsed time in microseconds
    f64 ElapsedTime = ((f64)(Data->End.QuadPart - Data->Start.QuadPart) * 1e6) / Data->Frequency.QuadPart;
    
    char LogEntry[512];
    int Offset = snprintf(LogEntry, sizeof(LogEntry), "[TID %lu] %s;", ThreadID, Data->Name);
    
    profile_data *Current = Data->Next;
    
    while(Current && Offset < (int)sizeof(LogEntry) - 64)
    {
        Offset += snprintf(LogEntry + Offset, sizeof(LogEntry) - Offset, "%s;", Current->Name);
        Current = Current->Next;
    }
    
    // TODO(Farid): Check if (End - Start) and ElapsedTime are actually the same time.
    //snprintf(LogEntry, sizeof(LogEntry) - Offset, "%lld;%.3f\n", (s64)(Data->End.QuadPart - Data->Start.QuadPart), ElapsedTime);
    snprintf(LogEntry + Offset, sizeof(LogEntry) - Offset, "%.3f\n", ElapsedTime);
    profiler_QueueLogEntry(LogEntry);
    
    ThreadProfilerStack = Data->Next;
    free(Data);
}

void
profiler_Shutdown()
{
    GlobalProfilerCtx.Running = false;
    
    SetEvent(GlobalProfilerCtx.Event);
    WaitForSingleObject(GlobalProfilerCtx.Thread, INFINITE);
    
    CloseHandle(GlobalProfilerCtx.LogFile);
    CloseHandle(GlobalProfilerCtx.Mutex);
    CloseHandle(GlobalProfilerCtx.Event);
}
