// the following smaa implement based on following project:
// https://github.com/turol/smaaDemo
// 
// Copyright and License
// =====================
// 
// Copyright (c) 2015-2024 Alternative Games Ltd / Turo Lamminen
// 
// This code is licensed under the MIT license (see license.txt).
// 
// Third-party code under foreign/ is licensed according to their respective licenses.

# extension GL_EXT_control_flow_attributes: require


#ifndef SMAA_GLSL_4
#define SMAA_GLSL_4 
#endif // !SMAA_GLSL_4

#define SMAA_INCLUDE_CS 1
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 0

// disable separate sampler since we are using push descriptor template
#ifdef SMAA_GLSL_SEPARATE_SAMPLER
#undef SMAA_GLSL_SEPARATE_SAMPLER
#endif // SMAA_GLSL_SEPARATE_SAMPLER

// flip y for vulkan
#define SMAA_FLIP_Y 0

#define SMAA_RT_METRICS float4(1.0/imageSize.xy, imageSize.xy)

#include "smaa.h"
