#version 450 core

layout (constant_id = 0) const float color_r = 0;
layout (constant_id = 1) const float color_g = 0;
layout (constant_id = 2) const float color_b = 0;

layout(location = 0) out vec4 color;


void main() {
    color = vec4(color_r, color_g, color_b, 1.0);
}