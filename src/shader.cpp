#include "shader.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>

static GLuint compile_shader(GLenum stage, const char* file) {
    const std::string path = std::string(SHADER_DIR) + "/" + file;
    std::ifstream stream(path);
    if (!stream) {
        std::fprintf(stderr, "Shader fayli ochilmadi: %s\n", path.c_str());
        return 0;
    }

    std::stringstream buffer;
    buffer << stream.rdbuf();
    const std::string source = buffer.str();
    const char* text = source.c_str();

    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "%s komilyatsiya xatosi: \n%s\n", file, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint create_program(const char* vertex_file, const char* fragment_file) {
    GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_file);
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_file);
    if (!vertex || !fragment) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Link xatosi (%s, %s): \n%s\n", vertex_file, fragment_file, log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

GLint uniform_location(GLuint program, const char* name) {
    const GLint location = glGetUniformLocation(program, name);
    if (location == -1) {
        std::fprintf(stderr, "Uniform topilmadi: %s\n", name);
    }
    return location;
}
