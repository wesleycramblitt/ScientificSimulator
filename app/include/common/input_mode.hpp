#pragma once

/// Toggles between first-person camera control and UI interaction mode.
enum class InputMode {
    FPS,  ///< Relative mouse, camera moves with WASD+Mouse
    UI    ///< Absolute mouse, ImGui captures input, camera frozen
};

