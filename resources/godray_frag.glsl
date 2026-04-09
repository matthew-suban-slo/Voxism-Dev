#version 330 core

// Screen-space god rays (crepuscular rays) via radial blur.
// Sample the scene texture marching from each pixel toward the sun.
// Bright sky pixels accumulate; dark occlusion pixels do not.

in  vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTex;
uniform vec2  sunPos;   // sun position in [0,1] screen UV
uniform float time;     // for subtle shimmer

const int   NUM_SAMPLES  = 80;
const float EXPOSURE     = 0.62;
const float DECAY        = 0.970;
const float DENSITY      = 0.62;
const float WEIGHT       = 0.013;
// Orange sky has luminance ~0.35–0.55 — lower threshold to catch warm sunset pixels
// God-ray source is sky-gradient + sun (see sunmask_frag.glsl), not full scene.
const float SKY_THRESH   = 0.22;

void main()
{
    vec2 tc    = TexCoords;
    vec2 delta = (tc - sunPos) * (DENSITY / float(NUM_SAMPLES));

    vec3  accumulated       = vec3(0.0);
    float illuminationDecay = 1.0;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        tc -= delta;
        tc  = clamp(tc, vec2(0.0), vec2(1.0));

        vec3  s   = texture(sceneTex, tc).rgb;
        float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));

        // Smooth mask: only sky-bright pixels feed the rays
        float mask = smoothstep(SKY_THRESH, 0.75, lum);

        accumulated       += s * mask * illuminationDecay * WEIGHT;
        illuminationDecay *= DECAY;
    }

    // Warm golden-hour sun tint + very faint shimmer
    vec3  sunTint = vec3(1.0, 0.87, 0.52);
    float flicker = 1.0 + 0.035 * sin(time * 4.1 + TexCoords.x * 13.0 + TexCoords.y * 7.0);

    FragColor = vec4(accumulated * EXPOSURE * flicker * sunTint, 1.0);
}
