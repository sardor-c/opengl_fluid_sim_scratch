#pragma once

#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall
#endif
#include <GL/glcorearb.h>

#define GL_FUNCTIONS(X)                                                \
    X(PFNGLGETERRORPROC, glGetError)                                   \
    X(PFNGLGETSTRINGPROC, glGetString)                                 \
    X(PFNGLVIEWPORTPROC, glViewport)                                   \
    X(PFNGLCLEARCOLORPROC, glClearColor)                               \
    X(PFNGLCLEARPROC, glClear)                                         \
    X(PFNGLCREATESHADERPROC, glCreateShader)                           \
    X(PFNGLSHADERSOURCEPROC, glShaderSource)                           \
    X(PFNGLCOMPILESHADERPROC, glCompileShader)                         \
    X(PFNGLGETSHADERIVPROC, glGetShaderiv)                             \
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                   \
    X(PFNGLDELETESHADERPROC, glDeleteShader)                           \
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                         \
    X(PFNGLATTACHSHADERPROC, glAttachShader)                           \
    X(PFNGLLINKPROGRAMPROC, glLinkProgram)                             \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                           \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                 \
    X(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                         \
    X(PFNGLUSEPROGRAMPROC, glUseProgram)                               \
    X(PFNGLGENBUFFERSPROC, glGenBuffers)                               \
    X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                         \
    X(PFNGLBINDBUFFERPROC, glBindBuffer)                               \
    X(PFNGLBUFFERDATAPROC, glBufferData)                               \
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                     \
    X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)               \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                     \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)     \
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)             \
    X(PFNGLDRAWARRAYSPROC, glDrawArrays)                               \
    X(PFNGLENABLEPROC, glEnable)                                       \
    X(PFNGLBLENDFUNCPROC, glBlendFunc)                                 \
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)               \
    X(PFNGLUNIFORM1FPROC, glUniform1f)                                 \
    X(PFNGLUNIFORM2FPROC, glUniform2f)                                 \
    X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor)             \
    X(PFNGLPOLYGONMODEPROC, glPolygonMode)                             \
    X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced)


#define X(type, name) extern type name;
GL_FUNCTIONS(X)
#undef X

bool gl_load();
void gl_check(const char* where);
