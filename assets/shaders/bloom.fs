#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D sceneTexture;
uniform vec2  resolution;

// Simple bloom + tone mapping
uniform float bloomStrength;       // 0.0 = off, ~0.2–1.0 typical
uniform float uExposure;           // 0.5–2.0 typical (default 1.0)
uniform int   uToneMapOperator;    // 0=Off, 1=ACES, 2=Reinhard

// --- sRGB <-> linear helpers ---
vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }
vec3 toSRGB  (vec3 c) { return pow(c, vec3(1.0/2.2)); }

// --- Tone mapping operators (expect linear) ---

// ACES (Hable-ish)
vec3 ToneMapACES(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

// Simple Reinhard
vec3 ToneMapReinhard(vec3 x) {
    return x / (1.0 + x);
}

void main() {
    vec2 texelSize = 1.0 / resolution;

    // Sample the source scene (sRGB) and convert to linear for math
    vec3 srcSRGB = texture(sceneTexture, fragTexCoord).rgb;
    vec3 srcLin  = toLinear(srcSRGB);

    // --- Basic bloom (bright-pass + small blur, all in linear) ---
    vec3 bloomLin   = vec3(0.0);
    float weightSum = 0.0;

    const float threshold = 0.8; // in linear luminance
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 uv = fragTexCoord + vec2(x, y) * texelSize;
            vec3 s   = toLinear(texture(sceneTexture, uv).rgb);

            float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
            float w   = smoothstep(threshold, threshold + 1.0, lum);

            bloomLin   += s * w;
            weightSum  += w;
        }
    }

    if (weightSum > 0.0) {
        bloomLin /= weightSum;
    } else {
        bloomLin = vec3(0.0);
    }

    // Combine scene + bloom in linear space
    vec3 resultLin = srcLin + bloomLin * bloomStrength;

    // --- Tone mapping (still linear) ---
    vec3 mappedLin = resultLin * uExposure;   // exposure pre-scale

    if (uToneMapOperator == 1) {
        mappedLin = ToneMapACES(mappedLin);
    } else if (uToneMapOperator == 2) {
        mappedLin = ToneMapReinhard(mappedLin);
    }
    // 0 = off (just exposure)

    // Convert back to sRGB for display
    vec3 resultSRGB = toSRGB(mappedLin);

    finalColor = vec4(clamp(resultSRGB, 0.0, 1.0), 1.0);
}


// #version 330

// in vec2 fragTexCoord;
// out vec4 finalColor;

// uniform sampler2D sceneTexture;
// uniform vec2  resolution;

// // Simple bloom + tone mapping + vignette
// uniform float bloomStrength;       // 0.0 = off, ~0.2–1.0 typical
// uniform float uExposure;           // 0.5–2.0 typical (default 1.0)
// uniform int   uToneMapOperator;    // 0=Off, 1=ACES, 2=Reinhard
// uniform float vignetteStrength;    // 0.0 = off, 0.5–1.0 = strong

// // --- sRGB <-> linear helpers ---
// vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }
// vec3 toSRGB  (vec3 c) { return pow(c, vec3(1.0/2.2)); }

// // --- Tone mapping operators (expect linear) ---

// // ACES (Hable-ish)
// vec3 ToneMapACES(vec3 x) {
//     float a = 2.51;
//     float b = 0.03;
//     float c = 2.43;
//     float d = 0.59;
//     float e = 0.14;
//     return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
// }

// // Simple Reinhard
// vec3 ToneMapReinhard(vec3 x) {
//     return x / (1.0 + x);
// }

// void main() {
//     vec2 texelSize = 1.0 / resolution;

//     // Sample the source scene (sRGB) and convert to linear for math
//     vec3 srcSRGB = texture(sceneTexture, fragTexCoord).rgb;
//     vec3 srcLin  = toLinear(srcSRGB);

//     // --- Basic bloom (bright-pass + small blur, all in linear) ---
//     vec3 bloomLin   = vec3(0.0);
//     float weightSum = 0.0;

//     // Fixed threshold & 5x5 kernel
//     const float threshold = 0.8; // in linear luminance
//     for (int y = -2; y <= 2; ++y) {
//         for (int x = -2; x <= 2; ++x) {
//             vec2 uv = fragTexCoord + vec2(x, y) * texelSize;
//             vec3 s   = toLinear(texture(sceneTexture, uv).rgb);

//             float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
//             // Soft-ish weight: fade in above threshold
//             float w = smoothstep(threshold, threshold + 1.0, lum);

//             bloomLin   += s * w;
//             weightSum  += w;
//         }
//     }

//     if (weightSum > 0.0) {
//         bloomLin /= weightSum;
//     } else {
//         bloomLin = vec3(0.0);
//     }

//     // Combine scene + bloom in linear space
//     vec3 resultLin = srcLin + bloomLin * bloomStrength;

//     // --- Tone mapping (still linear) ---
//     vec3 mappedLin = resultLin * uExposure;   // exposure pre-scale

//     if (uToneMapOperator == 1) {
//         mappedLin = ToneMapACES(mappedLin);
//     } else if (uToneMapOperator == 2) {
//         mappedLin = ToneMapReinhard(mappedLin);
//     }
//     // 0 = off (just exposure)

//     // Convert back to sRGB for display & vignette
//     vec3 resultSRGB = toSRGB(mappedLin);

//     // --- Vignette (in sRGB / display space) ---
//     vec2 centered = fragTexCoord - vec2(0.5);
//     float dist = clamp(length(centered) / 0.7071, 0.0, 1.0); // 0 at center, 1 at corners
//     float v = smoothstep(0.4, 1.0, dist);                    // feathered edge
//     resultSRGB = mix(resultSRGB, vec3(0.0), v * vignetteStrength);

//     // Final output
//     finalColor = vec4(clamp(resultSRGB, 0.0, 1.0), 1.0);
// }


// #version 330

// in vec2 fragTexCoord;
// out vec4 finalColor;

// uniform sampler2D sceneTexture;
// uniform vec2  resolution;
// uniform float bloomStrength;   // overall intensity
// uniform vec3  bloomColor;      // optional global tint (set to vec3(1.0) to keep scene hue)
// uniform float vignetteStrength;
// uniform float aaStrength;
// uniform float uSaturation;

// // NEW: thresholded bloom controls (linear space!)
// uniform float bloomThreshold;  // e.g. 1.0 in sRGB ≈ ~0.8 linear; start around 0.7–1.2
// uniform float bloomKnee;       // soft knee width (0.0–0.5), try 0.25

// // NEW: tone mapping controls
// uniform float uExposure;          // 0.5–2.0 typical (default 1.0)
// uniform int   uToneMapOperator;   // 0=Off, 1=ACES, 2=Reinhard

// uniform float lavaBoost; // try 0.8–2.0

// // Helpers
// float isLavaColor(vec3 sLin) {
//     // red-dominant & not too dark
//     float rDom = step(1.15*max(sLin.g, sLin.b), sLin.r);
//     float lum  = dot(sLin, vec3(0.2126, 0.7152, 0.0722));
//     return rDom * smoothstep(0.2, 1.2, lum);
// }

// vec3 applySaturation(vec3 rgb, float sat) {
//     float luma = dot(rgb, vec3(0.299, 0.587, 0.114));
//     return mix(vec3(luma), rgb, sat);
// }

// // Approx sRGB <-> linear (good enough)
// vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }
// vec3 toSRGB  (vec3 c) { return pow(c, vec3(1.0/2.2)); }

// // Soft-threshold weight (on luminance), returns 0..1
// float brightWeight(float lumLin, float thresh, float knee) {
//     // below (thresh - knee) -> 0
//     // above (thresh + knee) -> 1
//     return smoothstep(thresh - knee, thresh + knee, lumLin);
// }

// // --- Tone mapping operators (expect linear) ---

// // ACES (Hable's approximation to RRT+ODT)
// vec3 ToneMapACES(vec3 x) {
//     // constants tuned for approximate ACES curve
//     float a = 2.51;
//     float b = 0.03;
//     float c = 2.43;
//     float d = 0.59;
//     float e = 0.14;
//     return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
// }

// // Simple Reinhard
// vec3 ToneMapReinhard(vec3 x) {
//     return x / (1.0 + x);
// }

// void main() {
//     vec2 texelSize = 1.0 / resolution;

//     // Read original in sRGB for display path, but keep a linear copy for bloom/AA
//     vec3 srcSRGB = texture(sceneTexture, fragTexCoord).rgb;
//     vec3 srcLin  = toLinear(srcSRGB);

//     // --- Bright-pass + blur (all in linear) ---
//     vec3 blurLin = vec3(0.0);
//     float weightSum = 0.0;
//     //5x5
//     for (int y = -2; y <= 2; ++y) {
//         for (int x = -2; x <= 2; ++x) {
//             vec2 uv = fragTexCoord + vec2(x, y) * texelSize;

//             vec3 sSRGB = texture(sceneTexture, uv).rgb;
//             vec3 sLin  = toLinear(sSRGB);

//             // luminance in linear space
//             float lum = dot(sLin, vec3(0.2126, 0.7152, 0.0722));

//             // soft threshold mask
//             //float w = brightWeight(lum, bloomThreshold, bloomKnee);
//             float w = brightWeight(lum, bloomThreshold, bloomKnee);
//             w += isLavaColor(sLin) * lavaBoost;   // push lava into bloom

//             // keep original hue of the bright part (optionally tint later)
//             blurLin += sLin * w;
//             weightSum += w;
//         }
//     }

//     // (after the 5×5 finish, still in linear space)
//     vec3 ring = vec3(0.0); float ringSum = 0.0;
//     vec2 offs[4] = vec2[](vec2(4,0), vec2(-4,0), vec2(0,4), vec2(0,-4));
//     for (int i = 0; i < 4; ++i) {
//         vec2 uv = fragTexCoord + offs[i] * texelSize;
//         vec3 sLin = toLinear(texture(sceneTexture, uv).rgb);
//         float lum = dot(sLin, vec3(0.2126, 0.7152, 0.0722));
//         float w   = brightWeight(lum, bloomThreshold, bloomKnee)
//                 + isLavaColor(sLin) * (lavaBoost * 0.75);
//         ring    += sLin * w;
//         ringSum += w;
//     }
//     if (ringSum > 0.0) ring /= ringSum, blurLin = mix(blurLin, ring, 0.35);
    
//     if (weightSum > 0.0) blurLin /= weightSum;

//     // Mild pre-desat to avoid “milky” bloom; stays in linear
//     blurLin = toLinear(applySaturation(toSRGB(blurLin), 0.95));

//     // Optional global tint
//     blurLin *= bloomColor;

//     // --- Combine (in linear) ---
//     vec3 resultLin = srcLin;
//     // Mix and a small additive push for “glow”
//     resultLin = mix(resultLin, blurLin, bloomStrength * 0.7);
//     resultLin += blurLin * (bloomStrength * 0.3);

//     // --- Vignette (do in sRGB-ish domain for look, then convert back) ---
//     vec3 tmpSRGB = toSRGB(resultLin);
//     vec2 centered = fragTexCoord - vec2(0.5);
//     float dist = clamp(length(centered) / 0.7071, 0.0, 1.0);
//     float vignette = smoothstep(0.4, 1.0, dist);
//     tmpSRGB = mix(tmpSRGB, vec3(0.0), vignette * vignetteStrength);

//     // --- “Bootleg AA”: blur in linear, mix back ---
//     // Convert back to linear for AA mix
//     resultLin = toLinear(tmpSRGB);

//     vec3 aaBlurLin = vec3(0.0);
//     float aaSum = 0.0;
//     for (int y = -1; y <= 1; ++y) {
//         for (int x = -1; x <= 1; ++x) {
//             vec2 uv = fragTexCoord + vec2(x, y) * texelSize;
//             vec3 sLin = toLinear(texture(sceneTexture, uv).rgb);
//             aaBlurLin += sLin;
//             aaSum += 1.0;
//         }
//     }
//     aaBlurLin /= aaSum;
//     resultLin = mix(resultLin, aaBlurLin, aaStrength);

//     // --- Tone mapping (still in linear!) ---
//     vec3 mappedLin = resultLin * uExposure; // exposure pre-scale
//     if (uToneMapOperator == 1) {
//         mappedLin = ToneMapACES(mappedLin);
//     } else if (uToneMapOperator == 2) {
//         mappedLin = ToneMapReinhard(mappedLin);
//     }
//     // else 0 = Off (leave mappedLin as-is)

//     // Back to sRGB for final shapers
//     vec3 resultSRGB = toSRGB(mappedLin);

//     // Post-bloom / post-tonemap saturation
//     resultSRGB = applySaturation(resultSRGB, uSaturation);

//     // Output
//     finalColor = vec4(clamp(resultSRGB, 0.0, 1.0), 1.0);
// }

