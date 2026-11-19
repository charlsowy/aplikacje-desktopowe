uniform sampler2D texture;
uniform float blurRadius;
uniform vec2 texSize;

void main()
{
    vec2 texel = 1.0 / texSize;
    int r = int(blurRadius + 0.5);

    vec4 sum = vec4(0.0);
    float count = 0.0;

    for (int x = -10; x <= 10; x++)
    {
        if (x < -r || x > r) continue;
        for (int y = -10; y <= 10; y++)
        {
            if (y < -r || y > r) continue;
            sum += texture2D(texture, gl_TexCoord[0].xy + vec2(float(x), float(y)) * texel);
            count += 1.0;
        }
    }

    gl_FragColor = sum / count;
}
