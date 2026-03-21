#version 450

layout(std140, binding = 0) uniform CameraBuffer {
    mat4 view_proj;
    mat4 model_view_proj;
    mat4 model;
    vec4 color;
    vec4 pad1;
    vec4 pad2;
    vec4 pad3;
};

layout(std140, binding = 1) uniform NormalBuffer {
    vec4 normals[6];
};

layout(location = 0) in vec4 input_position;

layout(location = 0) out flat uvec2 output_texture;
layout(location = 1) out flat vec3 output_normal;
layout(location = 2) out flat vec3 output_color;

void main() {
    gl_Position = model_view_proj * input_position;

    int face = gl_VertexIndex / 6;

    output_texture = uvec2(face, 0);
    output_normal = (model * normals[face]).xyz;
    output_color = color.rgb;
}
