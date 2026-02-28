#version 330

// The mesh provides x/z in [-1..1]. We'll interpret that as NDC coordinates.
in vec3 vertexPosition;

out vec3 vWorldPos;
out vec2 vUV;
out float vHit;
out float vT;  

uniform mat4 u_ViewProj;     // Projection * View
uniform mat4 u_InvViewProj;  // inverse(u_ViewProj)
uniform vec3 u_CamPos;
uniform float u_WaterY;      // world-space water height
uniform float u_MaxDist;     // clamp ray distance (prevents extreme issues)
uniform float u_UVScale;     // world->uv scale (e.g. 0.002)

float RayPlaneIntersectY(vec3 ro, vec3 rd, float planeY)
{
    float denom = rd.y;
    if (abs(denom) < 1e-6) return -1.0;  // nearly parallel
    return (planeY - ro.y) / denom;
}

void main()
{
    vec2 ndc = vertexPosition.xz; // expected in [-1..1]

    // Unproject a ray through this NDC point by sampling near & far clip positions
    vec4 nearClip = vec4(ndc, -1.0, 1.0);
    vec4 farClip  = vec4(ndc,  1.0, 1.0);

    vec4 nearWorld4 = u_InvViewProj * nearClip;
    vec4 farWorld4  = u_InvViewProj * farClip;

    vec3 nearWorld = nearWorld4.xyz / nearWorld4.w;
    vec3 farWorld  = farWorld4.xyz  / farWorld4.w;

    // Ray origin/direction in world space
    vec3 ro = u_CamPos;
    //vec3 rd = normalize(farWorld - nearWorld);

    vec3 rd = normalize(farWorld - u_CamPos);

    // Intersect with horizontal water plane y = u_WaterY

    float t = RayPlaneIntersectY(ro, rd, u_WaterY);



    vHit = (t > 0.0) ? 1.0 : 0.0;

    // If no hit (parallel / looking upward), clamp to a far point
    if (t < 0.0) t = u_MaxDist;
    t = clamp(t, 0.0, u_MaxDist);


    vT = t;

    vec3 worldPos = ro + rd * t;

    vWorldPos = worldPos;
    vUV = worldPos.xz * u_UVScale;

    // Now rasterize this world point
    gl_Position = u_ViewProj * vec4(worldPos, 1.0);
}