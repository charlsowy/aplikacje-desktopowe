uniform sampler2D texture;

void main()
{
    vec4 pixelColor = texture2D(texture, gl_TexCoord[0].xy);

    gl_FragColor = vec4(1.0 - pixelColor.rgb, pixelColor.a);
}