#pragma once

#include "gl.hpp"

GLuint create_program(const char* vertex_file, const char* fragment_file);

GLint uniform_location(GLuint program, const char* name);