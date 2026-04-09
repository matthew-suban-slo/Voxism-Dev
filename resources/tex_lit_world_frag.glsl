#version 330 core
in vec3 fragPos;
in vec3 fragNor;
in vec2 fragTex;

uniform sampler2D Texture0;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform vec3 lightColor;
uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float shininess;
uniform vec3 tintColor;
uniform float emissiveStrength;
uniform vec3 emissiveColor;
uniform int useEmissiveMap;

out vec4 color;

void main()
{
	vec3 N = normalize(fragNor);
	vec3 L = normalize(lightPos - fragPos);
	vec3 Vdir = normalize(camPos - fragPos);
	vec3 H = normalize(L + Vdir);

	vec4 texel = texture(Texture0, fragTex);
	vec3 albedo = texel.rgb * tintColor;

	float diff = max(dot(N, L), 0.0);
	float spec = 0.0;
	if (diff > 0.0)
		spec = pow(max(dot(N, H), 0.0), shininess);

	vec3 ambient = matAmbient * albedo * lightColor;
	vec3 diffuse = matDiffuse * albedo * lightColor * diff;
	vec3 specular = matSpecular * lightColor * spec;
	vec3 emissiveMap = (useEmissiveMap == 1) ? texel.rgb : vec3(1.0);
	vec3 emissive = emissiveColor * emissiveStrength * emissiveMap;

	color = vec4(ambient + diffuse + specular + emissive, 1.0);
}
