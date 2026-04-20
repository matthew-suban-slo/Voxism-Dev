#version 330 core

// Bright-pass filter: extract pixels above a luminance threshold for bloom.
// Uses a soft-knee curve so the transition is not a hard cut.

in  vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTex;

const float THRESHOLD = 0.45;
const float KNEE      = 0.22;

void main()
{
    vec3  color      = texture(sceneTex, TexCoords).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Soft-knee: pixels just below the threshold get a partial contribution
    float rq   = clamp(brightness - THRESHOLD + KNEE, 0.0, 2.0 * KNEE);
    rq         = (rq * rq) / (4.0 * KNEE + 0.00001);
    float w    = max(rq, brightness - THRESHOLD) / max(brightness, 0.0001);

    FragColor = vec4(color * w, 1.0);
}
