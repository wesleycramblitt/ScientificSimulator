#version 330 core

layout (location=0) in vec3 a_pos;
layout (location=1) in vec3 a_nrm;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_worldPos;
out vec3 v_worldNrm;

void main() {
    vec4 worldPos = u_model * vec4(a_pos, 1.0);
    v_worldPos = worldPos.xyz;
    v_worldNrm = mat3(u_model) * a_nrm;
    gl_Position = u_proj * u_view * worldPos;
}
