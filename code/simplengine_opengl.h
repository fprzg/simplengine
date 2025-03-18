#pragma once

#include <GL/gl.h>
#include <GL/glext.h>



//
// OPENGL FUNCTIONS
//

PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLDELETEPROGRAMPROC glDeleteProgram;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLDELETESHADERPROC glDeleteShader;

PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC  glGetShaderInfoLog;



//
// ASSISTING FUNCTIONS
//

#define ogl_CheckError() _ogl_CheckError((char *)__FUNCTION__, (char *)__FILE__, __LINE__)

void
_ogl_CheckError(char *Function, char *File, int Line)
{
    
    loop
    {
        GLuint Error = glGetError();
        if(Error == GL_NO_ERROR)
        {
            break;
        }
        
        switch(Error)
        {
            case GL_INVALID_ENUM:
            {
                printf("Invalid enum");
            } break;
            
            case GL_INVALID_VALUE:
            {
                printf("Invalid value");
            } break;
            
            case GL_INVALID_OPERATION:
            {
                printf("Invalid operation");
            } break;
            
            case GL_INVALID_FRAMEBUFFER_OPERATION:
            {
                printf("Invalid framebuffer operation");
            } break;
            
            case GL_OUT_OF_MEMORY:
            {
                printf("Out of memory");
            } break;
            
            //case GL_CONTEXT_LOST: break;
            //case GL_TABLE_TOO_LARGE: break;
            
            default:
            {
                printf("Unknown error code: %d", Error);
            }
            
        }
        
        printf(" encountered at function %s (%s:%d)\n", Function, File, Line);
        
#if ogl_ExitOnError
        abort_program();
#endif
    }
}



//
// FUNCTION DECLARATIONS
//

b8
ogl_Enable(HWND hWnd, HDC *hDC, HGLRC *hRC);



//
// IMPLEMENTATION
//

#if defined(_PLATFORM_WIN32)
#include "win32_simplengine_opengl.cpp"
#endif