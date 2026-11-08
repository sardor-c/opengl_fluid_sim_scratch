#include "gl.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>

#define X(type, name) type name = nullptr;
GL_FUNCTIONS(X)
#undef X

bool gl_load() {
#define X(type, name)                                                     \
    name = reinterpret_cast<type>(glfwGetProcAddress(#name));             \
    if (!name) {                                                          \
        std::fprintf(stderr, "OpenGL funksiyasi topilmadi: %s\n", #name); \
        return false;                                                     \
    }
    GL_FUNCTIONS(X)
#undef X
        return true;
}

void gl_check(const char* where) {
    while (GLenum error = glGetError()) {
        const char* name = "noma'lum";
        switch (error) {
        case GL_INVALID_ENUM: name = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: name = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: name = "GL_INVALID_OPERATION"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: name = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_OUT_OF_MEMORY: name = "GL_OUT_OF_MEMORY"; break;
        }
        std::fprintf(stderr, "OpenGL xatosi (%s): %s\n", where, name);
    }
}
