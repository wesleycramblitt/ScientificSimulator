#version 330 core

in vec3 v_world_pos;

layout(location = 0) out vec4 out_color;

uniform vec3      u_cam_pos;
uniform vec3      u_box_min;
uniform vec3      u_box_max;
uniform sampler3D u_volume;

// Black-body / flame color: black → red → orange → yellow → white
vec3 flame(float t) {
    t = clamp(t, 0.0, 1.0);
    if (t < 0.25) return mix(vec3(0,0,0), vec3(0.8,0,0), t/0.25);        // black → red
    if (t < 0.50) return mix(vec3(0.8,0,0), vec3(1.0,0.5,0), (t-0.25)/0.25); // red → orange
    if (t < 0.75) return mix(vec3(1.0,0.5,0), vec3(1.0,1.0,0), (t-0.5)/0.25); // orange → yellow
    return mix(vec3(1.0,1.0,0), vec3(1.0,1.0,1.0), (t-0.75)/0.25);       // yellow → white
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

    float t = max(t_entry, 0.0);
    float diag = length(u_box_max - u_box_min);
    int   max_steps = 256;
    float step_size = diag / float(max_steps);

    vec4 accum = vec4(0.0);

    for (int i = 0; i < max_steps; i++) {
        if (t > t_exit) break;
        vec3 pos = u_cam_pos + ray_dir * t;
        vec3 texcoord = (pos - u_box_min) / (u_box_max - u_box_min);
        if (any(lessThan(texcoord, vec3(0.0))) || any(greaterThan(texcoord, vec3(1.0)))) break;

        float val = texture(u_volume, texcoord).r;

        vec3 col;
        float alpha;
        if (val < 0.0005) {
            col = vec3(0.05, 0.05, 0.1);  // dark blue for still/obstacle
            alpha = 0.3;
        } else {
            col = flame(val * 15.0);       // scale: 0.05 → 0.75 on flame scale
            alpha = val * 6.0;             // opacity proportional to speed
        }

        accum.rgb += (1.0 - accum.a) * alpha * col;
        accum.a   += (1.0 - accum.a) * alpha;
        if (accum.a > 0.95) break;
        t += step_size;
    }

    if (accum.a < 0.01) discard;
    out_color = accum;
}
