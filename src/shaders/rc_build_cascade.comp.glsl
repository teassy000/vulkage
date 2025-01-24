#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "mesh_gpu.h"
#include "rc_common.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block
{
	RadianceCascadesConfig config;
};

layout(binding = 0) readonly uniform Transform
{
    TransformData trans;
};

layout(binding = 1) uniform sampler2D in_albedo;
layout(binding = 2) uniform sampler2D in_normal;
layout(binding = 3) uniform sampler2D in_wPos;
layout(binding = 4) uniform sampler2D in_emmision;
layout(binding = 5) uniform sampler2D in_depth;

layout(binding = 6) uniform sampler3D in_voxAlbedo;
layout(binding = 7, rgba32f) uniform writeonly image2DArray octProbAtlas;

void main()
{
    const uint lv = config.level;
    const uint prob_diameter = config.probeDiameter;
    const uint ray_diameter = config.rayGridDiameter;

    const uint prob_count = prob_diameter * prob_diameter * prob_diameter;
    const uint prob_per_layer = prob_diameter * prob_diameter;
    const uint ray_count = ray_diameter * ray_diameter;

    // Calculate the rayDir index
    uint prob_idx = gl_GlobalInvocationID.x / ray_count;
    uint prob_layer_idx = prob_idx % prob_per_layer;
    uint layer_idx = prob_idx / prob_per_layer + config.layerOffset;
    const uint prob_ray_idx = gl_GlobalInvocationID.x % ray_count;
    ivec2 sub_prob_coord = ivec2(prob_layer_idx % prob_diameter, prob_layer_idx / prob_diameter);
    ivec2 sub_ray_coord = ivec2(prob_ray_idx % ray_diameter, prob_ray_idx / ray_diameter);
    vec2 prob_coord = sub_prob_coord * ray_diameter + sub_ray_coord;

    // the texel
    const vec2 ray_uv = (sub_ray_coord + .5) / ray_diameter;
    const vec3 rayDir = octDecode(ray_uv);

    // ray march with fixed step size, each ray will split into 4 sub-rays
    // cascades should generated in view space
    // thus the ray marching should be in view space

    // the ray origin start form the center of the probe

    





    imageStore(octProbAtlas, ivec3(prob_coord.xy, layer_idx), vec4(rayDir, 1));
}