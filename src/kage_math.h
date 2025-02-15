#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;
using quat = glm::quat;
using mat3 = glm::mat3;
using mat4 = glm::mat4;


inline uint32_t previousPow2(uint32_t v)
{
    uint32_t r = 1;

    while (r < v)
        r <<= 1;
    r >>= 1;
    return r;
}

inline uint32_t calcMipLevelCount(uint32_t _w)
{
    uint32_t ret = 0;
    while (_w > 1)
    {
        ret++;
        _w >>= 1;
    }
    return ret;
}

inline uint32_t calcMipLevelCount(uint32_t _w, uint32_t _h)
{
    uint32_t ret = 0;
    while (_w > 1 || _h > 1)
    {
        ret++;
        _w >>= 1;
        _h >>= 1;
    }
    return ret;
}