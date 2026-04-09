#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2D skybox;

const float PI = 3.14159265359;

void main()
{
	vec3 dir = normalize(TexCoords);
	float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
	float v = asin(clamp(dir.y, -1.0, 1.0)) / PI + 0.5;
	vec3 sky = texture(skybox, vec2(u, v)).rgb;
	sky *= 1.15;
	FragColor = vec4(sky, 1.0);
}
