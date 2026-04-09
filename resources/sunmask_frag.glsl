#version 330 core

// Full-screen sky approximation + sun disk for god-ray radial blur input.
// The previous tiny sun-only mask had near-zero luminance everywhere else, so
// the blur accumulated nothing (godray_frag masks by luminance vs SKY_THRESH).

in vec2 TexCoords;
out vec4 FragColor;

uniform vec2 sunPos;

void main()
{
	// Vertical gradient: bright enough that luminance clears SKY_THRESH (~0.32) on most of the image.
	vec3 skyLow = vec3(0.18, 0.22, 0.34);
	vec3 skyHigh = vec3(0.52, 0.62, 0.88);
	float t = clamp(TexCoords.y, 0.0, 1.0);
	vec3 sky = mix(skyLow, skyHigh, t);

	vec2 d = TexCoords - sunPos;
	float r = length(d);
	float core = smoothstep(0.09, 0.0, r);
	float halo = smoothstep(0.42, 0.0, r) * 0.55;
	float sunMask = clamp(core + halo, 0.0, 1.0);
	vec3 sunColor = vec3(1.15, 1.05, 0.75);
	vec3 col = sky + sunMask * sunColor * 2.4;

	FragColor = vec4(col, 1.0);
}
