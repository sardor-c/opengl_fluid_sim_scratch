#include "gl.hpp"
#include "shader.hpp"
#include "sim2d.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <vector>

struct Color {
    float r, g, b;
};

struct Controls {
    bool paused = false;
    bool step_once = false;
    bool reset = false;
};

static void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW xatosi %d: %s\n", code, description);
}

static void on_key(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) {
        return;
    }

    Controls* controls = static_cast<Controls*>(glfwGetWindowUserPointer(window));
    switch (key) {
    case GLFW_KEY_SPACE: controls->paused = !controls->paused; break;
    case GLFW_KEY_RIGHT: controls->step_once = true; break;
    case GLFW_KEY_R: controls->reset = true; break;
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    default: break;
    }
}

static const char* gl_string(GLenum name) {
    return reinterpret_cast<const char*>(glGetString(name));
}

static std::vector<Color> make_colors(const std::vector<Vec2>& positions, const SpawnSettings& spawn) {
    std::vector<Color> colors;
    colors.reserve(positions.size());

    for (const Vec2& p : positions) {
        const float tx = (p.x - spawn.centre.x) / spawn.size.x + 0.5f;
        const float ty = (p.y - spawn.centre.y) / spawn.size.y + 0.5f;
        colors.push_back({ 0.2f + 0.8f * tx, 0.2f + 0.8f * ty, 1.0f - 0.7f * tx });
    }
    return colors;
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

    Controls controls;
    glfwSetWindowUserPointer(window, &controls);
    glfwSetKeyCallback(window, on_key);

    GLuint program = create_program("particle2d.vert", "particle2d.frag");
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLint loc_world_to_clip = uniform_location(program, "u_world_to_clip");
    const GLint loc_diameter = uniform_location(program, "u_diameter");

    const SimSettings settings;
    const SpawnSettings spawn;
    const Simulation initial = spawn_particles(spawn);
    Simulation sim = initial;

    const std::vector<Color> colors = make_colors(initial.positions, spawn);
    const int particle_count = static_cast<int>(sim.positions.size());

    GLuint vao = 0;
    GLuint position_vbo = 0;
    GLuint color_vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &position_vbo);
    glGenBuffers(1, &color_vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), nullptr);
    glVertexAttribDivisor(0, 1);

    glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(Color), colors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Color), nullptr);
    glVertexAttribDivisor(1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_check("sozlash");

    double last_time = glfwGetTime();
    double fps_time = last_time;
    int fps_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double now = glfwGetTime();
        const float frame_time = static_cast<float>(now - last_time);
        last_time = now;

        if (controls.reset) {
            sim = initial;
            controls.reset = false;
            controls.paused = true;
        }

        if (!controls.paused || controls.step_once) {
            const float dt = std::min(frame_time, settings.max_frame_time) / settings.iterations_per_frame;
            for (int i = 0; i < settings.iterations_per_frame; i++) {
                step_simulation(sim, settings, dt);
            }
        }
        controls.step_once = false;

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);

        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const float aspect = static_cast<float>(std::max(fb_width, 1)) / std::max(fb_height, 1);
        float half_height = 5.0f;
        float half_width = half_height * aspect;

        const float min_half_width = settings.bounds_size.x * 0.5f * 1.02f;
        if (half_width < min_half_width) {
            half_width = min_half_width;
            half_height = half_width / aspect;
        }

        glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
        glBufferData(GL_ARRAY_BUFFER, sim.positions.size() * sizeof(Vec2), sim.positions.data(), GL_STREAM_DRAW);

        glUseProgram(program);
        glUniform2f(loc_world_to_clip, 1.0f / half_width, 1.0f / half_height);
        glUniform1f(loc_diameter, 0.08f);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particle_count);
        gl_check("kadr");

        glfwSwapBuffers(window);

        fps_frames++;
        if (now - fps_time >= 0.5) {
            int win_width = 0;
            int win_height = 0;
            glfwGetWindowSize(window, &win_width, &win_height);

            char title[128];
            std::snprintf(title, sizeof(title), "Fluid Sim | %.0f FPS | %d zarracha | oyna %dx%d | framebuffer %dx%d%s",
                fps_frames / (now - fps_time), particle_count, win_width, win_height, fb_width, fb_height,
                controls.paused ? " | pauza" : "");
            glfwSetWindowTitle(window, title);

            fps_time = now;
            fps_frames = 0;
        }
    }

    glDeleteBuffers(1, &position_vbo);
    glDeleteBuffers(1, &color_vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
