#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform float animPhase;
uniform float moveBlend;

out vec3 fragPos;
out vec3 fragNor;
out vec2 fragTex;

void main()
{
	vec3 p = vertPos;
	// Procedural motion (bind pose only — approximates idle / jog when rigged mesh has no GPU skinning)
	float bob = sin(animPhase * 6.2831853 * 2.0) * 0.04 * moveBlend;
	float sway = sin(animPhase * 6.2831853) * 0.03 * moveBlend;
	p.y += bob;
	p.x += sway;
	float lean = sin(animPhase * 6.2831853 * 2.0) * 0.11 * moveBlend;
	float c = cos(lean);
	float s = sin(lean);
	vec3 pr = vec3(c * p.x - s * p.z, p.y, s * p.x + c * p.z);

	vec4 wp = M * vec4(pr, 1.0);
	fragPos = wp.xyz;
	mat3 N = mat3(transpose(inverse(M)));
	fragNor = normalize(N * vertNor);
	fragTex = vertTex;
	gl_Position = P * V * wp;
}
