#ifndef __VKZ_UTIL_H__
#define __VKZ_UTIL_H__

#include <vector>

namespace vkz
{
    constexpr uint16_t kInvalidIndex = 0xffff;
    template<typename T>
    const uint16_t getElemIndex(const std::vector<T>& _vec, const T _data)
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
        uint16_t idx = getElemIndex(_vec, _data);

        if (kInvalidIndex == idx)
        {
            idx = (uint16_t)_vec.size();
            _vec.push_back(_data);
        }

        return idx;
    }

    template<typename IdType, typename DataType>
    struct UniDataContainer
    {
    public:
        size_t size() const {
            return ids.size();
        }

        void setValueAt(size_t _idx, const IdType _id, const DataType& _data) {
            assert(_idx < ids.size());

            ids[_idx] = _id;
            indexToData[_idx] = _data;
        }

        size_t addData(IdType _id, const DataType& _data) {
            size_t idx = getElemIndex(ids, _id);
            if (idx == kInvalidIndex) {
                ids.push_back(_id);
                indexToData.push_back(_data);

                idx = (size_t)ids.size() - 1;
            }

            return idx;
        }

        bool isValidId(IdType _id) const {
            return getElemIndex(ids, _id) != kInvalidIndex;
        }

        const size_t getIndex(IdType _id) const {
            return getElemIndex(ids, _id);
        }

        const IdType& getId(size_t _idx) const {
            assert(_idx < ids.size());
            return ids[_idx];
        }

        const DataType& getData(IdType _id) const {
            size_t index = getElemIndex(ids, _id);
            assert(index != kInvalidIndex);

            return indexToData[index];
        }

        const DataType& getDataAt(size_t _idx) const {
            assert(_idx < ids.size());
            return indexToData[_idx];
        }

    private:
        std::vector<IdType> ids;
        std::vector<DataType> indexToData;
    };
}

#endif // __VKZ_UTIL_H__
