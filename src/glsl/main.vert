#version 450 core

layout(set = 0, binding = 0) uniform Uniform {
    vec2 extent;
} u;

layout(location = 0) in vec2 in_vertex;


void main() {
    gl_Position = vec4(in_vertex / u.extent * 2.0 - vec2(1.0), 0.0, 1.0);
}