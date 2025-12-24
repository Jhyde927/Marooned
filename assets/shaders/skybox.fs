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

// 2D rotation helper (for ring angle)
vec2 rot2(vec2 p, float a) {
    float c = cos(a), s = sin(a);
    return vec2(c*p.x - s*p.y, s*p.x + c*p.y);
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

        float moonRadius = radians(4.0);
        float edgeSoft   = radians(0.95);

        float moonMask = smoothstep(moonRadius + edgeSoft,
                                    moonRadius - edgeSoft * 0.3,
                                    theta);
        moonMask = pow(moonMask, 0.6);

        if (moonMask > 0.0) {
            vec3 tmpUp = vec3(0.0, 1.0, 0.0);
            if (abs(dot(tmpUp, moonDir)) > 0.9) {
                tmpUp = vec3(1.0, 0.0, 0.0);
            }

            vec3 moonRight = normalize(cross(tmpUp, moonDir));
            vec3 moonUp    = normalize(cross(moonDir, moonRight));

            vec2 uv = vec2(dot(dir, moonRight), dot(dir, moonUp));

            vec2 pBig  = uv * 20.0;
            vec2 pFine = uv * 120.0;

            float nBig  = fbm3f(vec3(pBig,  0.0));
            float nFine = fbm3f(vec3(pFine + 19.7, 0.0));

            float ridgeBig = 1.0 - abs(2.0 * nBig - 1.0);
            float combined = ridgeBig + 0.15 * nFine;

            float detail = smoothstep(0.55, 0.9, combined);
            float surfaceShade = mix(0.5, 1.0, detail);

            float rLocal = length(uv);
            float limb   = smoothstep(0.0, 0.99, rLocal);
            float rimBoost = mix(1.0, 2.12, limb);

            vec3 moonBase  = vec3(0.9, 0.9, 0.95);
            vec3 moonColor = moonBase * surfaceShade * rimBoost;

            col = mix(col, moonColor, moonMask);
        }

        // ======================
        // PLANET + RING (below the moon)
        // ======================

        vec3 planetDir = normalize(vec3(0.45, 0.60, 0.87)); // pick a nice spot
        float planetRadius = radians(0.75); 
        float edgeSoftP    = radians(0.25);

        float cosP   = clamp(dot(dir, planetDir), -1.0, 1.0);
        float thetaP = acos(cosP);

        float planetMask = smoothstep(planetRadius + edgeSoftP,
                                      planetRadius - edgeSoftP * 0.25,
                                      thetaP);
        planetMask = pow(planetMask, 1.4);

        // Build planet-local basis (for ring + texture-ish shading)
        vec3 tmpUpP = vec3(0.0, 1.0, 0.0);
        if (abs(dot(tmpUpP, planetDir)) > 0.9) tmpUpP = vec3(1.0, 0.0, 0.0);

        vec3 pRight = normalize(cross(tmpUpP, planetDir));
        vec3 pUp    = normalize(cross(planetDir, pRight));

        vec2 puv = vec2(dot(dir, pRight), dot(dir, pUp));

        float pR = length(puv); // distance from planet center in planet-uv space
        float behindFade = smoothstep(planetRadius*0.85, planetRadius*1.00, pR);
        // behindFade = 0 in center, 1 near/outside limb

        // ----- Planet color / subtle banding -----
        // simple terminator-ish lighting
        vec3 lightDir = normalize(vec3(-0.2, 0.5, 0.84));
        float ndl = clamp(dot(planetDir, lightDir), 0.0, 1.0); // constant-ish for a skybox, but helps
        float baseLit = 0.55 + 0.45 * ndl;

        // add faint latitudinal bands using noise in planet uv
        float bands = fbm3f(vec3(puv * 18.0 + vec2(time*0.01, 0.0), 0.0));
        bands = smoothstep(0.35, 0.75, bands); // clamp into band-ish regions
        float bandShade = mix(0.85, 1.05, bands);

        //PLANET COLOR
        vec3 planetBase = vec3(0.25, 0.55, 1.10);//blue //vec3(1.10, 0.55, 0.18);   // orange
        vec3 planetCol  = planetBase * baseLit * bandShade;

        // ----- Ring band -----
        // rotate ring a bit so it isn't perfectly horizontal
        vec2 ruv = rot2(puv, radians(-20.0));

        // tilt: squash Y for an ellipse (bigger tilt => more squashed)
        float tilt = 0.35; //0.65
        vec2 e = vec2(ruv.x, ruv.y * (1.0 + tilt));

        float r = length(e);

        float ringR = planetRadius * 2.0;    // ring center radius
        float ringW = planetRadius * 1.5;    // ring thickness

        // soft band edges
        float ringMask = smoothstep(ringR + ringW*0.50, ringR + ringW*0.35, r) *
                         smoothstep(ringR - ringW*0.50, ringR - ringW*0.35, r);

        // ring texture: a little radial variation
        float ringTex = fbm3f(vec3(e * 80.0 + vec2(12.3, 4.7), 0.0));
        ringTex = mix(0.7, 1.2, ringTex);


        //RING COLOR
        vec3 ringBase =  vec3(0.05, 0.10, 0.35);//vec3(0.2, 0.02, 0.2); //vec3(0.1, 0.08, 0.05);

        // "line through it" vibe: boost brightness along centerline of ring thickness
        float centerLine = smoothstep(ringW*0.22, 0.0, abs(r - ringR));
        float ringBright = mix(1.0, 1.35, centerLine);

        vec3 ringCol = ringBase * ringTex * ringBright;

        // simple occlusion:
        // - front ring (outside planet disk) is strong
        // - a tiny hint of back ring (behind planet) can help it read
        float ringFront = ringMask * (1.0 - planetMask);
        //float ringBack  = ringMask * planetMask * 0.12; //0.12
        float ringBack = ringMask * planetMask * behindFade * 0.08;

        // composite planet then ring
        col = mix(col, planetCol, planetMask);
        col += ringCol * (ringFront + ringBack);

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

    vec3 col = mix(sky, clouds, mask * 0.8);
    finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
}


// #version 330 core
// in vec3 vDir;
// out vec4 finalColor;

// uniform float time;       // seconds
// uniform int   isDungeon;  // 0 = overworld/day; 1 = dungeon (starfield)

// // --------- small helpers ----------
// float hash31(vec3 p) {
//     return fract(sin(dot(p, vec3(127.1,311.7,74.7))) * 43758.5453123);
// }

// // NOTE: renamed to avoid colliding with GLSL builtins
// float valueNoise3f(vec3 p) {
//     vec3 i = floor(p);
//     vec3 f = fract(p);
//     f = f*f*(3.0 - 2.0*f);

//     float n000 = hash31(i + vec3(0,0,0));
//     float n100 = hash31(i + vec3(1,0,0));
//     float n010 = hash31(i + vec3(0,1,0));
//     float n110 = hash31(i + vec3(1,1,0));
//     float n001 = hash31(i + vec3(0,0,1));
//     float n101 = hash31(i + vec3(1,0,1));
//     float n011 = hash31(i + vec3(0,1,1));
//     float n111 = hash31(i + vec3(1,1,1));

//     float nx00 = mix(n000, n100, f.x);
//     float nx10 = mix(n010, n110, f.x);
//     float nx01 = mix(n001, n101, f.x);
//     float nx11 = mix(n011, n111, f.x);
//     float nxy0 = mix(nx00, nx10, f.y);
//     float nxy1 = mix(nx01, nx11, f.y);
//     return mix(nxy0, nxy1, f.z);
// }

// // cheap fractal brownian motion (using valueNoise3f)
// float fbm3f(vec3 p) {
//     float amp = 0.5;
//     float sum = 0.0;
//     for (int i = 0; i < 4; ++i) {
//         sum += amp * valueNoise3f(p);
//         p *= 2.0;
//         amp *= 0.5;
//     }
//     return sum;
// }

// // -----------------------------------

// void main() {
//     vec3 dir = normalize(vDir);

//     if (isDungeon == 1) {
//         // --- NIGHT: minimal starfield ---
//         vec3 night = vec3(0.001, 0.002, 0.004);

//         // ============================
//         // SMALL STARS with tiny radius
//         // ============================
//         float smallScale = 350.0;          // controls density / size on dome
//         vec3  gridPos    = dir * smallScale;

//         vec3  baseCell   = floor(gridPos);
//         vec3  cellCenter = baseCell + vec3(0.5);

//         float rSmall     = hash31(baseCell);
//         float hasStar    = step(0.998, rSmall);  // density

//         float d          = length(gridPos - cellCenter);
//         float radius     = 0.8;
//         float falloff    = smoothstep(radius, 0.0, d);

//         float brightSmall = mix(0.4, 1.0,
//                                 hash31(baseCell + vec3(17.3, 9.1, 3.7)));

//         float starsSmall = hasStar * falloff * brightSmall;

//         // ======================
//         // BIG STARS (as before)
//         // ======================
//         float bigScale = 250.0;
//         vec3  cellBig  = floor(dir * bigScale);
//         float rBig     = hash31(cellBig + vec3(91.7, 23.5, 11.2));

//         float maskBig   = step(0.999, rBig); // rare
//         float brightBig = 1.5 + hash31(cellBig + vec3(5.0, 8.0, 13.0));
//         float starsBig  = maskBig * brightBig;

//         // Base sky color + stars
//         vec3 col = night + vec3(1.0) * (starsSmall + starsBig);

//         // ======================
//         // MOON: soft crater-like surface
//         // ======================

//         vec3 moonDir = normalize(vec3(-0.3, 0.4, 0.7));

//         float cosTheta = clamp(dot(dir, moonDir), -1.0, 1.0);
//         float theta    = acos(cosTheta);

//         float moonRadius = radians(4.0);   // keep this
//         float edgeSoft   = radians(0.95);   // was 1.5, much narrower band now

//         // Sharper edge: narrower smoothstep band
//         float moonMask = smoothstep(moonRadius + edgeSoft,
//                                     moonRadius - edgeSoft * 0.3,  // slightly asymmetric = crisper
//                                     theta);

//         // Extra sharpening, but still not razor-sharp
//         moonMask = pow(moonMask, 0.6);   // 0.5–0.7 = nice; 1.0 = original

//         if (moonMask > 0.0) {
//             // Orthonormal basis around moonDir to get 2D coords
//             vec3 tmpUp = vec3(0.0, 1.0, 0.0);
//             if (abs(dot(tmpUp, moonDir)) > 0.9) {
//                 tmpUp = vec3(1.0, 0.0, 0.0);
//             }

//             vec3 moonRight = normalize(cross(tmpUp, moonDir));
//             vec3 moonUp    = normalize(cross(moonDir, moonRight));

//             float x = dot(dir, moonRight);
//             float y = dot(dir, moonUp);
//             vec2  uv = vec2(x, y);

//             vec2 pBig  = uv * 20.0;   // big crater size
//             vec2 pFine = uv * 120.0;  // tiny surface detail

//             float nBig  = fbm3f(vec3(pBig,  0.0));
//             float nFine = fbm3f(vec3(pFine + 19.7, 0.0));

//             // ridge from big noise -> crater rims
//             float ridgeBig = 1.0 - abs(2.0 * nBig - 1.0);

//             // combine: strong big rims, subtle fine breakup
//             float combined = ridgeBig + 0.15 * nFine;

//             // remap
//             float detail = smoothstep(0.55, 0.9, combined);
//             float surfaceShade = mix(0.5, 1.0, detail);

//             // slight limb brightening so it reads as a sphere
//             float rLocal = length(uv);   // 0 at center, ~moonRadius in "uv-space"
//             float limb   = smoothstep(0.0, 0.99, rLocal);
//             float rimBoost = mix(1.0, 2.12, limb); // subtle

//             vec3 moonBase  = vec3(0.9, 0.9, 0.95);
//             vec3 moonColor = moonBase * surfaceShade * rimBoost;

//             col = mix(col, moonColor, moonMask);
//         }

//         // Gamma
//         finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
//         return;
//     }


//     // --- DAY: blue sky + soft clouds ---
//     float h = clamp(dir.y*0.5 + 0.5, 0.0, 1.0);
//     vec3 top = vec3(0.0, 0.10, 1.00); // zenith blue
//     vec3 hor = vec3(0.0, 0.60, 1.00); // near horizon
//     vec3 sky = mix(hor, top, pow(h, 1.2));

//     // moving clouds via fbm
//     float cloud = fbm3f(dir * 3.0 + vec3(0.0, time*0.01, 0.0));
//     float mask = smoothstep(0.55, 0.70, cloud);
//     vec3 clouds = vec3(1.0);

//     vec3 col = mix(sky, clouds, mask * 0.8); // 0.8 = cloud opacity
//     finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
// }
