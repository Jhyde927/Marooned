#version 330 core
in vec3 vDir;
out vec4 finalColor;

uniform float time;       // seconds
uniform int   isDungeon;  // 0 = overworld/day; 1 = dungeon (starfield)

// --------- small helpers ----------
float hash31(vec3 p) {
    return fract(sin(dot(p, vec3(127.1,311.7,74.7))) * 43758.5453123);
}

// NOTE: renamed to avoid colliding with GLSL builtins
float valueNoise3f(vec3 p) {
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

// cheap fractal brownian motion (using valueNoise3f)
float fbm3f(vec3 p) {
    float amp = 0.5;
    float sum = 0.0;
    for (int i = 0; i < 4; ++i) {
        sum += amp * valueNoise3f(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return sum;
}

// -----------------------------------

void main() {
    vec3 dir = normalize(vDir);

    if (isDungeon == 1) {
        // --- NIGHT: minimal starfield ---
        vec3 night = vec3(0.001, 0.002, 0.004);

        // ============================
        // SMALL STARS with tiny radius
        // ============================
        float smallScale = 350.0;          // controls density / size on dome
        vec3  gridPos    = dir * smallScale;

        vec3  baseCell   = floor(gridPos);
        vec3  cellCenter = baseCell + vec3(0.5);

        float rSmall     = hash31(baseCell);
        float hasStar    = step(0.998, rSmall);  // density

        float d          = length(gridPos - cellCenter);
        float radius     = 0.8;
        float falloff    = smoothstep(radius, 0.0, d);

        float brightSmall = mix(0.4, 1.0,
                                hash31(baseCell + vec3(17.3, 9.1, 3.7)));

        float starsSmall = hasStar * falloff * brightSmall;

        // ======================
        // BIG STARS (as before)
        // ======================
        float bigScale = 250.0;
        vec3  cellBig  = floor(dir * bigScale);
        float rBig     = hash31(cellBig + vec3(91.7, 23.5, 11.2));

        float maskBig   = step(0.999, rBig); // rare
        float brightBig = 1.5 + hash31(cellBig + vec3(5.0, 8.0, 13.0));
        float starsBig  = maskBig * brightBig;

        // Base sky color + stars
        vec3 col = night + vec3(1.0) * (starsSmall + starsBig);

        // ======================
        // MOON: soft crater-like surface
        // ======================

        vec3 moonDir = normalize(vec3(-0.3, 0.4, 0.7));

        float cosTheta = clamp(dot(dir, moonDir), -1.0, 1.0);
        float theta    = acos(cosTheta);

        float moonRadius = radians(4.0);   // keep this
        float edgeSoft   = radians(0.95);   // was 1.5, much narrower band now

        // Sharper edge: narrower smoothstep band
        float moonMask = smoothstep(moonRadius + edgeSoft,
                                    moonRadius - edgeSoft * 0.3,  // slightly asymmetric = crisper
                                    theta);

        // Extra sharpening, but still not razor-sharp
        moonMask = pow(moonMask, 0.6);   // 0.5–0.7 = nice; 1.0 = original

        if (moonMask > 0.0) {
            // Orthonormal basis around moonDir to get 2D coords
            vec3 tmpUp = vec3(0.0, 1.0, 0.0);
            if (abs(dot(tmpUp, moonDir)) > 0.9) {
                tmpUp = vec3(1.0, 0.0, 0.0);
            }

            vec3 moonRight = normalize(cross(tmpUp, moonDir));
            vec3 moonUp    = normalize(cross(moonDir, moonRight));

            float x = dot(dir, moonRight);
            float y = dot(dir, moonUp);
            vec2  uv = vec2(x, y);

            vec2 pBig  = uv * 20.0;   // big crater size
            vec2 pFine = uv * 120.0;  // tiny surface detail

            float nBig  = fbm3f(vec3(pBig,  0.0));
            float nFine = fbm3f(vec3(pFine + 19.7, 0.0));

            // ridge from big noise -> crater rims
            float ridgeBig = 1.0 - abs(2.0 * nBig - 1.0);

            // combine: strong big rims, subtle fine breakup
            float combined = ridgeBig + 0.15 * nFine;

            // remap
            float detail = smoothstep(0.55, 0.9, combined);
            float surfaceShade = mix(0.5, 1.0, detail);

            // slight limb brightening so it reads as a sphere
            float rLocal = length(uv);   // 0 at center, ~moonRadius in "uv-space"
            float limb   = smoothstep(0.0, 0.99, rLocal);
            float rimBoost = mix(1.0, 2.12, limb); // subtle

            vec3 moonBase  = vec3(0.9, 0.9, 0.95);
            vec3 moonColor = moonBase * surfaceShade * rimBoost;

            col = mix(col, moonColor, moonMask);
        }

        // Gamma
        finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
        return;
    }


    // --- DAY: blue sky + soft clouds ---
    float h = clamp(dir.y*0.5 + 0.5, 0.0, 1.0);
    vec3 top = vec3(0.0, 0.10, 1.00); // zenith blue
    vec3 hor = vec3(0.0, 0.60, 1.00); // near horizon
    vec3 sky = mix(hor, top, pow(h, 1.2));

    // moving clouds via fbm
    float cloud = fbm3f(dir * 3.0 + vec3(0.0, time*0.01, 0.0));
    float mask = smoothstep(0.55, 0.70, cloud);
    vec3 clouds = vec3(1.0);

    vec3 col = mix(sky, clouds, mask * 0.8); // 0.8 = cloud opacity
    finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
}
