
// skybox.vert
#version 330 core

layout(location = 0) in vec3 aPos;
layout (location=1) in vec3 a_nrm;   // optional; can ignore if you want

out vec3 vDir;

uniform mat4 u_view;   // camera view matrix
uniform mat4 u_proj;   // camera projection matrix

void main()
{
    // Remove translation so the skybox is always centered on the camera
    mat4 viewNoTrans = mat4(mat3(u_view));

    // Direction for cubemap lookup (object space cube centered at origin)
    vDir = aPos;

    // Push to far plane (depth = 1.0) so it stays behind everything
    vec4 pos = u_proj * viewNoTrans * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
