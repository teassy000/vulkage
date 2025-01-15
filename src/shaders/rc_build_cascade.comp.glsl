#version 450

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#extension GL_GOOGLE_include_directive: require

#include "rc_common.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform block
{
	RadianceCascadesConfig config;
};

layout(binding = 0, rgba32f) uniform writeonly image2DArray octProbAtlas;

void main()
{
    const uint lv = config.level;
    const uint prob_diameter = config.probeDiameter;
    const uint ray_diameter = config.rayGridDiameter;

    const uint prob_count = prob_diameter * prob_diameter * prob_diameter;
    const uint prob_per_layer = prob_diameter * prob_diameter;
    const uint ray_count = ray_diameter * ray_diameter;

    // Calculate the ray index
    uint prob_idx = gl_GlobalInvocationID.x / ray_count;
    uint prob_layer_idx = prob_idx % prob_per_layer;
    uint layer_idx = prob_idx / prob_per_layer + config.layerOffset;
    const uint prob_ray_idx = gl_GlobalInvocationID.x % ray_count;
    vec2 sub_prob_coord = vec2(prob_layer_idx % prob_diameter, prob_layer_idx / prob_diameter);
    vec2 sub_ray_coord = vec2(prob_ray_idx % ray_diameter, prob_ray_idx / ray_diameter);
    vec2 prob_coord = sub_prob_coord * ray_diameter + sub_ray_coord;

    // the texel
    const vec2 ray_uv = (sub_ray_coord + .5) / ray_diameter;
    const vec3 ray = octDecode(ray_uv);

    imageStore(octProbAtlas, ivec3(prob_coord.xy, layer_idx), vec4(ray, 1));
}