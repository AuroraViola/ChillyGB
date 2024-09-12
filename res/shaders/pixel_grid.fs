#version 330
precision highp float;
in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

vec2 res = vec2(160, 144);
float thickness = 0.35;
float antiAliasing = 0.9;

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);

    vec2 lines = smoothstep(1.0-antiAliasing, 1.0, abs(fract(fragTexCoord * res)*(1/thickness)));
    float line = dot(lines, vec2(1.0)) - 1.0;

    finalColor = vec4(texelColor.rgb, line);
}