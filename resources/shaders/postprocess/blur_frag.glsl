#version 330 core

// Separable 9-tap Gaussian blur.
// Call once with horizontal=1 (blur in X), then again with horizontal=0 (blur in Y).

in  vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D image;
uniform int       horizontal;
uniform vec2      texelSize;   // vec2(1/width, 1/height)

const float W[5] = float[](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{
    vec2 dir    = (horizontal == 1) ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    vec3 result = texture(image, TexCoords).rgb * W[0];

    for (int i = 1; i < 5; ++i)
    {
        result += texture(image, TexCoords + float(i) * dir).rgb * W[i];
        result += texture(image, TexCoords - float(i) * dir).rgb * W[i];
    }

    FragColor = vec4(result, 1.0);
}
