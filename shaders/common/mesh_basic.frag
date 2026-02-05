#version 330 core

in vec2 v_uv;
out vec4 out_color;

void main() {
    // v0: constant color, later sample texture / PBR etc.
    out_color = vec4(0.85, 0.85, 0.90, 1.0);
}
