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

    enum class ResourceFormat : uint16_t
    {
        Unknown, // plain color formats below

        A8,
        R8,
        R8I,
        R8S,
        R8U,
        R16,
        R16I,
        R16U,
        R16F,
        R16S,
        R32,
        R32I,
        R32U,
        R32F,
        R32S,
        BGRA8,
        BGRA8I,
        BGRA8U,
        BGRA8F,
        BGRA8S,
        RGBA8,
        RGBA8I,
        RGBA8U,
        RGBA8F,
        RGBA8S,

        UnknownDepth, // Depth formats below.

        D16,
        D32,
        D16F,
        D32F,

        Count,
    };

    enum class ResourceLifetime : uint8_t
    {
        none = 0,
        single_frame,
        multi_frame,
        count,
    };

    enum class VertexInputRate {
        input_rate_vertex = 0,
        input_rate_instance = 1,
        input_rate_max = 0x7FFFFFFF
    };

    enum class BufferUsageFlagBits : uint16_t
    {
        none = 0,
        vertex = 1 << 0,
        index = 1 << 1,
        uniform = 1 << 2,
        storage = 1 << 3,
        indirect = 1 << 4,
        max_enum = 0x7fff,
    };
    using BufferUsageFlags = uint16_t;

    enum class ImageUsageFlagBits : uint16_t
    {
        none = 0,
        color_attachment = 1 << 0,
        depth_stencil_attachment = 1 << 1,
        sampled = 1 << 2,
        storage = 1 << 3,
        transient_attachment = 1 << 4,
        input_attachment = 1 << 5,
        max_enum = 0x7fff,
    };
    using ImageUsageFlags = uint16_t;

    enum class MemoryPropFlagBits : uint16_t
    {
        none = 0,
        device_local = 1 << 0,
        host_visible = 1 << 1,
        host_coherent = 1 << 2,
        host_cached = 1 << 3,
        lazily_allocated = 1 << 4,
        max_enum = 0x7fff,
    };
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
        uint16_t layers;
        uint16_t mips;

        ResourceFormat format;
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
        void* vertexBindingInfos;
        void* vertexAttributeInfos;

        PipelineConfig  pipelineConfig;
    };

} // namespace vkz
#endif // __VKZ_STRUCTS_H__
