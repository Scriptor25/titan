#version 450

layout(location = 0) in vec3 input_position;
layout(location = 1) in vec3 input_normal;
layout(location = 2) in vec2 input_texture;

layout(location = 0) out vec4 output_color;

const vec3 LIGHT_DIR = normalize(vec3(0.0, 1.0, 0.0));

void main() {
    vec3 normal = normalize(input_normal);
    float light = 0.1 + 0.9 * clamp(dot(normal, LIGHT_DIR), 0.0, 1.0);

    output_color = vec4(vec3(light), 1.0);
}
