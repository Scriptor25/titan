#version 450

layout(push_constant) uniform CameraData {
    mat4 screen_mat;
    mat4 normal_mat;
};

layout(location = 0) in vec3 input_position;
layout(location = 1) in vec3 input_normal;
layout(location = 2) in vec2 input_texture;

layout(location = 0) out vec3 output_position;
layout(location = 1) out vec3 output_normal;
layout(location = 2) out vec2 output_texture;

void main() {
    vec4 position = screen_mat * vec4(input_position, 1.0);
    gl_Position = position;

    output_position = position.xyz / position.w;
    output_normal = mat3(normal_mat) * input_normal;
    output_texture = input_texture;
}
