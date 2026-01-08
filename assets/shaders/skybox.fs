#version 330 core
in vec3 vDir;
out vec4 finalColor;

uniform float time;       // seconds
uniform int   isDungeon;  // 0 = overworld/day; 1 = dungeon (starfield)
uniform int   isSwamp;
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

        // Build planet-local basis (for ring + shading)
        vec3 tmpUpP = vec3(0.0, 1.0, 0.0);
        if (abs(dot(tmpUpP, planetDir)) > 0.9) tmpUpP = vec3(1.0, 0.0, 0.0);

        vec3 pRight = normalize(cross(tmpUpP, planetDir));
        vec3 pUp    = normalize(cross(planetDir, pRight));

        vec2 puv = vec2(dot(dir, pRight), dot(dir, pUp));

        float pR = length(puv); // distance from planet center in planet-uv space
        float behindFade = smoothstep(planetRadius*0.85, planetRadius*1.00, pR);

        // ----- Planet color: POPPY 3D SHADING + bands -----

        // normalize disk coords so the sphere reconstruction works for any planetRadius
        vec2 puvN = puv / max(1e-4, sin(planetRadius));

        float rr = dot(puvN, puvN);
        float z  = sqrt(max(0.0, 1.0 - rr));  // 1 at center, 0 at limb

        vec3 n = normalize(puvN.x * pRight + puvN.y * pUp + z * planetDir);

        // vec3 lightDir = normalize(vec3(-0.2, 0.5, 0.84));
        vec3 lightDir = normalize(vec3(-0.2, 0.9, 0.3));

        float ndl = max(dot(n, lightDir), 0.0);

        // harder terminator + darker nightside
        float light = 0.03 + 1.25 * ndl;
        light = pow(light, 1.8);

        // bands that curve correctly
        float lat = asin(clamp(dot(n, pUp), -1.0, 1.0));
        float stripes = sin(lat * 10.0 + time * 0.02) * 0.5 + 0.5;

        float bandNoise = fbm3f(vec3(puvN * 35.0 + vec2(5.2, 9.1), 0.0));
        stripes = mix(stripes, stripes * bandNoise, 0.45);

        float bandShade = mix(0.75, 1.25, stripes);

        vec3 planetBase = vec3(0.2, 0.5, 1.0);
        vec3 planetCol  = planetBase * light * bandShade;

        // specular highlight
        vec3 V = -dir;
        vec3 H = normalize(lightDir + V);
        float spec = pow(max(dot(n, H), 0.0), 64.0);
        planetCol += vec3(0.30, 0.40, 0.55) * spec;

        // stronger rim atmosphere (optional)
        float rim = pow(1.0 - z, 2.0);
        planetCol += vec3(0.08, 0.16, 0.30) * rim * 0.55;

        // ----- Ring band -----
        vec2 ruv = rot2(puv, radians(-20.0));

        float tilt = 0.3 + 0.4 * abs(dot(pUp, vec3(0.0, 1.0, 0.0)));
        vec2 e = vec2(ruv.x, ruv.y * (1.0 + tilt));

        float r = length(e);

        float ringR = planetRadius * 2.25;     // ring center radius
        float ringW = planetRadius * 1.5;    // ring thickness

        float ringMask = smoothstep(ringR + ringW*0.50, ringR + ringW*0.35, r) *
                         smoothstep(ringR - ringW*0.50, ringR - ringW*0.35, r);

        float ringTex = fbm3f(vec3(e * 80.0 + vec2(12.3, 4.7), 0.0));
        ringTex = mix(0.7, 1.2, ringTex);

        // RING COLOR
        vec3 ringBase = vec3(0.025, 0.05, 0.175);

        float centerLine = smoothstep(ringW*0.22, 0.0, abs(r - ringR));
        float ringBright = mix(1.0, 1.35, centerLine);

        vec3 ringCol = ringBase * ringTex * ringBright;

        float ringFront = ringMask * (1.0 - planetMask);
        float ringBack  = ringMask * planetMask * behindFade * 0.08;

        
        float frontOpacity = 1.00;   // try 0.8–1.5
        float backOpacity  = 0.03;   // try 0.00–0.10

        ringFront *= frontOpacity;
        ringBack  *= backOpacity;

        // composite planet then ring
        col = mix(col, planetCol, planetMask);
        col += ringCol * (ringFront + ringBack);

        // ======================
        // SPIRAL GALAXY (opposite side, same height as planet)
        // ======================

        vec3 galaxyDir = normalize(vec3(-planetDir.x, planetDir.y, -planetDir.z));

        float galaxyRadius = radians(2.0);
        float galaxyEdge   = radians(1.2);

        float galaxyTilt = 1.0;               // 0 = face-on, bigger = more edge-on
        float galaxyAngle = radians(25.0);    // rotate the ellipse on the sky

        float cosG   = clamp(dot(dir, galaxyDir), -1.0, 1.0);
        float thetaG = acos(cosG);

        float galaxyMask = smoothstep(galaxyRadius + galaxyEdge,
                                      galaxyRadius - galaxyEdge * 0.35,
                                      thetaG);
        galaxyMask = pow(galaxyMask, 0.9);

        if (galaxyMask > 0.001) {
            vec3 tmpUpG = vec3(0.0, 1.0, 0.0);
            if (abs(dot(tmpUpG, galaxyDir)) > 0.9) tmpUpG = vec3(1.0, 0.0, 0.0);

            vec3 gRight = normalize(cross(tmpUpG, galaxyDir));
            vec3 gUp    = normalize(cross(galaxyDir, gRight));

            vec2 uv = vec2(dot(dir, gRight), dot(dir, gUp));

            // Rotate (optional) so the squash direction isn't locked to gUp
            uv = rot2(uv, galaxyAngle);

            // Squash/stretch to make an oblong/tilted disk
            // Bigger galaxyTilt => more squished (more edge-on look)
            uv.y *= (1.0 + galaxyTilt);

            float r = length(uv);
            float a = atan(uv.y, uv.x);

            float rn = r / (galaxyRadius);
            rn = max(rn, 1e-4);

            float core = exp(-rn * 6.0);
            core = pow(core, 1.2);

            float arms     = 2.0;
            float twist    = 4.0;
            float armWidth = 1.5;

            float phase = a - twist * log(rn);

            float armSignal = cos(arms * phase);
            float armMask = smoothstep(1.0 - armWidth, 1.0, armSignal);

            float armFalloff = exp(-rn * 2.2);
            armMask *= armFalloff;

            float dust = fbm3f(vec3(uv * 140.0 + vec2(17.0, 9.0), 0.0));
            dust = smoothstep(0.45, 0.85, dust);
            float dustCut = mix(1.0, 0.55, dust);

            float clump = fbm3f(vec3(uv * 260.0 + vec2(33.0, 71.0), 0.0));
            clump = smoothstep(0.78, 0.95, clump);

            vec3 coreCol = vec3(1.05, 0.95, 0.85);
            vec3 armCol  = vec3(0.65, 0.80, 1.10);

            vec3 galaxyCol =
                coreCol * (core * 1.4) +
                armCol  * (armMask * 1.1) * dustCut +
                vec3(1.0) * (clump * armMask * 0.8);

            galaxyCol *= 0.6;
            col += galaxyCol * galaxyMask;
        }

        // Gamma
        finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
        return;
    }
    //swamp colors
    vec3 swampSkyHorizon = vec3(0.42, 0.56, 0.44); //.44
    vec3 swampSkyZenith = vec3(0.48, 0.62, 0.56);  //.56

    vec3 oceanSkyHorizon = vec3(0.0, 0.60, 1.00);
    vec3 oceanSkyZenith = vec3(0.0, 0.10, 1.00);
    // --- DAY: blue sky + soft clouds ---
    float h = clamp(dir.y*0.5 + 0.5, 0.0, 1.0);
    vec3 top = (isSwamp == 1) ? swampSkyZenith : oceanSkyZenith;
    vec3 hor = (isSwamp == 1) ? swampSkyHorizon : oceanSkyHorizon;

    vec3 sky = mix(hor, top, pow(h, 1.2));

    // moving clouds via fbm
    float cloud = fbm3f(dir * 3.0 + vec3(0.0, time*0.01, 0.0));
    float mask = smoothstep(0.55, 0.70, cloud);
    vec3 clouds = vec3(1.0);

    // base day color
    vec3 col = mix(sky, clouds, mask * 0.8);

    // ======================
    // SUN: yellow orb at zenith with fuzzy edge + soft glow
    // ======================
    vec3 sunDir = vec3(0.0, 1.0, 0.0);   // straight overhead

    float cosS   = clamp(dot(dir, sunDir), -1.0, 1.0);
    float thetaS = acos(cosS);

    // disk + soft edge
    float sunRadius = radians(2.0);      // size of the sun disk (try 1.0–3.0)
    float sunSoft   = radians(1.2);      // edge fuzz (bigger = fuzzier)
    float sunMask   = smoothstep(sunRadius + sunSoft,
                                sunRadius - sunSoft * 0.25,
                                thetaS);

    // sharpen slightly so it reads as a disk, but still soft
    // Warm sun color (keep <= 1.0 to preserve hue)
    vec3 sunCol  = vec3(1.0, 0.92, 0.25);  // yellow-orange, not pure yellow
    vec3 haloCol = vec3(1.0, 0.75, 0.20);

    // Disk mask (what you already compute)
    float sunDisk = sunMask;

    // Halo mask (wide, soft)
    float halo = exp(-thetaS * 10.0);
    halo = clamp(halo - sunDisk, 0.0, 1.0); // halo only outside the disk

    // First: tint the halo by mixing (not adding)
    col = mix(col, haloCol, halo * 0.35);

    // Then: place the disk by mixing (stronger, preserves color)
    col = mix(col, sunCol, sunDisk);

    finalColor = vec4(pow(col, vec3(1.0/2.2)), 1.0);
}
