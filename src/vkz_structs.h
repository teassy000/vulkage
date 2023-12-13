#ifndef __VKZ_STRUCTS_H__
#define __VKZ_STRUCTS_H__


#include <stdint.h>

namespace vkz
{

    enum class PassExeQueue : uint16_t
    {
        Graphics = 0,
        AsyncCompute0 = 1,
        AsyncCompute1 = 2,
        AsyncCompute2 = 3,
        Count = 4,
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

    struct VertexBinding
    {
        uint16_t    binding;
        uint16_t    stride;
        uint16_t    inputRate;
    };

    struct VertexAttribute
    {
        uint32_t        location;
        uint32_t        binding;
        ResourceFormat   format;
        uint32_t        offset;
    };

    struct PipelineConfig
    {
        uint16_t enableDepthTest;
        uint16_t enableDepthWrite;
        CompareOp depthCompOp{ CompareOp::greater };
    };

    struct BufferDesc {
        uint32_t size;

        BufferUsageFlags usage;
        MemoryPropFlags memFlags;
    };

    struct ImageDesc {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint16_t arrayLayers;
        uint16_t mips;

        ImageType       type;
        ImageViewType   viewType;
        ImageLayout     layout;
        ResourceFormat  format;
        ImageUsageFlags usage;
    };

    struct VertexBindingDesc
    {
        uint32_t            binding;
        uint32_t            stride;
        VertexInputRate     inputRate;
    };

    struct VertexAttributeDesc
    {
        uint32_t        location;
        uint32_t        binding;
        uint32_t        offset;
        ResourceFormat  format;
    };

    struct PassDesc
    {
        uint16_t        programId;
        PassExeQueue    queue;

        uint16_t        vertexBindingNum;
        uint16_t        vertexAttributeNum;
        void*           vertexBindingInfos;
        void*           vertexAttributeInfos;

        PipelineConfig  pipelineConfig;
    };

} // namespace vkz
#endif // __VKZ_STRUCTS_H__
