#version 450

#extension GL_GOOGLE_include_directive: require
#include "debug_gpu.h"

layout(location = 0) in flat uint in_drawId;
layout(location = 0) out vec4 outColor;

void main()
{
    uint mhash = hash(in_drawId);
    vec4 color = vec4(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255), 255) / 255.0;

    outColor = vec4(color);
}