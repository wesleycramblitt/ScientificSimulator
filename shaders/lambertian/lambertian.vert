#version 330 core

layout (location=0) in vec3 a_pos;
layout (location=1) in vec3 a_nrm;   // optional; can ignore if you want
//layout (location=2) in vec3 a_uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

//out vec3 v_uv;
out vec3 norm;

void main() {
 //   v_uv = a_uv;
    gl_Position = u_proj * u_view * u_model * vec4(a_pos, 1.0);
    norm = mat3(transpose(inverse(u_model))) * a_nrm;
}
