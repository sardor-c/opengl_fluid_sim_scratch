#version 410 core

in vec2 v_local;
in vec3 v_color;

layout(location = 0) out vec4 out_color;

void main() {
    float dist = length(v_local);
    float pixel = max(fwidth(dist), 0.0001);
    float alpha = 1.0 - smoothstep(1.0 - pixel, 1.0, dist);
    out_color = vec4(v_color, alpha);
}