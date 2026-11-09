#include "gl.hpp"
#include "shader.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>

struct Particle {
    float x, y;
    float r, g, b;
};

static void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW xatosi %d: %s\n", code, description);
}

static const char* gl_string(GLenum name) {
    return reinterpret_cast<const char*>(glGetString(name));
}

static std::vector<Particle> make_grid(int nx, int ny, float width, float height) {
    std::vector<Particle> particles;
    particles.reserve(nx * ny);

    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            const float tx = i / (nx - 1.0f);
            const float ty = j / (ny - 1.0f);

            Particle p;
            p.x = (tx - 0.5f) * width;
            p.y = (ty - 0.5f) * height;
            p.r = 0.2f + 0.8f * tx;
            p.g = 0.2f + 0.8f * ty;
            p.b = 1.0f - 0.7f * tx;
            particles.push_back(p);
        }
    }
    return particles;
}

int main() {
    glfwSetErrorCallback(on_glfw_error);

    if (!glfwInit()) {
        return 1;
    }
    std::printf("GLFW:   %s\n", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fluid Sim", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    if (!gl_load()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("OpenGL: %s\n", gl_string(GL_VERSION));
    std::printf("GLSL:   %s\n", gl_string(GL_SHADING_LANGUAGE_VERSION));
    std::printf("GPU:    %s\n", gl_string(GL_RENDERER));

    glfwSwapInterval(1);

    GLuint program = create_program("particle2d.vert", "particle2d.frag");
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLint loc_world_to_clip = uniform_location(program, "u_world_to_clip");
    const GLint loc_diameter = uniform_location(program, "u_diameter");

    const std::vector<Particle> particles = make_grid(81, 56, 6.42f, 4.39f);
    const int particle_count = static_cast<int>(particles.size());

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle),
        reinterpret_cast<const void*>(offsetof(Particle, x)));
    glVertexAttribDivisor(0, 1);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle),
        reinterpret_cast<const void*>(offsetof(Particle, r)));
    glVertexAttribDivisor(1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_check("sozlash");

    double fps_time = glfwGetTime();
    int fps_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);

        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const float half_height = 5.0f;
        const float aspect = static_cast<float>(std::max(fb_width, 1)) / std::max(fb_height, 1);
        const float half_width = half_height * aspect;

        glUseProgram(program);
        glUniform2f(loc_world_to_clip, 1.0f / half_width, 1.0f / half_height);
        glUniform1f(loc_diameter, 0.08f);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particle_count);
        gl_check("kadr");

        glfwSwapBuffers(window);

        fps_frames++;
        const double now = glfwGetTime();
        if (now - fps_time >= 0.5) {
            int win_width = 0;
            int win_height = 0;
            glfwGetWindowSize(window, &win_width, &win_height);

            char title[128];
            std::snprintf(title, sizeof(title), "Fluid Sim | %.0f FPS | %d zarracha | oyna %dx%d | framebuffer %dx%d",
                fps_frames / (now - fps_time), particle_count, win_width, win_height, fb_width, fb_height);
            glfwSetWindowTitle(window, title);

            fps_time = now;
            fps_frames = 0;
        }
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
