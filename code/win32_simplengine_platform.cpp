
#include <shlwapi.h>

//
// PLATFORM MEMORY
//

inline void *
platform_MemoryAlloc(u64 Size)
{
    return malloc(Size);
}

inline void
platform_MemoryFree(void *Ptr)
{
    free(Ptr);
}

inline void
platform_MemoryZero(void *Ptr, u64 Size)
{
    ZeroMemory(Ptr, Size);
}

inline void *
platform_MemoryCopy(void *Dst, void *Src, u64 Count)
{
    return memcpy(Dst, Src, Count);
}

inline void *
platform_MemoryMove(void *Dst, void *Src, u64 Count)
{
    return memmove(Dst, Src, Count);
}



//
// PLATFORM FILES
//

b8
platform_ReadFileFromHandle(read_file *File, HANDLE HandleFile)
{
    *File = {};
    
    if(HandleFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    
    LARGE_INTEGER FileSize;
    if(!GetFileSizeEx(HandleFile, &FileSize))
    {
        return false;
    }
    
    File->Size = FileSize.QuadPart;
    File->Content = (char *)platform_MemoryAlloc(File->Size);
    
    DWORD BytesRead;
    if(!ReadFile(HandleFile, File->Content, (DWORD)File->Size, &BytesRead, null))
    {
        platform_MemoryFree(File->Content);
        *File = {};
        return false;
    }
    
    
    if(!GetFileTime(HandleFile, &File->CreationTime, &File->LastAccessTime, &File->LastWriteTime))
    {
        // We couldn't read the creation, last access and last write times.
    }
    
    return true;
}

b8
platform_ReadFile(read_file *File, char* FilePath)
{
    HANDLE HandleFile = CreateFile(FilePath,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   null,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   null);
    
    b8 Result = platform_ReadFileFromHandle(File, HandleFile);
    CloseHandle(HandleFile);
    
    File->FilePath = FilePath;
    
    return Result;
}
