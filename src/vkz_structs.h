#ifndef __VKZ_STRUCTS_H__
#define __VKZ_STRUCTS_H__


#include <stdint.h>
#include "macro.h"

namespace vkz
{
    static const uint16_t kInvalidHandle = UINT16_MAX;

    template <class HandleType>
    struct NO_VTABLE Handle {
        uint16_t id;

        bool operator == (const Handle<HandleType>& rhs) const {
            return id == rhs.id;
        }
    };

    template <class HandleType>
    bool inline isValid(const Handle<HandleType>& handle) {
        return kInvalidHandle != handle.id;
    }


    enum class PassExeQueue : uint16_t
    {
        graphics = 0,
        compute = 1,
        copy,
        count,
    };

    enum class PipelineBindPoint : uint16_t
    {
        compute = 0,
        graphics = 1,
        raytracing = 2,
        max_enum = 0x7FFF
    };

    enum class CompareOp : uint16_t
    {
        never = 0,
        less = 1,
        equal = 2,
        less_or_equal = 3,
        greater = 4,
        not_equal = 5,
        greater_or_equal = 6,
        always = 7,
        max_enum = 0x7FFF
    };

    enum class ResourceFormat
    {
        undefined, // plain color formats below

        r8_sint,
        r8_uint,

        r16_uint,
        r16_sint,
        r16_snorm,
        r16_unorm,
        
        r32_uint,
        r32_sint,
        r32_sfloat,
        
        b8g8r8a8_snorm,
        b8g8r8a8_unorm,
        b8g8r8a8_sint,
        b8g8r8a8_uint,

        r8g8b8a8_snorm,
        r8g8b8a8_unorm,
        r8g8b8a8_sint,
        r8g8b8a8_uint,

        unknown_depth, // Depth formats below.

        d16,
        d32,

        depth = d16 | d32,

        resource_format_max = 0x7fffffff,
    };

    enum class ResourceLifetime : uint8_t
    {
        none = 0,
        single_frame,
        multi_frame,
        count,
    };

    enum class VertexInputRate {
        vertex = 0,
        instance = 1,
        input_rate_max = 0x7FFFFFFF
    };

    enum class ImageType
    {
        type_1d,
        type_2d,
        type_3d,
        img_type_max = 0x7FFFFFFF
    };

    enum class ImageViewType
    {
        type_1d,
        type_2d,
        type_3d,
        type_cube,
        type_1d_array,
        type_2d_array,
        type_cube_array,
        img_view_type_max = 0x7FFFFFFF

    };

    enum class ImageLayout
    {
        undefined,
        general,
        color_attachment_optimal,
        depth_stencil_attachment_optimal,
        depth_stencil_read_only_optimal,
        shader_read_only_optimal,
        transfer_src_optimal,
        transfer_dst_optimal,
        preinitialized,
        depth_read_only_stencil_attachment_optimal,
        depth_attachment_stencil_read_only_optimal,
        depth_attachment_optimal,
        depth_read_only_optimal,
        stencil_attachment_optimal,
        stencil_read_only_optimal,
        read_only_optimal,
        attacment_optimal,
        present_optimal,

        img_layout_max = 0x7FFFFFFF
    };

    namespace BufferUsageFlagBits
    {
        enum Enum : uint16_t
        {
            none = 0,

            vertex = 1 << 0,
            index = 1 << 1,
            uniform = 1 << 2,
            storage = 1 << 3,
            indirect = 1 << 4,
            transfer_src = 1 << 5,
            transfer_dst = 1 << 6,

            max_enum = 0x7fff,
        };
    }
    using BufferUsageFlags = uint16_t;


    namespace ImageUsageFlagBits
    {
        enum Enum : uint16_t
        {
            none = 0,

            color_attachment = 1 << 0,
            depth_stencil_attachment = 1 << 1,
            sampled = 1 << 2,
            storage = 1 << 3,
            transient_attachment = 1 << 4,
            input_attachment = 1 << 5,
            transfer_src = 1 << 6,
            transfer_dst = 1 << 7,

            max_enum = 0x7fff,
        };
    }
    using ImageUsageFlags = uint16_t;

    namespace ImageAspectFlagBits
    {
        enum Enum : uint16_t
        {
            none = 0,

            color = 1 << 0,
            depth = 1 << 1,
            stencil = 1 << 2,

            max_enum = 0x7fff,
        };
    }
    using ImageAspectFlags = uint16_t;

    namespace MemoryPropFlagBits
    {
        enum Enum : uint16_t
        {
            none = 0,

            device_local = 1 << 0,
            host_visible = 1 << 1,
            host_coherent = 1 << 2,
            host_cached = 1 << 3,
            lazily_allocated = 1 << 4,

            max_enum = 0x7fff,
        };
    }
    using MemoryPropFlags = uint16_t;

    struct PipelineConfig
    {
        bool enableDepthTest{true};
        bool enableDepthWrite{true};
        CompareOp depthCompOp{ CompareOp::greater };
    };

    struct BufferDesc {
        uint32_t size{ 0 };

        BufferUsageFlags usage{ BufferUsageFlagBits::none };
        MemoryPropFlags memFlags{ MemoryPropFlagBits::none };
    };

    struct ImageDesc {
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t depth{ 0 };
        uint16_t arrayLayers{ 1 };
        uint16_t mips{ 1 };

        ImageType       type{ ImageType::type_2d };
        ImageViewType   viewType{ ImageViewType::type_2d };
        ImageLayout     layout{ ImageLayout::general };
        ResourceFormat  format{ ResourceFormat::undefined };
        ImageUsageFlags usage{ ImageUsageFlagBits::color_attachment };
    };

    struct VertexBindingDesc
    {
        uint32_t            binding{ 0 };
        uint32_t            stride{ 0 };
        VertexInputRate     inputRate{ VertexInputRate::vertex };
    };

    struct VertexAttributeDesc
    {
        uint32_t        location{ 0 };
        uint32_t        binding{ 0 };
        uint32_t        offset{ 0 };
        ResourceFormat  format{ ResourceFormat::undefined };
    };


    struct PassDesc
    {
        uint16_t        programId{kInvalidHandle};
        PassExeQueue    queue{ PassExeQueue::graphics};

        uint16_t        vertexBindingNum{0};
        uint16_t        vertexAttributeNum{0};
        void* vertexBindingInfos{ nullptr };
        void* vertexAttributeInfos{ nullptr };

        uint32_t        pushConstantNum{0}; // each constant is 4 byte
        void*           pushConstants{nullptr};
        PipelineConfig  pipelineConfig{};
    };

} // namespace vkz
#endif // __VKZ_STRUCTS_H__
