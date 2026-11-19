uniform sampler2D texture;
uniform float threshold;
uniform vec2 texSize;

float lum(vec2 uv)
{
    vec3 c = texture2D(texture, uv).rgb;
    return (c.r+c.g+c.b)/3.0;
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec2 t = 1.0 / texSize;

    float tl = lum(uv + vec2(-t.x,  t.y));
    float tc = lum(uv + vec2( 0.0,  t.y));
    float tr = lum(uv + vec2( t.x,  t.y));
    float ml = lum(uv + vec2(-t.x,  0.0));
    float mr = lum(uv + vec2( t.x,  0.0));
    float bl = lum(uv + vec2(-t.x, -t.y));
    float bc = lum(uv + vec2( 0.0, -t.y));
    float br = lum(uv + vec2( t.x, -t.y));

    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -tl - 2.0 * tc - tr + bl + 2.0 * bc + br;

    float edge = clamp(length(vec2(gx, gy)), 0.0, 1.0);
    float v = step(threshold, edge);

    gl_FragColor = vec4(vec3(v), texture2D(texture, uv).a);
}
