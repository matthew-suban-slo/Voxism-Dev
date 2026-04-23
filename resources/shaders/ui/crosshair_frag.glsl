#version 330 core

uniform vec3 crossColor;
out vec4 FragColor;

void main()
{
	FragColor = vec4(crossColor, 1.0);
}