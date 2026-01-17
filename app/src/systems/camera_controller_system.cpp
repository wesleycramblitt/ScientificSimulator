#include "systems/camera_controller_system.hpp"

void CameraControllerSystem::Update(Registry& registry, float dt) {
 //    vec3 campos{0.f, 2.f, 6.f};
 //    float yaw = -90.f;
 //    float pitch = -10.f;
 //    float speed = 8.f;
 //    float mousesens = 0.12f;
 //    mat4 p   = perspective(deg2rad(70.f), aspect, 0.05f, 400.f);
 //    mat4 v   = lookat(campos, campos + front, {0,1,0});
 //    mat4 mvp = mul(p, v);
 //
 //    gluseprogram(prog);
 //    gluniformmatrix4fv(umvp, 1, gl_false, mvp.m);
 //
 // auto deg2rad = [](float d){ return d * 3.1415926535f / 180.f; };
 //    vec3 front{
 //      std::cos(deg2rad(yaw)) * std::cos(deg2rad(pitch)),
 //      std::sin(deg2rad(pitch)),
 //      std::sin(deg2rad(yaw)) * std::cos(deg2rad(pitch))
 //    };
 //    front = norm(front);
 //
 //    vec3 right = norm(cross(front, {0,1,0}));
 //    vec3 up    = norm(cross(right, front));
 //
 //    float s = speed * (keys[sdl_scancode_lshift] ? 3.0f : 1.0f);
 //
 //    if (keys[SDL_SCANCODE_W]) camPos = camPos + front * (s * dt);
 //    if (keys[SDL_SCANCODE_S]) camPos = camPos - front * (s * dt);
 //    if (keys[SDL_SCANCODE_A]) camPos = camPos - right * (s * dt);
 //    if (keys[SDL_SCANCODE_D]) camPos = camPos + right * (s * dt);
 //    if (keys[SDL_SCANCODE_Q]) camPos = camPos - up    * (s * dt);
 //    if (keys[SDL_SCANCODE_E]) camPos = camPos + up    * (s * dt);
 //

}
