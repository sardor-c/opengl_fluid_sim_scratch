#pragma once

#include <vector>

struct Vec2 {
    float x, y;
};

inline Vec2 operator*(Vec2 v, float s){
    return {v.x * s, v.y * s};
}

inline Vec2 operator+=(Vec2& a, Vec2 b){
    a.x += b.x;
    a.y += b.y;
    return a;
}

struct SimSettings{
    float gravity = -12.0f;
    float collision_damping = 0.95f;
    Vec2 bounds_size = {17.1f, 9.3f};
    float max_frame_time = 1.0f / 60.0f;
    int iterations_per_frame = 3;
};

struct SpawnSettings{
    Vec2 centre = {0.0f, 0.66f};
    Vec2 size = {6.42f, 4.39f};
    float density = 159.0f;
    float jitter = 0.03f;
    Vec2 initial_velocity = {0.0f, 0.0f};
};


struct Simulation{
    std::vector<Vec2> positions;
    std::vector<Vec2> velocities;
};

Simulation spawn_particles(const SpawnSettings& spawn);
void step_simulation(Simulation& sim, const SimSettings& settings, float dt);
 