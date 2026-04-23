#version 330 core

in vec2 TexCoords;
out float FragColor;

// the raw noisy ssao output from ssao_frag
uniform sampler2D ssaoInput;
// size of one pixel in uv space (1/screenWidth, 1/screenHeight)
uniform vec2 texelSize;

void main() {
    float result = 0.0;

    // sample a 5x5 grid of neighboring pixels and average them
    // this smooths out the per-pixel noise introduced by the random rotation texture
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            // offset in uv space to reach this neighbor
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, TexCoords + offset).r;
        }
    }

    // divide by 25 (5x5 = 25 samples) to get the average
    FragColor = result / 25.0;
}
