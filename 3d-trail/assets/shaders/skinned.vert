#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in ivec4 aBoneIds;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec2 aUv;

uniform mat4 uModel;
uniform mat4 uViewProj;

const int MAX_BONES = 64;
uniform mat4 uBones[MAX_BONES];

out vec2 vUv;

void main()
{
    mat4 skin =
        uBones[aBoneIds[0]] * aWeights[0] +
        uBones[aBoneIds[1]] * aWeights[1] +
        uBones[aBoneIds[2]] * aWeights[2] +
        uBones[aBoneIds[3]] * aWeights[3];
    vec4 posL = skin * vec4(aPos, 1.0);
    vUv = aUv;
    gl_Position = uViewProj * uModel * posL;
}
