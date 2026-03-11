#version 330 core

in vec3 norm;
out vec4 out_color;


uniform vec3 u_light_dir;
uniform vec3 u_object_color;

void main() {
    vec3 N = normalize(norm);
    vec3 L = normalize(-u_light_dir);

    float diff = max(dot(N,L), 0.0);

    vec3 color = u_object_color * diff;
    out_color = vec4(color, 1.0); 
}
