#pragma once

#include "volk.h"
#include "bx/bx.h"

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
		operator size_t() const { return (size_t)vk; }           \
		bool operator == (const Vk##_name& _other) const { return vk == _other.vk; }   \
		::Vk##_name* operator &() { return &vk; }                \
		const ::Vk##_name* operator &() const { return &vk; }    \
	};                                                           \
	BX_STATIC_ASSERT(sizeof(::Vk##_name) == sizeof(Vk##_name) ); \
	void vkDestroy(Vk##_name&);                                  \
	void release(Vk##_name&)
    VK_DESTROY
VK_DESTROY_FUNC(DeviceMemory);
VK_DESTROY_FUNC(SurfaceKHR);
VK_DESTROY_FUNC(DescriptorSet);

#undef VK_DESTROY_FUNC

}
}