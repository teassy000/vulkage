#pragma once
#include "common.h"
#include "vkz.h"

namespace vkz
{
    // from bgfx, with modifications
    // use a sparse set to store the handles
    class HandleContainer
    {
    public:
        HandleContainer(uint16_t _maxHandles);
        ~HandleContainer();

        uint16_t getNumHandles() const;
        uint16_t getMaxHandles() const;

        uint16_t alloc();
        uint16_t getHandleAt(uint16_t _at) const;
        void free(uint16_t _handle);

        bool isValid(uint16_t _handle) const;

        void reset();

    private:
        uint16_t* getDensePtr() const;
        uint16_t* getSparsePtr() const;

        HandleContainer() = delete;
        uint16_t m_numHandles;
        uint16_t m_maxHandles;
    };


    uint16_t* HandleContainer::getDensePtr() const
    {
        uint8_t* ptr = (uint8_t*)reinterpret_cast<const uint8_t*>(this);
        return (uint16_t*)&ptr[sizeof(HandleContainer)];
    }

    uint16_t* HandleContainer::getSparsePtr() const
    {
        return &getDensePtr()[m_maxHandles];
    }

    HandleContainer::HandleContainer(uint16_t _maxHandles)
        : m_maxHandles(_maxHandles)
        , m_numHandles(0)
    {
    }

    inline HandleContainer::~HandleContainer()
    {
    }


    inline uint16_t HandleContainer::getNumHandles() const
    {
        return m_numHandles;
    }

    inline uint16_t HandleContainer::getMaxHandles() const
    {
        return m_maxHandles;
    }

    inline uint16_t HandleContainer::alloc()
    {
        if (m_numHandles < m_maxHandles)
        {
            uint16_t index = m_numHandles;
            ++m_numHandles;

            uint16_t* dense = getDensePtr();
            uint16_t  handle = dense[index];
            uint16_t* sparse = getSparsePtr();
            sparse[handle] = index;
            return handle;
        }

        return kInvalidHandle;
    }
    
    inline bool HandleContainer::isValid(uint16_t _handle) const
    {
        uint16_t* dense = getDensePtr();
        uint16_t* sparse = getSparsePtr();
        uint16_t  index = sparse[_handle];

        return index < m_numHandles
            && dense[index] == _handle;
    }

    inline uint16_t HandleContainer::getHandleAt(uint16_t _at) const
    {
        return getDensePtr()[_at];
    }

    inline void HandleContainer::free(uint16_t _handle)
    {
        uint16_t* dense = getDensePtr();
        uint16_t* sparse = getSparsePtr();
        uint16_t index = sparse[_handle];
        --m_numHandles;
        uint16_t temp = dense[m_numHandles];
        dense[m_numHandles] = _handle;
        sparse[temp] = index;
        dense[index] = temp;
    }

    inline void HandleContainer::reset()
    {
        m_numHandles = 0u;
        uint16_t* dense = getDensePtr();
        for (uint16_t ii = 0, num = m_maxHandles; ii < num; ++ii)
        {
            dense[ii] = ii;
        }
    }

    template <uint16_t maxHandlesT>
    class HandleArrayT : public HandleContainer
    {
    public:
        HandleArrayT();
        ~HandleArrayT();

    private:
        std::array<uint16_t, maxHandlesT*2> m_paddings;
    };

    template <uint16_t maxHandlesT>
    vkz::HandleArrayT<maxHandlesT>::~HandleArrayT()
    {

    }

    template <uint16_t maxHandlesT>
    vkz::HandleArrayT<maxHandlesT>::HandleArrayT()
    {

    }

}