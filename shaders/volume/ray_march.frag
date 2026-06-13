#version 330 core

in vec3 v_world_pos;

layout(location = 0) out vec4 out_color;

uniform vec3      u_cam_pos;
uniform vec3      u_box_min;
uniform vec3      u_box_max;
uniform sampler3D u_volume;
uniform ivec3     u_grid_dims;
uniform float     u_absorption;

// Diverging: blue (slow) → transparent (normal) → red (fast)
// t ranges from -1 (slow) to +1 (fast), 0 = free-stream speed
vec3 colormap(float t) {
    t = clamp(t, -1.0, 1.0);
    if (t < 0.0) {
        // Slower: deep blue → light blue → fade to transparent
        float s = -t;  // 0→1 as we go slower
        if (s < 0.33) return mix(vec3(0.05, 0.05, 0.1), vec3(0.0, 0.3, 0.7), s / 0.33);
        if (s < 0.66) return mix(vec3(0.0, 0.3, 0.7), vec3(0.2, 0.6, 1.0), (s - 0.33) / 0.33);
        else          return mix(vec3(0.2, 0.6, 1.0), vec3(0.6, 0.85, 1.0), (s - 0.66) / 0.34);
    } else {
        // Faster: deep red → orange → bright yellow
        if (t < 0.25) return mix(vec3(0.05, 0.05, 0.1), vec3(0.7, 0.0, 0.0), t / 0.25);
        if (t < 0.50) return mix(vec3(0.7, 0.0, 0.0), vec3(1.0, 0.4, 0.0), (t - 0.25) / 0.25);
        if (t < 0.75) return mix(vec3(1.0, 0.4, 0.0), vec3(1.0, 0.8, 0.0), (t - 0.50) / 0.25);
        else          return mix(vec3(1.0, 0.8, 0.0), vec3(1.0, 1.0, 0.3), (t - 0.75) / 0.25);
    }
}

bool ray_box_intersect(vec3 origin, vec3 dir, out float t_entry, out float t_exit) {
    vec3 t0 = (u_box_min - origin) / dir;
    vec3 t1 = (u_box_max - origin) / dir;
    vec3 tmin_v = min(t0, t1);
    vec3 tmax_v = max(t0, t1);
    t_entry = max(max(tmin_v.x, tmin_v.y), tmin_v.z);
    t_exit  = min(min(tmax_v.x, tmax_v.y), tmax_v.z);
    return t_exit >= 0.0 && t_entry <= t_exit;
}

void main() {
    vec3 ray_dir = normalize(v_world_pos - u_cam_pos);
    float t_entry, t_exit;
    if (!ray_box_intersect(u_cam_pos, ray_dir, t_entry, t_exit)) discard;

    // Step size based on voxel dimensions (sample at half-voxel resolution)
    vec3  box_size   = u_box_max - u_box_min;
    vec3  voxel_size = box_size / vec3(u_grid_dims);
    float step_size  = 0.5 * min(voxel_size.x, min(voxel_size.y, voxel_size.z));

    float t = max(t_entry, 0.0);
    vec4 accum = vec4(0.0);

    // Jitter first sample to break up uniform-opacity banding
    float noise = fract(
        sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233)))
        * 43758.5453);
    t += noise * step_size;

    for (int i = 0; i < 2048 && t <= t_exit; i++) {
        vec3 pos      = u_cam_pos + ray_dir * t;
        vec3 texcoord = (pos - u_box_min) / box_size;
        if (any(lessThan(texcoord, vec3(0.0))) || any(greaterThan(texcoord, vec3(1.0)))) break;

        float val     = texture(u_volume, texcoord).r;  // -1 (slow) .. 0 .. +1 (fast)
        float density = smoothstep(0.08, 0.50, abs(val)); // wide transparent zone at free-stream

        if (density > 0.0) {
            vec3  col   = colormap(val);
            float alpha = 1.0 - exp(-u_absorption * density * step_size);

            accum.rgb += (1.0 - accum.a) * alpha * col;
            accum.a   += (1.0 - accum.a) * alpha;
        }

        if (accum.a > 0.98) break;
        t += step_size;
    }

    if (accum.a < 0.005) discard;
    out_color = accum;
}
