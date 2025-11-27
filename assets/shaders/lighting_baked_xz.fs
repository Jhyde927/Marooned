#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;
in vec3 vNormal;

out vec4 finalColor;

uniform sampler2D texture0;   // Albedo
uniform vec4      colDiffuse;

// Lightmap from emission slot
uniform sampler2D texture4;

// { minX, minZ, invSizeX, invSizeZ }
uniform vec4 gridBounds;
uniform float dynStrength;
uniform float ambientBoost;

void main()
{
    // --- Albedo ---
    vec4 baseS = texture(texture0, vUV);
    vec3 base  = baseS.rgb * colDiffuse.rgb;
    float alpha = baseS.a * colDiffuse.a;

    // --- Lightmap UV ---
    float u = (vWorldPos.x - gridBounds.x) * gridBounds.z;
    float v = (vWorldPos.z - gridBounds.y) * gridBounds.w;
    vec2 lmUV = clamp(vec2(u, v), 0.0, 1.0);

    // --- Dynamic lightmap sample ---
    vec3 dyn = texture(texture4, lmUV).rgb;
    vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

    // --- Wall detection using normal ---
    vec3 N = normalize(vNormal);
    float wallFactor = 1.0 - abs(N.y);   // 1.0 = vertical (wall), 0.0 = floor/ceiling

    // Wall brightness boost
    float wallBoost = 1.1;               // tweak to taste
    L *= mix(1.0, wallBoost, wallFactor);

    // --- Warm tint near bright lights ---
    float lightLevel = max(L.r, max(L.g, L.b)); // overall brightness proxy
    vec3  torchColor = vec3(1.0, 0.75, 0.6);    // desired warm glow

    // base tint (your current behavior)
    float baseTint = smoothstep(0.1, 1.0, lightLevel);

    // extra boost very close to the source
    float hotspot = smoothstep(0.07, 1.0, lightLevel); // only when really bright
    float extraHot = 0.9;                             // how much stronger near center

    float tintAmount = baseTint * (1.0 + hotspot * extraHot);
    // optional: clamp so it doesn't go insane
    tintAmount = clamp(tintAmount, 0.0, 2.0);

    vec3 lightTint = mix(vec3(1.0), torchColor, tintAmount);

    ////CONTROL WALL COLOR - desaturate then color. if normal = wall
    // compute luminance to get a gray version of the texture
    // float luma = dot(base, vec3(0.299, 0.587, 0.114));
    // vec3 gray  = vec3(luma);

    // // desaturate walls toward gray
    // float desatAmount   = 0.99; // 0..1
    // vec3  lessPurple    = mix(base, gray, desatAmount * wallFactor);

    // // now tint toward your wallColor
    // vec3 wallColor      = vec3(0.1, 0.2, 0.7);
    // float wallTintStrength = 0.25;
    // float wallTintAmount   = wallTintStrength * wallFactor;

    // vec3 baseWallTweaked = mix(lessPurple, wallColor, wallTintAmount);

    // vec3 lit = baseWallTweaked * (L * lightTint);
    // Apply tint to light before modulating albedo
    vec3 lit = base * (L * lightTint);


    finalColor = vec4(lit, alpha);
}




// #version 330

// in vec2 vUV;
// in vec3 vWorldPos;
// in vec4 vColor;
// in vec3 vertexNormal;  // <-- add this

// out vec4 finalColor;

// uniform sampler2D texture0;   // albedo
// uniform vec4 colDiffuse;

// // Lightmap now comes from emission slot:
// uniform sampler2D texture4;   // gDynamic.tex bound per-material

// uniform vec4 gridBounds;
// uniform float dynStrength;
// uniform float ambientBoost;

// out vec3 vNormal;      // <-- pass to fragment

// void main() 
// {
//     vec4 baseS = texture(texture0, vUV);
//     vec3 base  = baseS.rgb * colDiffuse.rgb;
//     float alpha = baseS.a * colDiffuse.a;

//     float u = (vWorldPos.x - gridBounds.x) * gridBounds.z;
//     float v = (vWorldPos.z - gridBounds.y) * gridBounds.w;
//     vec2 lmUV = clamp(vec2(u, v), 0.0, 1.0);

//     vec3 dyn = texture(texture4, lmUV).rgb;

//     vec3 L = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

//     finalColor = vec4(base * L, alpha);
// }
