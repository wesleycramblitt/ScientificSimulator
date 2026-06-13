#version 330 core
in vec3 v_color;
out vec4 out_color;

void main() {
    // Soft circular falloff (gaussian-like)
    float d = length(gl_PointCoord - 0.5) * 2.0;  // 0 at center, 1 at edge
    float alpha = exp(-d * d * 3.0);               // sharp gaussian
    // Premultiplied alpha for additive blending
    out_color = vec4(v_color * alpha, alpha);
}
