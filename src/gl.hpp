#pragma once

#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall
#endif
#include <GL/glcorearb.h>

#define GL_FUNCTIONS(X)                  \
    X(PFNGLGETERRORPROC, glGetError)     \
    X(PFNGLGETSTRINGPROC, glGetString)   \
    X(PFNGLVIEWPORTPROC, glViewport)     \
    X(PFNGLCLEARCOLORPROC, glClearColor) \
    X(PFNGLCLEARPROC, glClear)

#define X(type, name) extern type name;
GL_FUNCTIONS(X)
#undef X

bool gl_load();
void gl_check(const char* where);
