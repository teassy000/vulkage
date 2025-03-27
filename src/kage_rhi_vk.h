#pragma once

#include "volk.h"
#include "bx/bx.h"
#include "common.h"
#include "bx/handlealloc.h"

#define VK_DESTROY                                \
			VK_DESTROY_FUNC(Buffer);              \
			VK_DESTROY_FUNC(CommandPool);         \
			VK_DESTROY_FUNC(DescriptorPool);      \
			VK_DESTROY_FUNC(DescriptorSetLayout); \
			VK_DESTROY_FUNC(Fence);               \
			VK_DESTROY_FUNC(Framebuffer);         \
			VK_DESTROY_FUNC(Image);               \
			VK_DESTROY_FUNC(ImageView);           \
			VK_DESTROY_FUNC(Pipeline);            \
			VK_DESTROY_FUNC(PipelineCache);       \
			VK_DESTROY_FUNC(PipelineLayout);      \
			VK_DESTROY_FUNC(QueryPool);           \
			VK_DESTROY_FUNC(RenderPass);          \
			VK_DESTROY_FUNC(Sampler);             \
			VK_DESTROY_FUNC(Semaphore);           \
			VK_DESTROY_FUNC(ShaderModule);        \
			VK_DESTROY_FUNC(SwapchainKHR);        \

namespace kage { namespace vk 
{

#define VK_DESTROY_FUNC(_name)                                   \
	struct Vk##_name                                             \
	{                                                            \
		::Vk##_name vk;                                          \
		Vk##_name() {}                                           \
		Vk##_name(::Vk##_name _vk) : vk(_vk) {}                  \
		operator ::Vk##_name() { return vk; }                    \
		operator ::Vk##_name() const { return vk; }              \
		::Vk##_name* operator &() { return &vk; }                \
		const ::Vk##_name* operator &() const { return &vk; }    \
		operator size_t() const { return (size_t)vk; }           \
		bool operator == (const Vk##_name& _other) const { return vk == _other.vk; }   \
	};                                                           \
	BX_STATIC_ASSERT(sizeof(::Vk##_name) == sizeof(Vk##_name) ); \
	void vkDestroy(Vk##_name&);                                  \
	void release(Vk##_name&)
    VK_DESTROY
VK_DESTROY_FUNC(DeviceMemory);
VK_DESTROY_FUNC(SurfaceKHR);
VK_DESTROY_FUNC(DescriptorSet);

#undef VK_DESTROY_FUNC

template <typename Ty, uint16_t MaxHandleT>
class StateCacheLru
{
public:
    Ty* add(uint64_t _key, const Ty& _value, uint16_t _parent)
    {
        uint16_t handle = m_alloc.alloc();
        if (UINT16_MAX == handle)
        {
            uint16_t back = m_alloc.getBack();
            invalidate(back);
            handle = m_alloc.alloc();
        }

        BX_ASSERT(UINT16_MAX != handle, "Failed to find handle.");

        Data& data = m_data[handle];
        data.m_hash = _key;
        data.m_value = _value;
        data.m_parent = _parent;
        m_hashMap.insert(stl::make_pair(_key, handle));

        return bx::addressOf(m_data[handle].m_value);
    }

    Ty* find(uint64_t _key)
    {
        HashMap::iterator it = m_hashMap.find(_key);
        if (it != m_hashMap.end())
        {
            uint16_t handle = it->second;
            m_alloc.touch(handle);
            return bx::addressOf(m_data[handle].m_value);
        }

        return NULL;
    }

    void invalidate(uint64_t _key)
    {
        HashMap::iterator it = m_hashMap.find(_key);
        if (it != m_hashMap.end())
        {
            uint16_t handle = it->second;
            m_alloc.free(handle);
            m_hashMap.erase(it);
            release(m_data[handle].m_value);
        }
    }

    void invalidate(uint16_t _handle)
    {
        if (m_alloc.isValid(_handle))
        {
            m_alloc.free(_handle);
            Data& data = m_data[_handle];
            m_hashMap.erase(m_hashMap.find(data.m_hash));
            release(data.m_value);
        }
    }

    void invalidateWithParent(uint16_t _parent)
    {
        for (uint16_t ii = 0; ii < m_alloc.getNumHandles();)
        {
            uint16_t handle = m_alloc.getHandleAt(ii);
            Data& data = m_data[handle];

            if (data.m_parent == _parent)
            {
                m_alloc.free(handle);
                m_hashMap.erase(m_hashMap.find(data.m_hash));
                release(data.m_value);
            }
            else
            {
                ++ii;
            }
        }
    }

    void invalidate()
    {
        for (uint16_t ii = 0, num = m_alloc.getNumHandles(); ii < num; ++ii)
        {
            uint16_t handle = m_alloc.getHandleAt(ii);
            Data& data = m_data[handle];
            release(data.m_value);
        }

        m_hashMap.clear();
        m_alloc.reset();
    }

    uint32_t getCount() const
    {
        return uint32_t(m_hashMap.size());
    }

private:
    typedef stl::unordered_map<uint64_t, uint16_t> HashMap;
    HashMap m_hashMap;
    bx::HandleAllocLruT<MaxHandleT> m_alloc;
    struct Data
    {
        uint64_t m_hash;
        Ty m_value;
        uint16_t m_parent;
    };

    Data m_data[MaxHandleT];
};

template <typename Ty>
class StateCacheT
{
public:
    void add(uint64_t _key, Ty _value)
    {
        invalidate(_key);
        m_hashMap.insert(stl::make_pair(_key, _value));
    }

    Ty find(uint64_t _key)
    {
        typename HashMap::iterator it = m_hashMap.find(_key);
        if (it != m_hashMap.end())
        {
            return it->second;
        }

        return 0;
    }

    void invalidate(uint64_t _key)
    {
        typename HashMap::iterator it = m_hashMap.find(_key);
        if (it != m_hashMap.end())
        {
            release(it->second);
            m_hashMap.erase(it);
        }
    }

    void invalidate()
    {
        for (typename HashMap::iterator it = m_hashMap.begin(), itEnd = m_hashMap.end(); it != itEnd; ++it)
        {
            release(it->second);
        }

        m_hashMap.clear();
    }

    uint32_t getCount() const
    {
        return uint32_t(m_hashMap.size());
    }

private:
    typedef stl::unordered_map<uint64_t, Ty> HashMap;
    HashMap m_hashMap;
};

}
}