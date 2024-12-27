#pragma once

#include <stdint.h>
#include "macro.h"
#include "config.h"

namespace kage
{
    static const uint16_t kInvalidHandle = UINT16_MAX;
    static const uint32_t kInvalidDescriptorSetIndex = UINT32_MAX;
    static const uint32_t kDescriptorSetBindingDontCare = UINT32_MAX - 1;

    static const uint16_t kAllMips = UINT16_MAX;

#define KAGE_HANDLE(_name)                                                              \
    struct _name {                                                                      \
        uint16_t id{kInvalidHandle};                                                    \
        bool operator == (const _name& rhs) const { return id == rhs.id; };             \
        bool operator < (const _name& rhs) const { return id < rhs.id; };               \
        operator size_t() const { return id; };                                         \
    };                                                                                  \
    bool inline isValid(const _name _handle) {return kInvalidHandle != _handle.id;};    \

    KAGE_HANDLE(ShaderHandle);
    KAGE_HANDLE(ProgramHandle);
    KAGE_HANDLE(PassHandle);
    KAGE_HANDLE(BufferHandle);
    KAGE_HANDLE(ImageHandle);
    KAGE_HANDLE(SamplerHandle);
    KAGE_HANDLE(BindlessHandle);

    using ReleaseFn = void (*)(void* _ptr, void* _userData);

    struct Memory
    {
        Memory() = delete;

        uint8_t* data;
        uint32_t size;
    };

    enum class VulkanSupportExtension : uint16_t
    {
        ext_swapchain,
        ext_push_descriptor,
        ext_8bit_storage,
        ext_16bit_storage,
        ext_mesh_shader,
        ext_spirv_1_4,
        ext_shader_float_controls,
        ext_shader_draw_parameters,
        ext_draw_indriect_count,
        ext_shader_float16_int8,
        ext_fragment_shading_rate,
    };

    enum class PassExeQueue : uint16_t
    {
        graphics = 0,
        compute,
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
        bc1,
        bc2,
        bc3,
        bc4,
        bc5,
        bc6,
        bc7,

        undefined, // plain color formats below

        r8_snorm,
        r8_unorm,
        r8_sint,
        r8_uint,

        r8g8_snorm,
        r8g8_unorm,
        r8g8_sint,
        r8g8_uint,

        r8g8b8_snorm,
        r8g8b8_unorm,
        r8g8b8_sint,
        r8g8b8_uint,

        b8g8r8_snorm,
        b8g8r8_unorm,
        b8g8r8_sint,
        b8g8r8_uint,

        r8g8b8a8_snorm,
        r8g8b8a8_unorm,
        r8g8b8a8_sint,
        r8g8b8a8_uint,

        b8g8r8a8_snorm,
        b8g8r8a8_unorm,
        b8g8r8a8_sint,
        b8g8r8a8_uint,

        r16_uint,
        r16_sint,
        r16_snorm,
        r16_unorm,
        r16_sfloat,

        r16g16_unorm,
        r16g16_snorm,
        r16g16_uint,
        r16g16_sint,
        r16g16_sfloat,

        r16g16b16a16_unorm,
        r16g16b16a16_snorm,
        r16g16b16a16_uint,
        r16g16b16a16_sint,
        r16g16b16a16_sfloat,
        
        r32_uint,
        r32_sint,
        r32_sfloat,

        r32g32_uint,
        r32g32_sint,
        r32g32_sfloat,

        r32g32b32_uint,
        r32g32b32_sint,
        r32g32b32_sfloat,

        r32g32b32a32_uint,
        r32g32b32a32_sint,
        r32g32b32a32_sfloat,

        b5g6r5_unorm,
        r5g6b5_unorm,
        b4g4r4a4_unorm,
        r4g4b4a4_unorm,
        b5g5r5a1_unorm,
        r5g5b5a1_unorm,
        a2r10g10b10_unorm,
        b10g11r11_sfloat,

        unknown_depth, // Depth formats below.

        d16,
        d24_sfloat_s8_uint,
        d32_sfloat,

        resource_format_max = 0x7fffffff,
    };

    enum class ResourceLifetime : uint8_t
    {
        none,
        transition,
        non_transition,
        count,
    };

    enum class VertexInputRate {
        vertex,
        instance,
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

    enum class SamplerFilter
    {
        nearest,
        linear,
        sampler_filter_max = 0x7FFFFFFF
    };

    enum class SamplerMipmapMode
    {
        nearest,
        linear,
        sampler_mipmap_mode_max = 0x7FFFFFFF
    };

    enum class SamplerAddressMode
    {
        repeat,
        mirrored_repeat,
        clamp_to_edge,
        clamp_to_border,
        mirror_clamp_to_edge,
        sampler_address_mode_max = 0x7FFFFFFF
    };

    enum class SamplerReductionMode
    {
        weighted_average,
        min,
        max,

        max_enum = 0x7FFFFFFF
    };

    enum class IndexType
    {
        uint16,
        uint32,
    };

    namespace CullModeFlagBits
    {
        enum Enum : uint16_t
        {
            none = 0,

            front = 1 >> 0,
            back = 1 >> 1,
            front_and_back = front | back,
            
            max_enum = 0x7fff,
        };
    };
    using CullModeFlags = uint16_t;

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
    using ImageAspectFlags  = uint16_t;

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

    namespace PipelineStageFlagBits
    {
        enum Enum : uint32_t
        {
            none = 0,

            top_of_pipe = 1 << 0,
            draw_indirect = 1 << 1,
            vertex_input = 1 << 2,
            vertex_shader = 1 << 3,

            geometry_shader = 1 << 6,
            fragment_shader = 1 << 7,
            early_fragment_tests = 1 << 8,
            late_fragment_tests = 1 << 9,
            color_attachment_output = 1 << 10,
            compute_shader = 1 << 11,
            transfer = 1 << 12,
            bottom_of_pipe = 1 << 13,
            host = 1 << 14,
            all_graphics = 1 << 15,
            all_commands = 1 << 16,

            task_shader = 1 << 22,
            mesh_shader = 1 << 23,
            
            max_enum = 0x7fffffff,
        };
    }
    using PipelineStageFlags = uint32_t;

    namespace AccessFlagBits
    {
        enum Enum : uint32_t
        {
            none = 0,

            indirect_command_read = 1 << 0,
            index_read = 1 << 1,
            vertex_attribute_read = 1 << 2,
            uniform_read = 1 << 3,
            input_attachment_read = 1 << 4,
            shader_read = 1 << 5,
            shader_write = 1 << 6,
            color_attachment_read = 1 << 7,
            color_attachment_write = 1 << 8,
            depth_stencil_attachment_read = 1 << 9,
            depth_stencil_attachment_write = 1 << 10,
            transfer_read = 1 << 11,
            transfer_write = 1 << 12,
            host_read = 1 << 13,
            host_write = 1 << 14,
            memory_read = 1 << 15,
            memory_write = 1 << 16,
            transform_feedback_write = 1 << 17,
            transform_feedback_counter_read = 1 << 18,
            transform_feedback_counter_write = 1 << 19,
            conditional_rendering_read = 1 << 20,
            color_attachment_read_noncoherent = 1 << 21,
            acceleration_structure_read = 1 << 22,
            acceleration_structure_write = 1 << 23,
            shading_rate_image_read = 1 << 24,

            read_only = indirect_command_read | index_read | vertex_attribute_read | uniform_read | input_attachment_read | shader_read | color_attachment_read | depth_stencil_attachment_read | transfer_read | host_read | memory_read | transform_feedback_counter_read | conditional_rendering_read | color_attachment_read_noncoherent | acceleration_structure_read | shading_rate_image_read,
            write_only = shader_write | color_attachment_write | depth_stencil_attachment_write | transfer_write | host_write | memory_write | transform_feedback_write | transform_feedback_counter_write | acceleration_structure_write,

            max_enum = 0x7fffffff,
        };
    }
    using AccessFlags = uint32_t;

    enum class AttachmentLoadOp
    {
        load,
        clear,
        dont_care,
        attachement_load_op_max = 0x7fffffff,
    };

    enum class AttachmentStoreOp
    {
        store,
        dont_care,
        none,
        attachement_store_op_max = 0x7fffffff,
    };

    enum class ResourceType : uint16_t
    {
        undefined,
        buffer,
        image,
    };

    namespace KeyModBits
    {
        enum Enum : uint32_t
        {
            none = 0,
            shift = 1 << 0,
            ctrl = 1 << 1,
            alt = 1 << 2,
            capslock = 1 << 3,
            numlock = 1 << 4,
            max_enum = 0xffffffff,
        };
    }
    using KeyModFlags = uint32_t;

    enum class KeyState : uint32_t
    {
        unknown,
        release,
        press,
        repeat,
    };

    enum  class KeyEnum : uint32_t
    {
        key_unknown,
        key_a,
        key_b,
        key_c,
        key_d,
        key_e,
        key_f,
        key_g,
        key_h,
        key_i,
        key_j,
        key_k,
        key_l,
        key_m,
        key_n,
        key_o,
        key_p,
        key_q,
        key_r,
        key_s,
        key_t,
        key_u,
        key_v,
        key_w,
        key_x,
        key_y,
        key_z,
        key_0,
        key_1,
        key_2,
        key_3,
        key_4,
        key_5,
        key_6,
        key_7,
        key_8,
        key_9,
        key_f1,
        key_f2,
        key_f3,
        key_f4,
        key_f5,
        key_f6,
        key_f7,
        key_f8,
        key_f9,
        key_f10,
        key_f11,
        key_f12,
        key_left,
        key_right,
        key_up,
        key_down,
        key_space,
        key_esc,
        key_enter,
        key_backspace,
        key_tab,
        key_capslock,
        key_shift,
        key_ctrl,
        key_alt,
        key_max_enum = 0xffffffff,
    };

    enum class BindingAccess : uint16_t
    {
        read,
        write,
        read_write,
        binding_access_max = 0x7fff,
    };


    struct Resolution
    {
        uint32_t width{ 2560 };
        uint32_t height{ 1440 };
        uint32_t reset{ 0 };
        ResourceFormat format{ ResourceFormat::r8g8b8a8_unorm };
        uint8_t numBackBuffers{ 2 };
        uint8_t maxFrameLatency{ kMaxNumFrameLatency };
    };

    struct Init
    {
        Resolution resolution;

        void * windowHandle{ nullptr };
        const char* name{ nullptr };
        uint32_t minCmdBufSize;
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
        ResourceFormat  format{ ResourceFormat::undefined };
        uint32_t        offset{ 0 };


        VertexAttributeDesc() = default;
        VertexAttributeDesc(uint32_t _location, uint32_t _binding, ResourceFormat _format, uint32_t _offset)
            : location(_location), binding(_binding), format(_format), offset(_offset) {}
    };

    struct PipelineConfig
    {
        bool enableDepthTest{ true };
        bool enableDepthWrite{ true };
        CompareOp depthCompOp{ CompareOp::greater };
        CullModeFlags cullMode{ CullModeFlagBits::back };
    };

    struct PassConfig
    {
        AttachmentLoadOp depthLoadOp{ AttachmentLoadOp::clear };
        AttachmentStoreOp depthStoreOp{ AttachmentStoreOp::store };

        AttachmentLoadOp colorLoadOp{ AttachmentLoadOp::clear };
        AttachmentStoreOp colorStoreOp{ AttachmentStoreOp::store };
    };

    struct BufferDesc 
    {
        uint32_t size{ 0 };

        uint32_t fillVal{ 0 };

        BufferUsageFlags usage{ BufferUsageFlagBits::none };
        MemoryPropFlags memFlags{ MemoryPropFlagBits::none };
    };

    struct ImageDesc 
    {
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t depth{ 1 };
        uint16_t numLayers{ 1 };
        uint16_t numMips{ 1 };

        ImageType       type{ ImageType::type_2d };
        ImageViewType   viewType{ ImageViewType::type_2d };
        ImageLayout     layout{ ImageLayout::undefined };
        ResourceFormat  format{ ResourceFormat::undefined };
        ImageUsageFlags usage{ ImageUsageFlagBits::color_attachment };
    };

    struct BindlessDesc
    {
        uint32_t setIdx{ 0 };
        uint32_t binding{ 0 };
        ResourceType resType{ ResourceType::undefined };
        SamplerReductionMode reductionMode{ SamplerReductionMode::max_enum };
    };

    struct PassDesc
    {
        uint16_t        programId{kInvalidHandle};
        PassExeQueue    queue{ PassExeQueue::graphics};

        uint16_t        vertexBindingNum{0};
        uint16_t        vertexAttributeNum{0};
        void* vertexBindings{ nullptr };
        void* vertexAttributes{ nullptr };

        uint32_t        pipelineSpecNum{0}; // each constant is 4 byte
        void*           pipelineSpecData{nullptr};
        PipelineConfig  pipelineConfig{};
    };

    struct Binding
    {
        union
        {
            BufferHandle buf{ kInvalidHandle };
            ImageHandle img;
        };

        SamplerHandle   sampler{ kInvalidHandle };
        uint16_t        mip{ kAllMips };
        ResourceType    type{ ResourceType::undefined };
        BindingAccess   access{ BindingAccess::read };
        PipelineStageFlags stage{ PipelineStageFlagBits::none };

        Binding(BufferHandle _hBuf, BindingAccess _access, PipelineStageFlags _stages)
        {
            buf = _hBuf;
            type = ResourceType::buffer;
            access = _access;
            stage = _stages;
        }

        Binding(ImageHandle _hImg, uint16_t _mip, SamplerHandle _hSampler, PipelineStageFlags _stages)
        {
            img = _hImg;
            mip = _mip;
            type = ResourceType::image;
            sampler = _hSampler;
            access = BindingAccess::read;
            stage = _stages;
        }

        Binding(ImageHandle _hImg, SamplerHandle _hSampler, PipelineStageFlags _stages)
        {
            img = _hImg;
            sampler = _hSampler;
            type = ResourceType::image;
            access = BindingAccess::read;
            stage = _stages;
        }

        Binding(ImageHandle _hImg, uint16_t _mip, PipelineStageFlags _stages)
        {
            img = _hImg;
            mip = _mip;
            type = ResourceType::image;
            access = BindingAccess::write;
            stage = _stages;
        }
    };

    struct Attachment
    {
        ImageHandle hImg{kInvalidHandle};
        AttachmentLoadOp load_op{ AttachmentLoadOp::dont_care};
        AttachmentStoreOp store_op{AttachmentStoreOp::none};

        Attachment() = default;

        Attachment(ImageHandle _hImg, AttachmentLoadOp _load_op, AttachmentStoreOp _store_op)
        {
            hImg = _hImg;
            load_op = _load_op;
            store_op = _store_op;
        }
    };

} // namespace kage
