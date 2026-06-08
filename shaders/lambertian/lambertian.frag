#version 330 core

in vec3 norm;
in vec4 color;
out vec4 out_color;

uniform vec3 u_light_dir;

void main() {
    vec3 N = normalize(norm);
    vec3 L = normalize(-u_light_dir);

    float diff = max(dot(N,L), 0.0);

    out_color = vec4(color.xyz * diff, color.w);
}
