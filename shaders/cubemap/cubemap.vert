
// skybox.vert
#version 330 core

layout(location = 0) in vec3 a_pos;
layout (location=1) in vec3 a_nrm;   

out vec3 v_dir;

uniform mat4 u_view;   // camera view matrix
uniform mat4 u_proj;   // camera projection matrix

void main()
{
    // Remove translation so the skybox is always centered on the camera
    mat4 viewNoTrans = mat4(mat3(u_view));

    // Direction for cubemap lookup (object space cube centered at origin)
    v_dir = a_pos;

    // Push to far plane (depth = 1.0) so it stays behind everything
    vec4 pos = u_proj * viewNoTrans * vec4(a_pos, 1.0);
    gl_Position = pos.xyww;
}
