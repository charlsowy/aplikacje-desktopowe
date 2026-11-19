uniform sampler2D texture;

void main()
{
    vec4 pixelColor = texture2D(texture, gl_TexCoord[0].xy);


    float gray = (pixelColor.r + pixelColor.g + pixelColor.b)/3;


    gl_FragColor = vec4(vec3(gray), pixelColor.a);
}