#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    float globalPos_y = (fragTexCoord.y) * 144;
    float globalPos_x = (fragTexCoord.x) * 160;
    float wavePos = (sin(fract(globalPos_y)) > 0.125 && sin(fract(globalPos_y)) < 0.875) ? 1 : 0.9;
    wavePos *= (sin(fract(globalPos_x)) > 0.125 && sin(fract(globalPos_x)) < 0.875) ? 1 : 0.9;

    vec4 texelColor = texture(texture0, fragTexCoord);

    finalColor = mix(vec4(0.0, 0.0, 0.0, 1.0), texelColor, wavePos);
}