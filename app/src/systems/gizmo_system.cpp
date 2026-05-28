#include "systems/gizmo_system.hpp"
#include "components/transform.hpp"
#include "components/camera.hpp"
#include "components/camera_controller.hpp"
#include "graphics/shader_manager.hpp"

#include <glad/gl.h>
#include <cmath>

GizmoSystem::GizmoSystem() {
    // Static VAO: 3 coloured axes from origin, 1 unit long
    struct { float x, y, z, r, g, b; } verts[] = {
        {0,0,0, 1.0f,0.2f,0.2f}, {1,0,0, 1.0f,0.2f,0.2f},   // X red
        {0,0,0, 0.2f,1.0f,0.2f}, {0,1,0, 0.2f,1.0f,0.2f},   // Y green
        {0,0,0, 0.2f,0.2f,1.0f}, {0,0,1, 0.2f,0.2f,1.0f},   // Z blue
    };
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void GizmoSystem::update(Registry& registry, const Window& window) {
    int ww, wh; float aspect;
    window.getDimensions(ww, wh, aspect);

    gw_ = 150; gh_ = 150;
    gx_ = ww - gw_ - 15;
    gy_ = 15;   // from bottom in GL coords

    // Diagonal camera looking at origin
    Vec3 eye = Vec3{1.4f, 1.1f, 1.7f}.norm() * 2.8f;
    gizmoView_ = Mat4::lookAt(eye, Vec3{0,0,0}, Vec3{0,1,0});
    gizmoProj_ = Mat4::perspective(0.55f, 1.0f, 0.1f, 20.0f);

    renderAxes();
    handleClick(registry, window);
}

void GizmoSystem::renderAxes() const {
    // Save current viewport
    GLint old_vp[4];
    glGetIntegerv(GL_VIEWPORT, old_vp);

    glViewport(gx_, gy_, gw_, gh_);
    glScissor(gx_, gy_, gw_, gh_);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    static ShaderManager shaders;
    GLuint prog = shaders.getOrLoad("grid", "shaders/grid/grid.vert", "shaders/grid/grid.frag");
    glUseProgram(prog);

    Mat4 I = Mat4::identity();
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view"),  1, GL_FALSE, gizmoView_.m);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"),  1, GL_FALSE, gizmoProj_.m);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, I.m);

    glBindVertexArray(vao_);
    glLineWidth(2.5f);
    glDrawArrays(GL_LINES, 0, 6);
    glLineWidth(1.0f);
    glBindVertexArray(0);

    glDisable(GL_SCISSOR_TEST);

    // Restore
    glViewport(old_vp[0], old_vp[1], old_vp[2], old_vp[3]);
}

void GizmoSystem::handleClick(Registry& registry, const Window& window) {
    int ww, wh; float aspect;
    window.getDimensions(ww, wh, aspect);

    float fmx, fmy;
    SDL_GetMouseState(&fmx, &fmy);
    float gl_my = (float)wh - fmy;  // GL viewport Y (bottom = 0)
    int   mx   = (int)fmx;

    bool inGizmo = (mx >= gx_ && mx <= gx_+gw_ && (int)gl_my >= gy_ && (int)gl_my <= gy_+gh_);

    // Detect fresh left click
    bool clicked = false;
    for (auto& ev : window.event_buffer) {
        if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN && ev.button.button == SDL_BUTTON_LEFT)
            clicked = true;
    }

    if (!inGizmo || !clicked) return;

    // Project each axis tip to screen space, find nearest to mouse
    Mat4 vp = Mat4::mul(gizmoProj_, gizmoView_);
    Vec3 axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

    int best = -1;
    float bestD2 = 32.0f * 32.0f;  // hit radius

    for (int i = 0; i < 3; ++i) {
        // Manual MVP transform for a single point
        float px = axes[i].x, py = axes[i].y, pz = axes[i].z;
        float cx = vp.m[0]*px + vp.m[4]*py + vp.m[8] *pz + vp.m[12];
        float cy = vp.m[1]*px + vp.m[5]*py + vp.m[9] *pz + vp.m[13];
        float cz = vp.m[2]*px + vp.m[6]*py + vp.m[10]*pz + vp.m[14];
        float cw = vp.m[3]*px + vp.m[7]*py + vp.m[11]*pz + vp.m[15];
        if (cw <= 1e-6f) continue;
        float nx = cx / cw;
        float ny = cy / cw;

        float sx = gx_ + (nx * 0.5f + 0.5f) * gw_;
        float sy = gy_ + (ny * 0.5f + 0.5f) * gh_;

        float dx = mx - sx;
        float dy = gl_my - sy;
        float d2 = dx*dx + dy*dy;
        if (d2 < bestD2) { bestD2 = d2; best = i; }
    }

    if (best < 0) return;

    // Snap camera to face clicked axis from a distance
    for (auto camE : registry.view<Camera, CameraController, Transform>()) {
        auto& cc = registry.get<CameraController>(camE);
        auto& tf = registry.get<Transform>(camE);

        float d = 15.0f;
        switch (best) {
            case 0: tf.position = Vec3{-d, 0, 0};  cc.yaw = 1.5708f;  cc.pitch = 0;        break; // +X  ← look from -X
            case 1: tf.position = Vec3{0, d, 0};   cc.yaw = 0;        cc.pitch = -1.5708f; break; // +Y  ← look from +Y down
            case 2: tf.position = Vec3{0, 0, -d};  cc.yaw = 0;        cc.pitch = 0;        break; // +Z  ← look from -Z
        }
        break;
    }
}
