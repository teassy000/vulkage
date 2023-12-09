#pragma once

namespace vkz
{
    enum class ShaderStage_vk : uint16_t
    {
        Invalid = 1 << 0,
        Vertex = 1 << 1,
        Mesh = 1 << 2,
        Task = 1 << 3,
        Fragment = 1 << 4,
        Compute = 1 << 5,
    };

    typedef uint16_t ShaderStageBits_vk;

    struct PassCreateInfo_vk
    {
        uint16_t    passId;
        uint16_t    readResNum; // 32bit res info
        uint16_t    writeResNum; // 32bit res info
        uint16_t    type; // 1: compute, 2: graphics

        uint32_t    programInfoOffset;
        uint32_t    programInfoSize; // map to program info

        uint32_t    pushConstantOffset;
        uint32_t    pushConstantSize; // map to push constant
        
        uint32_t    renderInfoOffset;
        uint32_t    renderInfoSize; // map to render info

        uint32_t    vertexInputStateOffset;
        uint32_t    vertexInputStateSize; // map to vertex input state

        uint32_t    pipelineConfigOffset;
        uint32_t    pipelineConfigSize;
    };

    struct ShaderCreateInfo_vk
    {
        uint16_t    shaderId;
        uint32_t    codeOffset;
        uint32_t    codeSize;
    };

    struct ProgramCreateInfo_vk
    {
        uint16_t    programId;
        uint16_t    shaderNum;
        uint32_t    shaderInfoOffset;
        uint32_t    shaderInfoSize;
    };

    struct PipelineConfig_vk
    {
        uint16_t enableDepthTest;
        uint16_t enableDepthWrite;
        VkCompareOp depthCompOp{ VK_COMPARE_OP_GREATER };
    };

    struct BufBucketInfo_vk
    {
        uint16_t    bucketId;
        uint16_t    resNum;
        
        uint32_t    size;
        uint32_t    usage;
        uint32_t    memFlag;
        bool        forceAliased{ false };
    };

    struct ImgBucketInfo_vk
    {
        uint16_t    bucketId;
        uint16_t    resNum;

        uint32_t    memFlag;
        
        uint32_t    level;

        uint32_t    width;
        uint32_t    height;
        uint32_t    depth;

        uint32_t    format;
        uint32_t    usage;
        uint32_t    type{ VK_IMAGE_TYPE_2D };
        uint32_t    layout{ VK_IMAGE_LAYOUT_GENERAL };
        uint32_t    viewType{ VK_IMAGE_VIEW_TYPE_2D };

        bool        forceAliased{ false };
    };

    class ResCreator_vk : public ResCreator
    {
    private:
        void createPass(MemoryReader& _reader) override;
        void createImage(MemoryReader& _reader) override;
        void createBuffer(MemoryReader& _reader) override;

        void createGraphicsPass(MemoryReader& _reader, const PassCreateInfo_vk& _info);
        void createComputePass(MemoryReader& _reader, const PassCreateInfo_vk& _info);

    private:
        VkDevice m_device;
        VkPhysicalDevice m_phyDevice;
        VkPhysicalDeviceMemoryProperties m_memProps;

        std::vector<uint16_t>   m_bufferIds;
        std::vector<Buffer_vk>  m_buffers;

        std::vector<uint16_t>   m_imageIds;
        std::vector<Image_vk>   m_images;
    };
} // namespace vkz