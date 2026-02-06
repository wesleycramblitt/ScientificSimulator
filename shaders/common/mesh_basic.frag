#version 330 core

//in vecv v_uv;
out vec4 out_color;

void main() {
    // v0: constant color, later sample texture / PBR etc.
    out_color = vec4(0.5, 0.5, 0.5, 1.0);
}
