#version 330
in vec3 vDir;
out vec4 finalColor;

uniform float time;       // seconds
uniform int   isDungeon;  // 0 = overworld/day; 1 = dungeon (starfield)

// --------- small helpers ----------
float hash31(vec3 p) {
    return fract(sin(dot(p, vec3(127.1,311.7,74.7))) * 43758.5453123);
}

float noise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f*f*(3.0 - 2.0*f);
    float n000 = hash31(i + vec3(0,0,0));
    float n100 = hash31(i + vec3(1,0,0));
    float n010 = hash31(i + vec3(0,1,0));
    float n110 = hash31(i + vec3(1,1,0));
    float n001 = hash31(i + vec3(0,0,1));
    float n101 = hash31(i + vec3(1,0,1));
    float n011 = hash31(i + vec3(0,1,1));
    float n111 = hash31(i + vec3(1,1,1));

    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);
    float nxy0 = mix(nx00, nx10, f.y);
    float nxy1 = mix(nx01, nx11, f.y);
    return mix(nxy0, nxy1, f.z);
}

// cheap fractal brownian motion
float fbm(vec3 p) {
    float a = 0.5, s = 0.0;
    for (int i=0;i<4;i++) { s += a*noise3(p); p*=2.0; a*=0.5; }
    return s;
}

// -----------------------------------

void main() {
    vec3 dir = normalize(vDir);

    if (isDungeon == 1) {
        // --- NIGHT: minimal starfield ---
        // base night tint (very dark blue)
        vec3 night = vec3(0.001, 0.002, 0.004);

        // small stars
        float n1 = noise3(dir * 80.0);
        float starsSmall = step(0.97, n1) * pow(n1, 40.0);

        // rare bright stars
        // float n2 = noise3(dir * 40.0);
        // float starsBig = step(0.98, n2) * pow(n2, 10.0) * 3.0;

        vec3 p = floor(dir * 220.0) + 0.1;   // snap to grid
        float n = hash31(p);
        float starsBig = step(0.9999, n);        // just a yes/no star



        vec3 col = night + vec3(1.0)*(starsSmall + starsBig);

        finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
        return;
    }

    // --- DAY: blue sky + soft clouds ---
    // sky gradient from zenith
    float h = clamp(dir.y*0.5 + 0.5, 0.0, 1.0);
    vec3 top = vec3(0.0, 0.10, 1.00); // zenith blue
    vec3 hor = vec3(0.0, 0.60, 1.00); // near horizon
    vec3 sky = mix(hor, top, pow(h, 1.2));

    // simple moving clouds (soft fbm mask)
    float cloud = fbm(dir * 3.0 + vec3(0.0, time*0.01, 0.0));
    float mask = smoothstep(0.55, 0.70, cloud); // threshold for puffy shapes
    vec3 clouds = vec3(1.0);

    vec3 col = mix(sky, clouds, mask * 0.8); // 0.8 = cloud opacity
    finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
}
