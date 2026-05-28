#version 330 core

in vec3 v_worldPos;
in vec3 v_worldNrm;

uniform vec3      u_camPos;
uniform samplerCube u_skybox;

out vec4 out_color;

void main() {
    vec3 N = normalize(v_worldNrm);
    vec3 V = normalize(u_camPos - v_worldPos);
    vec3 R = reflect(-V, N);

    out_color = texture(u_skybox, R);
}
