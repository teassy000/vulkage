#pragma once

#include <assert.h>
#include "common.h"


namespace vkz
{
    constexpr uint16_t kInvalidIndex = 0xffff;

    template<typename T>
    const size_t linearSearch(const stl::vector<T>& _vec, const T _data)
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
    const size_t getElemIndex(const stl::vector<T>& _vec, const T _data)
    {
        return linearSearch(_vec, _data);
    }

    template<typename T>
    const size_t push_back_unique(stl::vector<T>& _vec, const T _data)
    {
        size_t idx = getElemIndex(_vec, _data);

        if (kInvalidIndex == idx)
        {
            idx = _vec.size();
            _vec.push_back(_data);
        }

        return idx;
    }

    // store data in a vector, and use a map to store the index of the data in the vector
    template<typename IdType, typename DataType>
    struct ContinuousMap
    {
    public:
        size_t size() const {
            return ids.size();
        }

        void clear() {
            idToIndex.clear();
            ids.clear();
            indexToData.clear();
        }

        size_t getIdIndex(IdType _id) const {
            auto it = idToIndex.find(_id);
            if (it != idToIndex.end())
            {
                return it->second;
            }

            return kInvalidIndex;
        }

        bool exist(IdType _id) const {
            return getIdIndex(_id) != kInvalidIndex;
        }

        size_t update(const IdType& _id, const DataType& _data) {
            size_t idx = getIdIndex(_id);
            if (idx != kInvalidIndex) {
                indexToData[idx] = _data;
            }
            else
            {
                assert(0);
            }

            return idx;
        }

        size_t addOrUpdate(const IdType& _id, const DataType& _data) {
            if (!exist(_id))
            {
                idToIndex.insert({ _id, ids.size() });
                ids.push_back(_id);
                indexToData.push_back(_data);
            }
            else
            {
                update(_id, _data);
            }

            return getIdIndex(_id);
        }

        DataType& getDataRef(IdType _id) {
            size_t index = getIdIndex(_id);
            assert(index != kInvalidIndex);

            return indexToData[index];
        }

        const DataType& getIdToData(IdType _id) const {
            size_t index = getIdIndex(_id);
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

        stl::unordered_map<IdType, size_t> idToIndex;
        stl::vector<IdType> ids;
        stl::vector<DataType> indexToData;
    };
}
