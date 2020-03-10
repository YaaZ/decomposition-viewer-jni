#version 450 core

#define POINT_VERTICES_NUMBEER 16
const vec2 POINT_VERTICES[POINT_VERTICES_NUMBEER + 1] = {
    vec2(1.0, 0.0),
    vec2(0.9238795325112867, 0.3826834323650898),
    vec2(0.7071067811865476, 0.7071067811865475),
    vec2(0.38268343236508984, 0.9238795325112867),
    vec2(6.123233995736766E-17, 1.0),
    vec2(-0.3826834323650897, 0.9238795325112867),
    vec2(-0.7071067811865475, 0.7071067811865476),
    vec2(-0.9238795325112867, 0.3826834323650899),
    vec2(-1.0, 1.2246467991473532E-16),
    vec2(-0.9238795325112868, -0.38268343236508967),
    vec2(-0.7071067811865477, -0.7071067811865475),
    vec2(-0.38268343236509034, -0.9238795325112865),
    vec2(-1.8369701987210297E-16, -1.0),
    vec2(0.38268343236509, -0.9238795325112866),
    vec2(0.7071067811865474, -0.7071067811865477),
    vec2(0.9238795325112865, -0.3826834323650904),
    vec2(1.0, 0.0)
};
#define POINT_RADIUS 10.0


layout(lines) in;
layout(triangle_strip, max_vertices = POINT_VERTICES_NUMBEER * 3) out;

layout(set = 0, binding = 0) uniform Uniform {
    vec2 extent;
} u;


void main() {
    vec2 center = gl_in[0].gl_Position.xy;

    for(int i = 0; i < POINT_VERTICES_NUMBEER; i++) {

        gl_Position = vec4(center, 0.0, 1.0);
        EmitVertex();

        gl_Position = vec4(center + POINT_VERTICES[i] * POINT_RADIUS / u.extent, 0.0, 1.0);
        EmitVertex();

        gl_Position = vec4(center + POINT_VERTICES[i + 1] * POINT_RADIUS / u.extent, 0.0, 1.0);
        EmitVertex();

        EndPrimitive();

    }
}