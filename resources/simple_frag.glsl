#version 330 core
in vec3 fragPos;
in vec3 fragNor;

uniform vec3 lightPos;
uniform vec3 camPos;
uniform vec3 lightColor;

uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float shininess;

out vec4 color;

void main()
{
	vec3 N = normalize(fragNor);
	vec3 L = normalize(lightPos - fragPos);
	vec3 Vdir = normalize(camPos - fragPos);
	vec3 H = normalize(L + Vdir);

	float diff = max(dot(N, L), 0.0);
	float spec = 0.0;
	if (diff > 0.0)
		spec = pow(max(dot(N, H), 0.0), shininess);

	vec3 ambient = matAmbient * lightColor;
	vec3 diffuse = matDiffuse * lightColor * diff;
	vec3 specular = matSpecular * lightColor * spec;

	vec3 rgb = ambient + diffuse + specular;
	color = vec4(rgb, 1.0);
}
