#ifndef __VKZ_UTIL_H__
#define __VKZ_UTIL_H__

#include <vector>

namespace vkz
{
    constexpr uint16_t kInvalidIndex = 0xffff;
    template<typename T>
    const uint16_t getIndex(const std::vector<T>& _vec, const T _data)
    {
        auto it = std::find(begin(_vec), end(_vec), _data);
        if (it == end(_vec))
        {
            return kInvalidIndex;
        }

        return (uint16_t)std::distance(begin(_vec), it);
    }

    template<typename T>
    const uint16_t push_back_unique(std::vector<T>& _vec, const T _data)
    {
        uint16_t idx = getIndex(_vec, _data);

        if (kInvalidIndex == idx)
        {
            idx = (uint16_t)_vec.size();
            _vec.push_back(_data);
        }

        return idx;
    }
}

#endif // __VKZ_UTIL_H__
