#pragma once
#include <cstdint>

enum class ProjectionType : uint8_t {
    Perspective,
    Orthographic
}

struct Camera {
    ProjectionType projection = ProjectionType::Perspective;
    // Perspective
    float fov_y_radians = 60.0f * 3.1415926535f / 180.0f;

    // Orthographic
    float ortho_height = 10.0f;   // world units; width derived from aspect

    // Common
    float near_plane = 0.1f;
    float far_plane  = 1000.0f;

    // Optional tuning
    float exposure = 1.0f;        // useful if you do HDR later
};
