#version 330 core
in vec2 vUv;
out vec4 FragColor;

uniform int uUseTexture;
uniform sampler2D uDiffuse;
uniform vec3 uColor;

void main()
{
    if (uUseTexture != 0)
        FragColor = texture(uDiffuse, vUv);
    else
        FragColor = vec4(uColor, 1.0);
}
