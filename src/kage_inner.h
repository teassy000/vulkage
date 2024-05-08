#pragma once

#include "cmd_list.h"
#include "config.h"
#include "kage.h"

#include "entry/entry.h"


namespace kage
{
    extern bx::AllocatorI* g_bxAllocator;

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

    struct SpecImageViewsCreate
    {
        SpecImageViewsCreate() {};
        ImageHandle         image{ kInvalidHandle };
        uint32_t            viewCount{ 0 };
        ImageViewHandle     views[kMaxNumOfImageMipLevel]{ kInvalidHandle };
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

    struct ImageViewDesc
    {
        ImageViewDesc() {};

        uint16_t    imgId{ kInvalidHandle };
        uint16_t    imgViewId{ kInvalidHandle };
        uint32_t    baseMip{0};
        uint32_t    mipLevel{ 0 };

        inline bool operator == (const ImageViewDesc& rhs) const {
            return imgId == rhs.imgId &&
                baseMip == rhs.baseMip &&
                mipLevel == rhs.mipLevel;
        }

        inline bool operator != (const ImageViewDesc& rhs) const {
            return !(*this == rhs);
        }
    };

    inline ImageViewDesc defaultImageView(const ImageHandle _hImg)
    {
        ImageViewDesc desc;

        return desc;
    }

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

    using RenderFuncPtr = void (*)(CommandListI& _cmdList, const void* _data, uint32_t _size);
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

        RenderFuncPtr   renderFunc{ nullptr };
        void* renderFuncDataPtr{ nullptr };
        uint32_t renderFuncDataSize{ 0 };
    };

    struct RHIBrief
    {
        uint16_t finalPassId{ kInvalidHandle };
        uint16_t presentImageId{ kInvalidHandle };
        uint32_t presentMipLevel{ 0 };
    };

    namespace NameTags
    {
        // tag for varies of handle 
        static const char* kShader = "[sh]";
        static const char* kRenderPass = "[pass]";
        static const char* kProgram = "[prog]";
        static const char* kImage = "[img]";
        static const char* kBuffer = "[buf]";
        static const char* kSampler = "[samp]";
        static const char* kImageView = "[imgv]";

        // tag for alias: buffer, image
        static const char* kAlias = "[alias]";
    }

    enum class HandleType : uint16_t
    {
        unknown = 0,

        pass,
        shader,
        program,
        image,
        buffer,
        sampler,
        image_view,
    };

    struct HandleSignature
    {
        HandleType  type;
        uint16_t    id;

        // for tinystl::hash(const T& value)
        // which requires to cast to size_t
        operator size_t() const
        {
            return ((size_t)(type) << 16) | id;
        }
    };


    using String = bx::StringT<&g_bxAllocator>;
    
    const char* getName(ShaderHandle _hShader);
    const char* getName(ProgramHandle _hProg);
    const char* getName(PassHandle _hPass);
    const char* getName(ImageHandle _hImg);
    const char* getName(BufferHandle _hBuf);
    const char* getName(ImageViewHandle _hImgView);
    const char* getName(SamplerHandle _hSampler);
} // namespace kage