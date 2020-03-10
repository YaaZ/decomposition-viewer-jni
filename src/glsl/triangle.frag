#version 450 core

layout(location = 0) out vec4 color;


void main() {
    vec3 c = vec3(0.2, (1.0 - float(gl_FrontFacing)) * 0.8, float(gl_FrontFacing) * 0.8);
    color = vec4(c, 1.0);
}