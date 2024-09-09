#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    float r = texelColor.r;
    float g = texelColor.g;
    float b = texelColor.b;

    float R = (r * 13 + g * 2 + b) / 16;
    float G = (g *  3 + b) / 4;
    float B = (r *  3 + g * 2 + b * 11) / 16;

    // Calculate final fragment color
    finalColor = vec4(R, G, B, texelColor.a);
}