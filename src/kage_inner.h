#pragma once

#include "config.h"
#include "kage.h"

#include "entry/entry.h"


namespace kage
{
    extern bx::AllocatorI* g_bxAllocator;

    struct Handle
    {
        struct TypeName
        {
            const char* abbrName;
            const char* fullName;
        };

        enum Enum : uint16_t
        {
            Shader,
            Program,
            Pass,
            Buffer,
            Image,

            Count,
        };

        template<typename Ty>
        static constexpr Enum toEnum();

        Handle() = default;

        template<typename Ty>
        Handle(Ty _h) 
            : id(_h.id)
            , type(toEnum<Ty>())
        {
        }

        Enum getType() const 
        { 
            return (Enum)type;
        }

        static const TypeName& getTypeName(Handle::Enum _enum);

        const TypeName& getTypeName() const
        {
            return getTypeName(getType());
        }

        inline bool operator == (const Handle& _rhs) const {
            return id == _rhs.id && type == _rhs.type;
        }

        inline bool operator != (const Handle& _rhs) const {
            return id != _rhs.id;
        }

        inline bool operator < (const Handle& _rhs) const {
            return id < _rhs.id;
        }

        inline operator size_t() const {
            return ((size_t)type << 16) | id;
        }


        uint16_t id{ kInvalidHandle };
        uint16_t type{ Count };
    };

#define IMPLEMENT_HANDLE(_name)                                     \
	template<>                                                      \
	constexpr Handle::Enum Handle::toEnum<_name##Handle>()          \
	{                                                               \
		return Handle::_name;                                       \
	}                                                               \

    IMPLEMENT_HANDLE(Shader);
    IMPLEMENT_HANDLE(Program);
    IMPLEMENT_HANDLE(Pass);
    IMPLEMENT_HANDLE(Buffer);
    IMPLEMENT_HANDLE(Image);

#undef IMPLEMENT_HANDLE
    

    struct CombinedResID
    {
        uint16_t        id{kInvalidHandle};
        ResourceType    type;

        CombinedResID() = default;
        CombinedResID(uint16_t id, ResourceType type) : id(id), type(type) {}

        // for tinystl::hash(const T& value)
        // which requires to cast to size_t
        operator size_t() const
        {
            return (size_t)type << 16 | id;
        }

        bool operator == (const CombinedResID& rhs) const {
            return id == rhs.id && type == rhs.type;
        }

        bool operator < (const CombinedResID& rhs) const {
            uint32_t loc_c = (uint32_t)type << 16 | id;
            uint32_t rhs_c = (uint32_t)rhs.type << 16 | rhs.id;
            return loc_c < rhs_c;
        }
    };

    inline bool isBuffer(const CombinedResID& _id)
    {
        return _id.type == ResourceType::buffer;
    }

    inline bool isImage(const CombinedResID& _id)
    {
        return _id.type == ResourceType::image;
    }

    struct ImageMetaData : public ImageDesc
    {
        ImageMetaData() {};
        ImageMetaData(const ImageDesc& desc) : ImageDesc(desc) {}

        uint16_t    imgId{ kInvalidHandle };
        uint16_t    bpp{ 4u };

        uint32_t    size{ 0 };
        void*       pData{ nullptr };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };
        ImageAspectFlags    aspectFlags{ 0 };
    };

    struct ImageCreate
    {
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        uint32_t depth{ 1 };
        uint16_t numLayers{ 1 };
        uint16_t numMips{ 1 };

        ImageType           type{ ImageType::type_2d };
        ImageViewType       viewType{ ImageViewType::type_2d };
        ImageLayout         layout{ ImageLayout::undefined };
        ResourceFormat      format{ ResourceFormat::undefined };
        ImageUsageFlags     usage{ ImageUsageFlagBits::color_attachment };
        ImageAspectFlags    aspect{ 0 };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };

        const Memory*       mem{nullptr};
    };

    struct BufferMetaData : public BufferDesc
    {
        BufferMetaData() {};
        BufferMetaData(const BufferDesc& desc) : BufferDesc(desc) {};

        void*       pData{ nullptr };
        uint16_t    bufId{ kInvalidHandle };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };
    };

    // generate barrier, bind descriptor set
    struct NO_VTABLE ResInteractDesc
    {
        uint32_t            binding{ kInvalidDescriptorSetIndex };

        PipelineStageFlags  stage{ PipelineStageFlagBits::none };
        AccessFlags         access{ AccessFlagBits::none };

        ImageLayout         layout{ ImageLayout::general };

        inline bool operator == (const ResInteractDesc& rhs) const {
            return binding == rhs.binding &&
                stage == rhs.stage &&
                access == rhs.access &&
                layout == rhs.layout;
        }

        inline bool operator != (const ResInteractDesc& rhs) const {
            return !(*this == rhs);
        }
    };

    struct WriteOperationAlias
    {
        uint16_t writeOpIn;
        uint16_t writeOpOut;

        WriteOperationAlias() = default;

        WriteOperationAlias(uint16_t _resIn, uint16_t _resOut)
            : writeOpIn(_resIn)
            , writeOpOut(_resOut) 
        {}

        inline bool operator == (const WriteOperationAlias& rhs) const {
            return writeOpIn == rhs.writeOpIn &&
                writeOpOut == rhs.writeOpOut;
        }

        inline bool operator != (const WriteOperationAlias& rhs) const {
            return !(*this == rhs);
        }
    };

    struct BufferCreateInfo : public BufferDesc
    {
        uint16_t    bufId{ kInvalidHandle };
        void*       pData{ nullptr };
        uint16_t    resCount{ 0 };

        ResInteractDesc    barrierState;
    };

    struct BufferAliasInfo
    {
        uint16_t    bufId{ kInvalidHandle };
        uint32_t    size{ 0 };
    };

    struct BufferAliasInfo2
    {
        BufferHandle    handle{ kInvalidHandle };
        uint32_t        size{ 0 };

    };

    struct ImageCreateInfo : public ImageDesc
    {
        uint16_t    imgId{ kInvalidHandle };
        uint16_t    resCount{ 0 };

        uint32_t    size;
        void*       pData;

        ImageAspectFlags    aspectFlags;
        ResInteractDesc    barrierState;
    };

    struct ImageAliasInfo
    {
        uint16_t    imgId{ kInvalidHandle };
    };

    struct SamplerDesc
    {
        SamplerDesc() {};

        SamplerFilter filter{ SamplerFilter::nearest };
        SamplerAddressMode addressMode{ SamplerAddressMode::clamp_to_edge };
        SamplerReductionMode reductionMode{ SamplerReductionMode::weighted_average };

        bool operator == (const SamplerDesc& rhs) const {
            return filter == rhs.filter &&
                addressMode == rhs.addressMode &&
                reductionMode == rhs.reductionMode;
        }
    };

    struct SamplerMetaData : public SamplerDesc
    {
        SamplerMetaData() = default;
        SamplerMetaData(const SamplerDesc& desc) : SamplerDesc(desc) {}

        uint16_t samplerId{kInvalidHandle};
    };

    struct ShaderCreateInfo 
    {
        uint16_t shaderId{ kInvalidHandle };
        uint16_t pathLen{ 0 };
    };
    
    struct ProgramDesc
    {
        uint16_t progId{ kInvalidHandle };
        uint16_t shaderNum{ 0 };
        uint32_t sizePushConstants{ 0 };

        uint16_t shaderIds[kMaxNumOfShaderInProgram]{ kInvalidHandle };
    };

    struct ProgramCreateInfo : public ProgramDesc
    {
    };

    struct PassMetaData : public PassDesc
    {
        PassMetaData() {};
        PassMetaData(const PassDesc& desc) : PassDesc(desc) {}

        uint16_t    passId{ kInvalidHandle };

        // graphics pass specific
        uint16_t    vertexBufferId{ kInvalidHandle };
        uint16_t    indexBufferId{ kInvalidHandle };

        uint16_t    indirectBufferId{ kInvalidHandle };
        uint16_t    indirectCountBufferId{ kInvalidHandle };

        uint16_t    writeDepthId{ kInvalidHandle };

        uint16_t    readImageNum{ 0 };
        uint16_t    readBufferNum{ 0 };

        uint16_t    writeImageNum{ 0 };
        uint16_t    writeBufferNum{ 0 };

        uint16_t    writeBufAliasNum{ 0 };
        uint16_t    writeImgAliasNum{ 0 };

        uint16_t    sampleImageNum{ 0 };

        uint32_t    indexCount{ 0 };
        uint32_t    vertexCount{ 0 };
        uint32_t    instanceCount{ 0 };

        uint32_t    indirectBufOffset{ 0 };
        uint32_t    indirectCountBufOffset{ 0 };
        uint32_t    indirectMaxDrawCount{ 0 };
        uint32_t    indirectBufStride{ 0 };
    };

    struct RHIBrief
    {
        PassHandle finalPass{ kInvalidHandle };
        ImageHandle presentImage{ kInvalidHandle };
        uint32_t presentMipLevel{ 0 };
    };

    enum class HandleType : uint16_t
    {
        unknown = 0,

        pass,
        shader,
        program,
        image,
        buffer,
        sampler,
    };


    using String = bx::StringT<&g_bxAllocator>;
    
    const char* getName(Handle _h);

} // namespace kage