#version 330 core

layout (location=0) in vec3 a_pos;
layout (location=1) in vec3 a_nrm;   
layout (location=4) in vec4 a_color;   

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 norm;
out vec4 color;

void main() {
    color = a_color;
    gl_Position = u_proj * u_view * u_model * vec4(a_pos, 1.0);
    norm = mat3(transpose(inverse(u_model))) * a_nrm;
}
