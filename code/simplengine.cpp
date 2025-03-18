#include <windows.h>
#include <XInput.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define _PLATFORM_WIN32 1
#define ogl_ExitOnError 1

#include "simplengine.h"
#include "simplengine_opengl.h"
#include "simplengine_math.h"
#include "simplengine_utils.h"
#include "simplengine_profiler.h"

global b8 GlobalRunning;
global b8 GlobalIsFocused = true;

int keys[256] = {};
XINPUT_STATE controllerState;

LRESULT CALLBACK
WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
            PostQuitMessage(0);
        } break;
        
        case WM_DESTROY:
        {
        } break;
        
        case WM_SIZE:
        {
            
            int Width = LOWORD(LParam);
            int Height = HIWORD(LParam);
            
            if(Height <= 0)
            {
                Height = 1;
            }
            
            glViewport(0, 0, Width, Height);
        } break;
        
        case WM_SETFOCUS:
        {
            GlobalIsFocused = true;
        } break;
        
        case WM_KILLFOCUS:
        {
            GlobalIsFocused = false;
        } break;
        
        default:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        }
    }
    
    return 0;
}

void
ProcessInput()
{
    for (int i = 0; i < 256; i++)
    {
        keys[i] = (GetAsyncKeyState(i) & 0x8000) ? 1 : 0;
    }
    
    if (keys[VK_ESCAPE])
    {
        GlobalRunning = false;
    }
}

void
ProcessGamepad()
{
    ZeroMemory(&controllerState, sizeof(XINPUT_STATE));
    if (XInputGetState(0, &controllerState) == ERROR_SUCCESS)
    {
        // Controller is connected
        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
        
        if (pad->wButtons & XINPUT_GAMEPAD_A)
        {
            OutputDebugString("A button pressed\n");
        }
        if (pad->wButtons & XINPUT_GAMEPAD_B)
        {
            OutputDebugString("B button pressed\n");
        }
        if (pad->wButtons & XINPUT_GAMEPAD_X)
        {
            OutputDebugString("X button pressed\n");
        }
        if (pad->wButtons & XINPUT_GAMEPAD_Y)
        {
            OutputDebugString("Y button pressed\n");
        }
        
        // Handle thumbstick input
        if (pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            OutputDebugString("Left Stick Left\n");
        }
        if (pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            OutputDebugString("Left Stick Right\n");
        }
        if (pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            OutputDebugString("Left Stick Up\n");
        }
        if (pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            OutputDebugString("Left Stick Down\n");
        }
    }
}

struct basic_shader
{
    char *FilePath;
    FILETIME LastWriteTime;
    GLuint ID;
    GLenum Type;
};

b8
basic_shader_CompileFromFile(basic_shader *Shader, read_file *File, GLenum Type)
{
    profiler_Begin();
    
    Shader->FilePath = File->FilePath;
    Shader->LastWriteTime = File->LastWriteTime;
    Shader->Type = Type;
    
    GLuint Result = glCreateShader(Type);
    ogl_CheckError();
    Shader->ID = Result;
    
    glShaderSource(Shader->ID, 1, (const GLchar *const *)&File->Content, (GLint *)&File->Size);
    ogl_CheckError();
    
    glCompileShader(Shader->ID);
    ogl_CheckError();
    
    int Success;
    glGetShaderiv(Shader->ID, GL_COMPILE_STATUS, &Success);
    if(!Success)
    {
        char Log[512];
        glGetShaderInfoLog(Shader->ID, sizeof(Log), null, Log);
        printf("Shader compilation error @ '%s': %s\n", Shader->FilePath, Log);
        printf("%s\n", (char *)File->Content);
    }
    
    profiler_End();
    return Success;
}

b8
basic_shader_Compile(basic_shader *Shader, char *FilePath, GLenum Type)
{
    profiler_Begin();
    
    read_file File;
    if(!platform_ReadFile(&File, FilePath))
    {
        printf("error unu\n");
        return false;
    }
    
    b8 Result = basic_shader_CompileFromFile(Shader, &File, Type);;
    
    if(Result == 0)
    {
        debugger_placeholder();
    }
    
    
    free(File.Content);
    
    profiler_End();
    return Result;
}

b8
basic_shader_Reload(basic_shader *Shader)
{
    profiler_Begin();
    
    HANDLE HandleFile = CreateFile(Shader->FilePath,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   null,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   null);
    
    if(HandleFile != INVALID_HANDLE_VALUE)
    {
        FILETIME NewTime;
        
        if(!GetFileTime(HandleFile, null, null, &NewTime))
        {
            CloseHandle(HandleFile);
        }
        else
        {
            CloseHandle(HandleFile);
            
            b8 Changed = CompareFileTime(&NewTime, &Shader->LastWriteTime) > 0;
            
            if(Changed)
            {
                Shader->LastWriteTime = NewTime;
                GLuint OldShaderID = Shader->ID;
                
                printf("Reloading shaders (%s)... ", Shader->FilePath);
                
                if(!basic_shader_Compile(Shader, Shader->FilePath, Shader->Type))
                {
                    printf("Error occurred: Couldn't compile shader\n");
                    return false;
                }
                
                printf("Done\n");
                
                glDeleteShader(OldShaderID);
                
                return true;
            }
        }
    }
    
    profiler_End();
    return false;
}

struct material
{
    char *DiffuseTexture;
    v3 DiffuseColor;
};

struct mesh
{
    v3 *Vertices;
    v3 *TexCoords;
    v3 *Normals;
    u32 *Indices;
    u32 VertexCount;
    u32 IndexCount;
    material *Material;
};

struct scene
{
    mesh *Meshes;
    u32 MeshCount;
};

b8
LoadOBJ(scene *Scene, char *FilePath)
{
    profiler_Begin();
    
    read_file File;
    if(!platform_ReadFile(&File, FilePath))
    {
        return false;
    }
    
    
    *Scene = {};
    
    arena Arena_;
    arena_Alloc(&Arena_, arena_DEFAULT_SIZE);
    arena *Arena = &Arena_;
    
    char *Src = (char *)File.Content;
    
    loop
    {
        if(calc_offset(Src, File.Content) >= File.Size)
        {
            break;
        }
        
        if(SkipSpace(&Src))
        {
            continue;
        }
        
        switch(*Src)
        {
            case 'v':
            {
                if(strncmp(Src, "v ", 2) == 0)
                {
                    v3 V;
                    SScanf(&Src, "v %f %f %f", &V.X, &V.Y, &V.Z);
                    PushStructCopy(Arena, &V, v3);
                }
                else if(strncmp(Src, "vt ", 3) == 0)
                {
                    v2 V;
                    SScanf(&Src, "vt %f %f", &V.X, &V.Y);
                    PushStructCopy(Arena, &V, v2);
                }
                else if(strncmp(Src, "vn ", 3) == 0)
                {
                    v3 V;
                    SScanf(&Src, "vn %f %f %f", &V.X, &V.Y, &V.Z);
                    PushStructCopy(Arena, &V, v3);
                }
            } break;
            
            case 'f':
            {
                if(strncmp(Src, "f ", 2) == 0)
                {
                    u32 F0[3], F1[3], F2[3];
                    SScanf(
                           &Src,
                           "f %d/%d/%d %d/%d/%d %d/%d/%d",
                           &F0[0], &F0[1], &F0[2],
                           &F1[0], &F1[1], &F1[2], 
                           &F2[0], &F2[1], &F2[2]
                           );
                    PushStructCopy(Arena, &F0, typeof(F0));
                    PushStructCopy(Arena, &F1, typeof(F0));
                    PushStructCopy(Arena, &F2, typeof(F0));
                }
            } break;
            
            case 'm':
            {
                if(strncmp(Src, "mtllib ", 6) == 0)
                {
                    char Buffer[512];
                    SScanf(&Src, "mtllib %s", Buffer);
                    printf("Material lib '%s'\n", Buffer);
                }
            } break;
            
            case 'u':
            {
                if(strncmp(Src, "usemtl ", 7) == 0)
                {
                    char Buffer[512];
                    SScanf(&Src, "usemtl %s", Buffer);
                    printf("Using material '%s'\n", Buffer);
                }
            } break;
            
            case 'o':
            {
                if(strncmp(Src, "o ", 2) == 0)
                {
                    char Buffer[512];
                    SScanf(&Src, "o %s", Buffer);
                    printf("Object '%s'\n", Buffer);
                }
            } break;
            
            case 's':
            {
                if(strncmp(Src, "s ", 2) == 0)
                {
                    char Buffer[512];
                    SScanf(&Src, "s %s", Buffer);
                    printf("Shade: %s\n", Buffer);
                }
            } break;
            
            case '#':
            {
                loop
                {
                    if(Src[0] == '\0')
                    {
                        break;
                    }
                    
                    if(Src[0] == '\n')
                    {
                        ++Src;
                        break;
                    }
                    else
                    {
                        ++Src;
                    }
                }
                
            } break;
            
            default:
            {
                assert(false, "Unhandled case: '%*.s'\n", 50, Src);
            } break;
            
        }
        
    }
    
    platform_MemoryRelease(File.Content);
    
    profiler_End();
    return true;
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC;
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = LoadIcon(null, IDI_APPLICATION);
    WindowClass.hCursor = LoadCursor(null, IDC_ARROW);
    WindowClass.lpszClassName = "OpenGLWin32Class";
    
    if (RegisterClass(&WindowClass))
    {
        // Do nothing
    }
    else
    {
        return 0;
    }
    
    // Create Window
    HWND Window = CreateWindow("OpenGLWin32Class", "Simplengine Window",
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               800, 600,
                               null, null, hInstance, null);
    
    ShowWindow(Window, nCmdShow);
    
    
    // Console stuff
    {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {  // Attach to the existing console
            FILE *fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);  // Redirect stdout
            freopen_s(&fp, "CONOUT$", "w", stderr);  // Redirect stderr
            freopen_s(&fp, "CONIN$", "r", stdin);    // Redirect stdin
        }
        else
        {
            char Msg[512];
            
            snprintf(Msg, sizeof(Msg), "Error %lu: Failed to attach console.\n", GetLastError());
            
            //MessageBox(NULL, (LPCTSTR)Msg, TEXT("Error"), MB_OK | MB_ICONWARNING);
        }
    }
#if 0    
    {
        AllocConsole();  // Allocate a new console
        
        // Redirect standard input, output, and error streams to the new console
        FILE *fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);  // Redirect stdout
        freopen("CONIN$", "r", stdin);          // Redirect stdin
        freopen_s(&fp, "CONOUT$", "w", stderr); // Redirect stderr
        
        // Optional: Enable the console close button
        HWND consoleWindow = GetConsoleWindow();
        if (consoleWindow) {
            HMENU hMenu = GetSystemMenu(consoleWindow, FALSE);
            if (hMenu) {
                EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
            }
        }
    }
#endif
    
    profiler_Init();
    
    scene Scene;
    {
        b8 Result = LoadOBJ(&Scene, "..\\assets\\models\\backpack\\backpack.obj");
    }
    {
        b8 Result = LoadOBJ(&Scene, "..\\assets\\models\\rock\\rock.obj");
    }
    
    // Initialize OpenGL
    HDC DeviceContext;
    HGLRC GLReferenceContext;
    ogl_Enable(Window, &DeviceContext, &GLReferenceContext);
    
    basic_shader VertexShader, FragmentShader;
    {
        b8 Result = basic_shader_Compile(&VertexShader, "..\\code\\shaders\\vertex.glsl", GL_VERTEX_SHADER);
        assert(Result, "Shader compilation failed.");
    }
    {
        b8 Result = basic_shader_Compile(&FragmentShader, "..\\code\\shaders\\fragment.glsl", GL_FRAGMENT_SHADER);
        assert(Result, "Shader compilation failed.");
    }
    
    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader.ID);
    glAttachShader(ShaderProgram, FragmentShader.ID);
    glLinkProgram(ShaderProgram);
    
    // Define a diagonal line from (-1,-1) to (1,1)
    GLfloat Vertices[] = {
        //-1.0f, -1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f
    };
    
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    
    double LastTime = GetTime();
    int FrameCount = 0;
    
    
    GlobalRunning = true;
    
    
    // Main loop
    while (GlobalRunning)
    {
        // Handing messages
        {
            MSG Message;
            while (PeekMessage(&Message, null, 0, 0, PM_REMOVE))
            {
                if (Message.message == WM_QUIT)
                {
                    GlobalRunning = false;
                }
                else
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
            }
            
            // Input processing
            if(GlobalIsFocused)
            {
                ProcessInput();
                ProcessGamepad();
            }
        }
        
        // Rendering
        static float R = 0.0f;
        glClearColor(R, 0.0f, 1.0f - R, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        if(basic_shader_Reload(&VertexShader) || basic_shader_Reload(&FragmentShader))
        {
            glDeleteProgram(ShaderProgram);
            
            ShaderProgram = glCreateProgram();
            glAttachShader(ShaderProgram, VertexShader.ID);
            glAttachShader(ShaderProgram, FragmentShader.ID);
            glLinkProgram(ShaderProgram);
            
        }
        
        glUseProgram(ShaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 2);
        
        SwapBuffers(DeviceContext);
        
        R += 0.01f;
        if (R > 1.0f)
        {
            R = 0.0f;
        }
        
        // FPS & frame timing
        double CurrentTime = GetTime();
        double DeltaTime = CurrentTime - LastTime;
        LastTime = CurrentTime;
        FrameCount++;
        
        char Buffer[128];
        snprintf(Buffer, sizeof(Buffer), "Simplengine Window - %.2f fps, %.2f mspf", 1.0 / DeltaTime, DeltaTime * 1000);
        SetWindowText(Window, Buffer);
        
        Sleep(1);  // Prevent excessive CPU usage
    }
    
    // Cleanup
    
    wglMakeCurrent(null, null);
    wglDeleteContext(GLReferenceContext);
    ReleaseDC(Window, DeviceContext);
    
    profiler_Shutdown();
    
    //return msg.wParam;
}