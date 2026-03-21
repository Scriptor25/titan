#version 450

layout(std140, binding = 2) uniform Data {
    vec4 colors[6];
};

layout(location = 0) in flat uvec2 input_texture;
layout(location = 1) in vec3 input_normal;
layout(location = 2) in flat vec3 input_color;

layout(location = 0) out vec4 output_color;

void main() {
    uint i = input_texture.x;

    float light = 0.1 + 0.9 * clamp(input_normal.g, 0.0, 1.0);

    output_color = vec4(light * input_color.rgb, 1.0);
}
