#version 330 core
layout(location=0) in vec3 a_pos;
layout(location=1) in vec3 a_color;

uniform mat4 u_model, u_view, u_proj;

out vec3 v_color;

void main() {
    gl_Position = u_proj * u_view * u_model * vec4(a_pos, 1.0);
    // Larger, softer points — overlapping particles blend additively
    gl_PointSize = 4.0;
    v_color = a_color;
}
