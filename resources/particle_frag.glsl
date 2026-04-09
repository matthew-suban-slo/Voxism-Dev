#version 330 core

out vec4 FragColor;
uniform vec3 particleColor;

void main()
{
	vec2 p = gl_PointCoord * 2.0 - 1.0;
	float r2 = dot(p, p);
	if (r2 > 1.0)
		discard;
	float alpha = (1.0 - r2) * 0.85;
	FragColor = vec4(particleColor, alpha);
}
