#version 330 core

in vec2 TexCoords;
out float FragColor;

uniform sampler2D depthTex;
uniform sampler2D noiseTex;
uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 invProjection;
uniform vec2 noiseScale;
uniform float radius;
uniform float bias;

// Reconstruct view-space position from depth
vec3 reconstructViewPos(vec2 uv) {
    float depth = texture(depthTex, uv).r;
    // Convert to NDC
    vec4 ndcPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    // Unproject to view space
    vec4 viewPos = invProjection * ndcPos;
    return viewPos.xyz / viewPos.w;
}

void main() {
    // Step 1: Reconstruct view-space position
    vec3 fragPos = reconstructViewPos(TexCoords);
    
    // Step 2: Estimate view-space normal from depth cross-derivatives
    vec3 ddx = reconstructViewPos(TexCoords + vec2(dFdx(TexCoords.x), 0.0)) - fragPos;
    vec3 ddy = reconstructViewPos(TexCoords + vec2(0.0, dFdy(TexCoords.y))) - fragPos;
    vec3 normal = normalize(cross(ddx, ddy));
    
    // Step 3: Build TBN matrix using noise texture
    vec3 randomVec = normalize(texture(noiseTex, TexCoords * noiseScale).xyz);
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);
    
    // Step 4-5: Sample hemisphere and test against depth buffer
    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {
        // Transform sample to view space
        vec3 samplePos = fragPos + TBN * samples[i] * radius;
        
        // Project sample to screen space
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        
        // Sample depth at projected position
        float sampleDepth = reconstructViewPos(offset.xy).z;
        
        // Range check + occlusion test
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    // Step 6: Normalize and invert (1.0 = no occlusion, 0.0 = fully occluded)
    FragColor = 1.0 - (occlusion / 64.0);
}
