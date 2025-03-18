#pragma once



//
// MACROS
//

#define Kibibytes(value) ((value) * 1024ll)
#define Mebibytes(value) (Kibibytes(value) * 1024ll)
#define Gibibytes(value) (Mebibytes(value) * 1024ll)
#define Tebibytes(value) (Gibibytes(value) * 1024ll)
#define Pebibytes(value) (Tebibytes(value) * 1024ll)
#define Exbibytes(value) (Pebibytes(value) * 1024ll)
#define Zebibytes(value) (Exbibytes(value) * 1024ll)
#define Yobibytes(value) (Zebibytes(value) * 1024ll)

#define local static
#define global static
#define function static

#define loop for(;;)

#define array_count(array) (sizeof(array)/sizeof((array)[0]))
#define typeof decltype
#define stringify(token) #token

#define calc_offset(left, right) ((uintptr)(left) - (uintptr)(right))
#define indexed_addressing(base, offset, index, stride) to_voidptr( \
to_uintptr(base) + \
to_uintptr(offset) + \
to_uintptr(index) * to_uintptr(stride) \
)

#define debugger_placeholder() do { int a = 1; } while(false)

#ifndef null
#define null nullptr
#endif

#define abort_program() do { *(int *)0 = 1; } while(false)

//#define static_assert _Static_assert
#define assert(expr, ...) do { \
if(!(expr)) \
{ \
printf("Assertion failed at function %s (%s:%d): ", __FUNCTION__,__FILE__,  __LINE__);\
printf(__VA_ARGS__); \
printf("\n"); \
abort_program(); \
} \
} while(false)



//
// BASIC TYPES
//

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef unsigned char b8;
typedef unsigned int b32;

typedef u64 uintptr; // depends on the platform



static_assert(sizeof(s8) == 1, "s8 must have a size of 1.");
static_assert(sizeof(s16) == 2, "s16 must have a size of 2.");
static_assert(sizeof(s32) == 4, "s32 must have a size of 4.");
static_assert(sizeof(s64) == 8, "s64 must have a size of 8.");

static_assert(sizeof(u8) == 1, "u8 must have a size of 1.");
static_assert(sizeof(u16) == 2, "u16 must have a size of 2.");
static_assert(sizeof(u32) == 4, "u32 must have a size of 4.");
static_assert(sizeof(u64) == 8, "u64 must have a size of 8.");

static_assert(sizeof(f32) == 4, "f32 must have a size of 4.");
static_assert(sizeof(f64) == 8, "f64 must have a size of 8.");

static_assert(sizeof(b8) == 1, "b8 must have a size of 1.");
static_assert(sizeof(b32) == 4, "b32 must have a size of 4.");

static_assert(sizeof(uintptr) == sizeof(void *), "uintptr must have the same size as void *.");



#define to_u32(value) ((u32)(value))
#define to_u64(value) ((u64)(value))

#define to_f32(value) ((f32)(value))
#define to_f64(value) ((f64)(value))

#define to_u8ptr(value) ((u8 *)(value))
#define to_voidptr(value) ((void *)(value))
#define to_uintptr(value) ((uintptr)(value))

//
// HANDLES
//

#define uhandle_INVALID 0xffffffff

typedef u32 uhandle;
typedef u64 u64handle;



//
// PLATFORM MEMORY
//

inline void *
platform_MemoryAlloc(u64 Size);

inline void
platform_MemoryRelease(void *Ptr);

inline void
platform_MemoryZero(void *Ptr, u64 Size);

inline void *
platform_MemoryCopy(void *Dst, void *Src, u64 Count);

inline void *
platform_MemoryMove(void *Dst, void *Src, u64 Count);



//
// PLATFORM FILES
//

struct read_file
{
    char *FilePath;
    void *Content;
    u64 Size;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
};

b8
platform_ReadFileFromHandle(read_file *File, HANDLE HandleFile);

b8
platform_ReadFile(read_file *File, char* FilePath);



//
// PLATFORM OPENGL
//

b8
platform_EnableOpenGL(HWND hWnd, HDC *hDC, HGLRC *hRC);



//
// ARENAS
//

struct arena
{
    void *Memory;
    u64 Offset;
    u64 Size;
    
    //struct arena *Next;
};

#define arena_DEFAULT_SIZE Mebibytes(4)

b8
arena_Alloc(arena *Arena, u64 Size)
{
    *Arena = {};
    
    Arena->Memory = malloc(Size);
    if(Arena->Memory == null)
    {
        return false;
    }
    
    Arena->Offset = 0;
    Arena->Size = Size;
    
    return true;
}

void
arena_Release(arena *Arena)
{
    free(Arena->Memory);
    platform_MemoryZero(Arena, sizeof(Arena));
}

void
arena_Set(arena *Arena, u64 Size)
{
    assert(Arena->Size >= Size, "Error: Cannot set an offset bigger than the arena allocation size.");
}

void *
arena_Get(arena *Arena)
{
    void *Ptr = (char *)Arena->Memory + Arena->Offset;
    return Ptr;
}

void *
arena_Push(arena *Arena, u64 Size)
{
    assert(Arena->Size - Arena->Offset >= Size, "Error: Insufficient storage on arena.");
    
    void *Ptr = (char *)Arena->Memory + Arena->Offset;
    Arena->Offset += Size;
    
    return Ptr;
}

void *
arena_PushZero(arena *Arena, u64 Size)
{
    void *Ptr = arena_Push(Arena, Size);
    platform_MemoryZero(Ptr, Size);
    return Ptr;
}

void *
arena_PushCopy(arena *Arena, void *Src, u64 Size)
{
    void *Ptr = arena_Push(Arena, Size);
    return platform_MemoryCopy(Ptr, Src, Size);;
}

void
arena_Pop(arena *Arena, u64 Size)
{
    assert(Arena->Size - Arena->Offset >= Size, "Error: Cannot pop a size bigger than the offset.");
    
    Arena->Offset -= Size;
}

#define PushArray(arena, size, type) (type *)arena_Push(arena, (size)*sizeof(type))
#define PushStruct(arena, type) PushArray(arena, 1, type)
#define PushArrayZero(arena, size, type) (type *)arena_PushZero(arena, (size)*sizeof(type))
#define PushStructZero(arena, type) PushArrayZero(arena, 1, type)
#define PushArrayCopy(arena, src, size, type) (type *)arena_PushCopy(arena, src, (size)*sizeof(type))
#define PushStructCopy(arena, src, type) PushArrayCopy(arena, src, 1, type)
//#define PushArrayCopy(arena, src, size) (typeof(src)) arena_PushCopy(arena, src, (size)*sizeof(typeof(src)[0]))
//#define PushStructCopy(arena, src) PushArrayCopy(arena, src, 1)



//
// CSTR
//

u32
cstr_Copy(char *Src, char *Dst)
{
    u32 Count = 0;
    
    while(*Src)
    {
        Dst[Count] = Src[Count];
        
        ++Count;
    }
    
    return Count;
}

u32
cstr_NCopy(char *Src, char *Dst, u32 Size)
{
    u32 Count = 0;
    
    while(*Src && Size > Count)
    {
        Dst[Count] = Src[Count];
        
        ++Count;
    }
    
    return Count;
}



//
// STRINGS
//

struct string8_node
{
    uhandle Next;
    u8 Content[60];
};

struct string8
{
    uhandle Head;
    u32 Length;
};

static_assert(sizeof(string8_node) == 64, "str8 should have size 64");

struct string8_node_pool
{
    string8_node *Pool;
    u32 Capacity;
    u32 Size;
    
    // Free nodes
    uhandle Head;
    uhandle Tail;
};

global string8_node_pool GlobalString8Pool;

b8
string8_node_pool_Init(u32 Capacity)
{
    GlobalString8Pool.Pool = (string8_node *)malloc(sizeof(string8) * Capacity);
    GlobalString8Pool.Capacity = Capacity;
    GlobalString8Pool.Size = 0;
    GlobalString8Pool.Head = 0;
    
    for(int I = 0; I < Capacity - 1; ++I)
    {
        GlobalString8Pool.Pool[I].Next = I + 1;
    }
    
    GlobalString8Pool.Pool[Capacity - 1].Next = uhandle_INVALID;
    
    return true;
}

void
string8_node_pool_Shutdown(void)
{
    platform_MemoryRelease(GlobalString8Pool.Pool);
    GlobalString8Pool = {};
}

inline string8_node *
string8_node_pool_Get(uhandle Handle)
{
    string8_node *Node = null;
    
    if(Handle != uhandle_INVALID)
    {
        Node = (string8_node *)indexed_addressing(GlobalString8Pool.Pool, 0, Handle, sizeof(string8_node));
    }
    
    return Node;
}

uhandle
string8_node_pool_Borrow(void)
{
    uhandle Handle = GlobalString8Pool.Head;
    if(Handle != uhandle_INVALID)
    {
        string8_node *Node = string8_node_pool_Get(Handle);
        GlobalString8Pool.Head = Node->Next;
        Node->Next = uhandle_INVALID;
        platform_MemoryZero(Node->Content, sizeof(Node->Content));
    }
    
    return Handle;
}

void
string8_node_pool_Return(uhandle Handle)
{
    assert(false, "TODO: Implement %s", __FUNCTION__);
}

string8
string8_Copy(char *Src)
{
    string8 String8 = {};
    String8.Head = string8_node_pool_Borrow();
    
    string8_node *Node = string8_node_pool_Get(String8.Head);
    
    loop
    {
        u32 BytesCopied = 0;
        loop
        {
            if(BytesCopied < sizeof(Node->Content))
            {
                Node->Content[BytesCopied] = Src[BytesCopied];
                
                if(Src[BytesCopied] == '\0')
                {
                    break;
                }
                
                ++BytesCopied;
            }
            else
            {
                break;
            }
            
        }
        
        String8.Length += BytesCopied;
        Src += BytesCopied;
        
        if(BytesCopied == sizeof(Node->Content))
        {
            string8_node *PrevNode = Node;
            PrevNode->Next = string8_node_pool_Borrow();
            Node = string8_node_pool_Get(PrevNode->Next);
        }
        else
        {
            break;
        }
        
    }
    
    return String8;
}



//
// PLATFORM DETECTION
//

// TODO: Implement platform detection
#if defined(_PLATFORM_WIN32)
#include "win32_simplengine_platform.cpp"
#endif