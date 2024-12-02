#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive: require
#include "mesh_gpu.h"
#include "math.h"

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec3 inWorldPos;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outputColor;

layout (binding = 3) readonly uniform Transform 
{
    TransformData trans;
};

void main()
{
    const float p = 150.0;
    vec3 lights[4] = 
    {
        vec3(-p, -p*0.5f, -p),
        vec3(-p, -p*0.5f,  p),
        vec3( p, -p*0.5f,  p),
        vec3( p, -p*0.5f, -p),
    };
    vec3 norm = normalize(inNormal);
    vec3 v = normalize(trans.cameraPos - inWorldPos);

    float roughness = 0.3;
    float matalness = 0.6;

    // Specular contribution
    vec3 lightOutput = vec3(0.0);
    for (int i = 0; i < lights.length(); i++) {
        vec3 lightDir = normalize(lights[i].xyz - inWorldPos);
        lightOutput += BRDF(lightDir, v, norm, GOLD, matalness, roughness);
    };

    // Combine with ambient
    vec3 color = GOLD * 0.02;
    color += lightOutput;

    // Gamma correction
    color = pow(color, vec3(0.4545));

    outputColor = vec4(color, 1.0);
}