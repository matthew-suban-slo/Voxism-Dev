#version 330 core
layout(location = 0) in vec3 vertPos;

uniform mat4 P;
uniform mat4 V;
uniform float pointSize;

void main()
{
	vec4 viewPos = V * vec4(vertPos, 1.0);
	gl_Position = P * viewPos;
	float depth = max(0.25, -viewPos.z);
	gl_PointSize = pointSize / depth;
}
