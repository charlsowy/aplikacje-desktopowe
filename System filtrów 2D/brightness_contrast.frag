uniform sampler2D texture;
uniform float brightness;
uniform float contrast;

void main()
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

    vec3 color = pixel.rgb;

    color += brightness;

    color = (color - 0.5) * contrast + 0.5;

    color = clamp(color, 0.0, 1.0);

    gl_FragColor = vec4(color, pixel.a);
}
