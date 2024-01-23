#ifndef __VKZ_STRUCTS_INNER_H__
#define __VKZ_STRUCTS_INNER_H__

#include "vkz_structs.h"
#include "config.h"

namespace vkz
{
    enum class ResourceType : uint16_t
    {
        buffer = 0,
        image,
    };

    struct CombinedResID
    {
        uint16_t        id{kInvalidHandle};
        ResourceType    type;

        CombinedResID() = default;
        CombinedResID(uint16_t id, ResourceType type) : id(id), type(type) {}

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
        ImageMetaData(const ImageDesc& desc) : ImageDesc(desc) {}

        uint16_t    imgId{ kInvalidHandle };

        uint16_t    bpp{ 4u };

        ResourceLifetime    lifetime{ ResourceLifetime::transition };
        ImageAspectFlags    aspectFlags{ 0 };
    };

    struct BufferMetaData : public BufferDesc
    {
        BufferMetaData(const BufferDesc& desc) : BufferDesc(desc) {}

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
            : writeOpIn(_resIn), writeOpOut(_resOut) 
        {}

        inline bool operator == (const WriteOperationAlias& rhs) const {
            return writeOpIn == rhs.writeOpIn &&
                writeOpOut == rhs.writeOpOut;
        }

        inline bool operator != (const WriteOperationAlias& rhs) const {
            return !(*this == rhs);
        }
    };

    struct SpecificImageViewInfo
    {
        uint32_t    baseMip;
        uint32_t    mipLevels;

        inline bool operator == (const SpecificImageViewInfo& rhs) const {
            return baseMip == rhs.baseMip &&
                mipLevels == rhs.mipLevels;
        }

        inline bool operator != (const SpecificImageViewInfo& rhs) const {
            return !(*this == rhs);
        }
    };

    inline SpecificImageViewInfo defaultSpecificImageViewInfo()
    {
        return { 0, kAllMipLevel };
    }

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


    struct BufferCreateInfo : public BufferDesc
    {
        uint16_t    bufId{ kInvalidHandle };
        uint16_t    aliasNum{ 0 };

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
        uint16_t    aliasNum{ 0 };

        ImageAspectFlags    aspectFlags;
        ResInteractDesc    barrierState;
    };

    struct ImageAliasInfo
    {
        uint16_t    imgId{ kInvalidHandle };
    };

    using SamplerHandle = Handle<struct SamplerHandleTag>;

    struct SamplerDesc
    {
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
        void* pPushConstants{ nullptr };

        uint16_t shaderIds[kMaxNumOfShaderInProgram]{ kInvalidHandle };
    };

    struct ProgramCreateInfo : public ProgramDesc
    {

    };

    struct PassMetaData : public PassDesc
    {
        PassMetaData() = default;
        PassMetaData(const PassDesc& desc) : PassDesc(desc) {}

        uint16_t    passId{ kInvalidHandle };

        // graphics pass specific
        uint16_t    vertexBufferId{ kInvalidHandle };
        uint16_t    indexBufferId{ kInvalidHandle };
        
        uint16_t    indirectBufferId{ kInvalidHandle };
        uint16_t    indirectCountBufferId{ kInvalidHandle };

        uint16_t    writeDepthId{ kInvalidHandle };

        uint16_t    writeImageNum{ 0 };
        uint16_t    readImageNum{ 0 };

        uint16_t    readBufferNum{ 0 };
        uint16_t    writeBufferNum{ 0 };

        uint16_t    writeBufAliasNum{ 0 };
        uint16_t    writeImgAliasNum{ 0 };

        uint16_t    sampleImageNum{ 0 };

        uint32_t    specImageViewNum{ 0 }; // count of image view for specific mip levels

        uint32_t    indirectBufOffset{ 0 };
        uint32_t    indirectCountBufOffset{ 0 };
        uint32_t    defaultIndirectMaxCount{ 1 };
        uint32_t    indirectBufStride{ 0 };
    };

    struct RHIBrief
    {
        uint16_t finalPassId{ kInvalidHandle };
        uint16_t presentImageId{ kInvalidHandle };
    };

    struct RHI_Config
    {
        uint32_t windowWidth{0};
        uint32_t windowHeight{0};
    };
} // namespace vkz

#endif // __VKZ_STRUCTS_INNER_H__