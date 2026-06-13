#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_model;

out vec3 vColor;

void main() {
    vec4 worldPos = u_model * vec4(aPosition, 1.0);
    gl_Position = u_proj * u_view * worldPos;
    vColor = aColor;
}
