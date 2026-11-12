#include "sim2d.hpp"

#include <cmath>
#include <cstddef>
#include <random>


Simulation spawn_particles(const SpawnSettings& spawn) {
    const float per_unit = std::sqrt(spawn.density);
    const int nx = static_cast<int>(std::ceil(spawn.size.x * per_unit));
    const int ny = static_cast<int>(std::ceil(spawn.size.y * per_unit));

    Simulation sim;
    sim.positions.reserve(nx * ny);
    sim.velocities.reserve(nx * ny);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    const float two_pi = 6.2831853f;

    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            const float tx = i / (nx - 1.0f);
            const float ty = j / (ny - 1.0f);

            const float angle = random01(rng) * two_pi;
            const float jitter = spawn.jitter * (random01(rng) - 0.5f);

            Vec2 position;
            position.x = spawn.centre.x + (tx - 0.5f) * spawn.size.x + std::cos(angle) * jitter;
            position.y = spawn.centre.y + (ty - 0.5f) * spawn.size.y + std::sin(angle) * jitter;

            sim.positions.push_back(position);
            sim.velocities.push_back(spawn.initial_velocity);
        }
    }
    return sim;
}

static void apply_gravity(Simulation& sim, const SimSettings& settings, float dt) {
    const Vec2 acceleration = { 0.0f, settings.gravity };

    for (Vec2& velocity : sim.velocities) {
        velocity += acceleration * dt;
    }
}

static void resolve_collision(Vec2& position, Vec2& velocity, const SimSettings& settings) {
    const float half_width = settings.bounds_size.x * 0.5f;
    const float half_height = settings.bounds_size.y * 0.5f;

    if (std::fabs(position.x) >= half_width) {
        position.x = std::copysign(half_width, position.x);
        velocity.x *= -settings.collision_damping;
    }

    if (std::fabs(position.y) >= half_height) {
        position.y = std::copysign(half_height, position.y);
        velocity.y *= -settings.collision_damping;
    }
}

static void update_position(Simulation& sim, const SimSettings& settings, float dt) {
    for (std::size_t i = 0; i < sim.positions.size(); i++) {
        sim.positions[i] += sim.velocities[i] * dt;
        resolve_collision(sim.positions[i], sim.velocities[i], settings);
    }
}

void step_simulation(Simulation& sim, const SimSettings& settings, float dt) {
    apply_gravity(sim, settings, dt);
    update_position(sim, settings, dt);
}
