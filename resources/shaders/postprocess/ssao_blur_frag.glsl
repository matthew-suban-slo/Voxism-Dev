#version 330 core

in vec2 TexCoords;
out float FragColor;

uniform sampler2D ssaoInput;
uniform vec2 texelSize;

void main() {
    float result = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, TexCoords + offset).r;
        }
    }
    FragColor = result / 25.0;
}
