#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float curveAmount;
uniform float gutterShadow;
uniform float edgeShadow;
uniform float pageLayers;

out vec4 finalColor;

float roundedBoxMask(vec2 uv, float radius)
{
    vec2 centered = uv - vec2(0.5);
    vec2 halfSize = vec2(0.5) - vec2(radius);

    vec2 q = abs(centered) - halfSize;

    float distanceFromShape =
        length(max(q, vec2(0.0))) +
        min(max(q.x, q.y), 0.0) -
        radius;

    return 1.0 - smoothstep(
        -0.002,
        0.002,
        distanceFromShape
    );
}

void main()
{
    vec2 uv = fragTexCoord;

    // Distance from the center gutter:
    // 0 at center, 1 at either outer edge.
    float fromCenter = abs(uv.x - 0.5) * 2.0;

    // Give the two-page spread a shallow curved appearance.
    // The vertical displacement is strongest toward the outer edges.
    float curve = fromCenter * fromCenter;
    uv.y += curve * curveAmount * (uv.y - 0.5);

    // Reject warped coordinates that leave the texture.
    if (uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0)
    {
        discard;
    }

    vec4 color = texture(texture0, uv) * fragColor;

    // Dark center gutter.
    float centerDistance = abs(uv.x - 0.5);
    float gutter = 1.0 - smoothstep(
        0.0,
        gutterShadow,
        centerDistance
    );

    color.rgb *= 1.0 - gutter * 0.32;

    // Gentle shadow around the outer page edges.
    float distanceToEdge = min(
        min(uv.x, 1.0 - uv.x),
        min(uv.y, 1.0 - uv.y)
    );

    float edge = 1.0 - smoothstep(
        0.0,
        edgeShadow,
        distanceToEdge
    );

    color.rgb *= 1.0 - edge * 0.18;

    // Thin bands along the bottom edge to imply stacked pages.
    float layerArea = 1.0 - smoothstep(
        0.0,
        pageLayers,
        1.0 - uv.y
    );

    float layerLines =
        0.5 + 0.5 * sin((1.0 - uv.y) * 900.0);

    layerLines = smoothstep(0.35, 0.75, layerLines);

    vec3 layerColor = vec3(0.40, 0.29, 0.13);

    color.rgb = mix(
        color.rgb,
        layerColor,
        layerArea * layerLines * 0.20
    );

    float pageMask = roundedBoxMask(fragTexCoord, 0.49);

    color.a *= pageMask;

    if (color.a < 0.01)
    {
        discard;
    }

    finalColor = color;
}