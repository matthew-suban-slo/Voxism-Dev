#version 330 core
// Textures
uniform sampler3D colorTex;

// General Chunk Data
uniform vec3 chunkWorldPos;
//uniform float chunkSizeMeters;
uniform float voxelSizeMeters;

// Lighting
uniform vec3 lightPos; //added
uniform vec3 camPos; // added
uniform vec3 lightColor; //added

// vertex shader ins.
flat in vec3 normal;
in vec3 worldPos;

out vec4 color;

void main()
{
    // get voxel color.
    ivec3 localCoord = ivec3(floor((worldPos - chunkWorldPos-normal*0.5*voxelSizeMeters)/voxelSizeMeters));
    vec4 voxColor = texelFetch(colorTex, localCoord, 0);
    
    // hard coded until ID lookup can be used.
    //vec3 matSpecular = clamp(voxColor.rgb + 0.5, 0.0, 1.0);
    vec3 matSpecular = vec3(0.15);
    vec3 matDiffuse = voxColor.rgb*1.0;
    vec3 matAmbient = voxColor.rgb*0.08;
    float shininess = 30;

    // Lighting Equations
    vec3 N = normalize(normal);
    //used to calculate the blinn-phong color per voxel.
    vec3 voxelPos = (floor(worldPos/voxelSizeMeters))*voxelSizeMeters+0.5*voxelSizeMeters;
    //vec3 voxelPos = (floor(worldPos/(voxelSizeMeters*2)))*(voxelSizeMeters*2)+0.5*(voxelSizeMeters*2); //lighting 2x voxel size.
	vec3 L = normalize(lightPos- voxelPos);
	vec3 Vdir = normalize(camPos - voxelPos);
    //used to calculate the blinn-phong color normally
    //vec3 L = normalize(lightPos- worldPos);
	//vec3 Vdir = normalize(camPos - worldPos);

	vec3 H = normalize(L + Vdir);

	float diff = max(dot(N, L), 0.0);
	float spec = 0.0;
	if (diff > 0.0) {
        spec = pow(max(dot(N, H), 0.0), shininess);
        //spec = smoothstep(0.2, 1.0, spec);
    }
        
	vec3 ambient = matAmbient;
	vec3 diffuse = matDiffuse * lightColor * diff;
	vec3 specular = matSpecular * lightColor * spec;
    
    vec3 rgb = ambient + diffuse + specular;
    color = vec4(rgb, 1.0);
    
    //color = vec4(vec3(spec), 1.0);
    //color = texelFetch(colorTex, localCoord, 0);
}
