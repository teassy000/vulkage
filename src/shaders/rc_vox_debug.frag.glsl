#version 450

#extension GL_GOOGLE_include_directive: require
#include "debug_gpu.h"

layout(location = 0) in flat uint in_instId;
layout(location = 1) out vec3 in_color;
layout(location = 0) out vec4 outColor;

void main()
{
    uint mhash = hash(in_instId);
    vec4 color = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;

    outColor = vec4(in_color, 1.f);
}