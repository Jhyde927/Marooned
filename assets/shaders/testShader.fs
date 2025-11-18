#version 330
//bare bones lighting_baked_xz.fs
in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;

out vec4 finalColor;

uniform sampler2D dynamicGridTex;
uniform vec4 gridBounds;

void main()
{
    float u = (vWorldPos.x - gridBounds.x) * gridBounds.z;
    float v = (vWorldPos.z - gridBounds.y) * gridBounds.w;
    vec2 lmUV = clamp(vec2(u, v), 0.0, 1.0);

    vec3 dyn = texture(dynamicGridTex, lmUV).rgb;
    finalColor = vec4(dyn, 1.0);
}

// Try sampling texture unit 0 explicitly
vec4 t0 = texture(texture0, vec2(0.5, 0.5));

// Try sampling texture unit 1 explicitly
uniform sampler2D dynamicGridTex;
vec4 t1 = texture(dynamicGridTex, vec2(0.5, 0.5));

finalColor = vec4(t0.rgb, 1.0);  // should show albedo tex color
// finalColor = vec4(t1.rgb, 1.0);  // shows black currently
