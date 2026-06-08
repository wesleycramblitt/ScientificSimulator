
// skybox.frag
#version 330 core

in vec3 v_dir;
out vec4 FragColor;

uniform samplerCube u_skybox;

void main()
{
    FragColor = texture(u_skybox, normalize(v_dir));
}
