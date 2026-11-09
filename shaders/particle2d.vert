#version 410 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

uniform vec2 u_world_to_clip;
uniform float u_diameter;

out vec2 v_local;
out vec3 v_color;

const vec2 CORNERS[6] = vec2[6](
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0)
);

void main(){
    vec2 corner = CORNERS[gl_VertexID];
    vec2 world = in_position + corner * 0.5 * u_diameter;
    gl_Position = vec4(world * u_world_to_clip, 0.0, 1.0);

    v_local = corner;
    v_color = in_color;
}


