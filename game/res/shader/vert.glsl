#version 450

layout(std140, set = 0, binding = 0) uniform CameraData {
    mat4 model;
    mat4 inv_model;
    mat4 view;
    mat4 inv_view;
    mat4 proj;
    mat4 inv_proj;
};

layout(location = 0) in vec3 input_position;
layout(location = 1) in vec3 input_normal;
layout(location = 2) in vec2 input_texture;

layout(location = 0) out vec3 output_position;
layout(location = 1) out vec3 output_normal;
layout(location = 2) out vec2 output_texture;

void main() {
    vec4 position = proj * view * model * vec4(input_position, 1.0);
    gl_Position = position;

    output_position = position.xyz / position.w;
    output_normal = mat3(transpose(inv_model)) * input_normal;
    output_texture = input_texture;
}
