#version 330 core

in vec2 TexCoords;
out float FragColor;

// depth image of the scene (how far away each pixel's surface is)
uniform sampler2D depthTex;
// tiny 4x4 texture of random rotation vectors, tiled across the screen
uniform sampler2D noiseTex;
// 64 pre-generated "tennis ball" directions in a hemisphere
uniform vec3 samples[64];
// camera projection matrix (3d -> screen)
uniform mat4 projection;
// inverse of projection (screen -> 3d), used to reconstruct positions
uniform mat4 invProjection;
// how many times the noise texture tiles across the screen (screenRes / 4)
uniform vec2 noiseScale;
// how far each sample travels from the surface (in world units)
uniform float radius;
// tiny depth offset to stop a surface from occluding itself
uniform float bias;

// takes a screen uv and returns the 3d position of whatever surface is there
vec3 reconstructViewPos(vec2 uv) {
    // read how far away the surface is at this pixel
    float depth = texture(depthTex, uv).r;
    // depth is stored as [0,1], convert it to ndc range [-1,1]
    vec4 ndcPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    // reverse the camera projection to get the actual 3d position back
    vec4 viewPos = invProjection * ndcPos;
    // perspective divide (required after matrix multiply)
    return viewPos.xyz / viewPos.w;
}

void main() {

    // --- where are we? ---
    // get the 3d position of the surface at this pixel
    vec3 fragPos = reconstructViewPos(TexCoords);

    // --- which way is "up" from this surface? ---
    // grab the 3d position of the pixel one step to the right and one step up
    vec3 ddx = reconstructViewPos(TexCoords + vec2(dFdx(TexCoords.x), 0.0)) - fragPos;
    vec3 ddy = reconstructViewPos(TexCoords + vec2(0.0, dFdy(TexCoords.y))) - fragPos;
    // cross product of two vectors lying on the surface = vector pointing straight out of it
    vec3 normal = normalize(cross(ddx, ddy));

    // --- randomize the throw direction so we don't get repeating patterns ---
    // sample the noise texture to get a random rotation vector for this pixel
    vec3 randomVec = normalize(texture(noiseTex, TexCoords * noiseScale).xyz);
    // gram-schmidt: make randomVec perpendicular to the normal (so it lies flat on the surface)
    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    // TBN is a rotation matrix: transforms sample directions to align with this surface + random spin
    mat3 TBN       = mat3(tangent, bitangent, normal);

    // --- throw 64 balls and count how many hit something ---
    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {

        // rotate the sample direction to face this surface, scale by radius
        // samplePos is where ball #i lands in 3d space
        vec3 samplePos = fragPos + TBN * samples[i] * radius;

        // figure out which screen pixel this 3d point corresponds to
        // (multiply by projection matrix, then remap from [-1,1] to [0,1])
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;           // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // ndc [-1,1] -> uv [0,1]

        // look up the actual surface depth at that screen pixel
        float sampleDepth = reconstructViewPos(offset.xy).z;

        // only count occlusion from surfaces within our radius
        // (smoothstep fades out contributions from far-away geometry)
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

        // if the real surface is at the same depth or closer than our sample point,
        // something is blocking it -> occluded. bias prevents self-intersection.
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    // divide hits by 64 to get a 0->1 ratio, then invert:
    // 1.0 = no occlusion (bright open area)
    // 0.0 = fully occluded (dark corner/crease)
    FragColor = 1.0 - (occlusion / 64.0);
}
