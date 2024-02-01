#ifndef __VKZ_UTIL_H__
#define __VKZ_UTIL_H__

#include <assert.h>
#include <TINYSTL/vector.h>
#include <vector>

namespace tstl = tinystl;

namespace vkz
{
    constexpr uint16_t kInvalidIndex = 0xffff;

    template<typename T>
    const size_t linearSearch(const tstl::vector<T>& _vec, const T _data)
    {
        for (size_t i = 0; i < _vec.size(); ++i)
        {
            if (_vec[i] == _data)
            {
                return i;
            }
        }

        return kInvalidIndex;
    }

    template<typename T>
    const size_t getElemIndex(const tstl::vector<T>& _vec, const T _data)
    {
        return linearSearch(_vec, _data);
    }

    template<typename T>
    const size_t push_back_unique(tstl::vector<T>& _vec, const T _data)
    {
        size_t idx = getElemIndex(_vec, _data);

        if (kInvalidIndex == idx)
        {
            idx = _vec.size();
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

        void clear() {
            ids.clear();
            indexToData.clear();
        }

        bool exist(IdType _id) const {
            return getElemIndex(ids, _id) != kInvalidIndex;
        }

        size_t push_back(const IdType& _id, const DataType& _data) {
            ids.push_back(_id);
            indexToData.push_back(_data);

            size_t idx = (size_t)ids.size() - 1;

            return idx;
        }

        size_t update_data(const IdType& _id, const DataType& _data) {
            size_t idx = getElemIndex(ids, _id);
            if (idx != kInvalidIndex) {
                indexToData[idx] = _data;
            }
            else
            {
                assert(0);
            }

            return idx;
        }

        
        DataType& getDataRef(IdType _id) {
            size_t index = getElemIndex(ids, _id);
            assert(index != kInvalidIndex);

            return indexToData[index];
        }

        const size_t getIndex(IdType _id) const {
            return getElemIndex(ids, _id);
        }

        const DataType& getIdToData(IdType _id) const {
            size_t index = getElemIndex(ids, _id);
            assert(index != kInvalidIndex);

            return indexToData[index];
        }

        const IdType& getIdAt(size_t _idx) const {
            assert(_idx < ids.size());
            return ids[_idx];
        }

        const DataType& getDataAt(size_t _idx) const {
            assert(_idx < ids.size());
            return indexToData[_idx];
        }

        const IdType* getIdPtr() const {
            return ids.data();
        }

        const DataType* getDataPtr() const {
            return indexToData.data();
        }
    private:
        tstl::vector<IdType> ids;
        tstl::vector<DataType> indexToData;
    };
}

#endif // __VKZ_UTIL_H__
