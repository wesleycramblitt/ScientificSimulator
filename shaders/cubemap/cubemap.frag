
// skybox.frag
#version 330 core

in vec3 vDir;
out vec4 FragColor;

uniform samplerCube u_skybox;

void main()
{
    FragColor = texture(u_skybox, normalize(vDir));
}
