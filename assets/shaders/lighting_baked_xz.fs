#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;

out vec4 finalColor;

uniform sampler2D texture0;   // albedo
uniform vec4 colDiffuse;

// Lightmap now comes from emission slot:
uniform sampler2D texture4;   // gDynamic.tex bound per-material

uniform vec4 gridBounds;
uniform float dynStrength;
uniform float ambientBoost;

void main() 
{
    vec4 baseS = texture(texture0, vUV);
    vec3 base  = baseS.rgb * colDiffuse.rgb;
    float alpha = baseS.a * colDiffuse.a;

    float u = (vWorldPos.x - gridBounds.x) * gridBounds.z;
    float v = (vWorldPos.z - gridBounds.y) * gridBounds.w;
    vec2 lmUV = clamp(vec2(u, v), 0.0, 1.0);

    vec3 dyn = texture(texture4, lmUV).rgb;

    vec3 L = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

    finalColor = vec4(base * L, alpha);
}
