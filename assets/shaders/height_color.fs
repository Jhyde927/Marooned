#version 330

in vec3 fragNormal;
in vec3 fragPosition;

out vec4 finalColor;

uniform vec3 cameraPos;

uniform vec2 u_WorldMinXZ;   // (minX, minZ)
uniform vec2 u_WorldSizeXZ;  // (sizeX, sizeZ)

// Textures bound via material slots
uniform sampler2D texGrass;
uniform sampler2D texSand;
uniform sampler2D textureOcclusion;   // palm shadow mask (RenderTexture)

// Tiling controls
uniform float grassTiling;  // e.g. 60.0
uniform float sandTiling;   // e.g. 20.0

// Fog / sky
uniform vec3  u_SkyColorTop;      // e.g. vec3(0.55, 0.75, 1.00)
uniform vec3  u_SkyColorHorizon;  // e.g. vec3(0.72, 0.82, 0.95)
uniform float u_FogStart;         // e.g. 4000.0
uniform float u_FogEnd;           // e.g. 18000.0
uniform float u_SeaLevel;         // e.g. 60.0
uniform float u_FogHeightFalloff; // e.g. 0.002

void main()
{
    float height = fragPosition.y;

    // ---- World-space UVs (0..1 across island), then tile
    vec2 uvIsland = (fragPosition.xz - u_WorldMinXZ) / u_WorldSizeXZ;
    uvIsland = clamp(uvIsland, 0.0, 1.0);

    vec2 uvGrass = uvIsland * grassTiling;
    vec2 uvSand  = uvIsland * sandTiling;

    // ---- Sample base textures
    vec3 grassTex = texture(texGrass, uvGrass).rgb;
    vec3 sandTex  = texture(texSand,  uvSand ).rgb;

    // ---- Height-based blend: water -> sand -> grass
    vec3 waterColor = vec3(0.10, 0.60, 1.00);

    float tWaterSand = smoothstep(30.0, 80.0, height);
    vec3 baseWS = mix(waterColor, sandTex, tWaterSand);

    float tSandGrass = smoothstep(100.0, 130.0, height);
    vec3 color = mix(baseWS, grassTex, tSandGrass);

    // Optional overall brightness (feel free to tweak/remove)
    color *= 0.85;

    // ---- Palm shadow mask (note Y flip for RenderTexture)
    vec2 uvMask = vec2(uvIsland.x, 1.0 - uvIsland.y);
    float mask = texture(textureOcclusion, uvMask).r; // 1=no shadow, 0=dark
    color *= mask;

    // ---- Sky-colored fog (no grayscale desat)
    float dist      = length(fragPosition - cameraPos);
    float fogDist   = smoothstep(u_FogStart, u_FogEnd, dist);

    float h         = max(fragPosition.y - u_SeaLevel, 0.0);
    float fogHeight = exp(-h * u_FogHeightFalloff);   // heavier near sea level
    float fog       = clamp(fogDist * fogHeight, 0.0, 1.0);

    // Simple sky gradient: bias toward horizon near sea level
    float skyT     = clamp((cameraPos.y - u_SeaLevel) / 600.0, 0.0, 1.0);
    vec3  skyColor = mix(u_SkyColorHorizon, u_SkyColorTop, skyT);

    color = mix(color, skyColor, fog);

    finalColor = vec4(color, 1.0);
}
