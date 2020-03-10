#version 450 core

//layout(set = 0, binding = 0) uniform Uniform {
//    float bar;
//} u;
//
//layout(location = 0) in In {
//    float foo;
//} i;
//
layout(location = 0) out vec4 color;


void main() {
    color = vec4(1.0, 0.0, 0.0, 1.0);
}