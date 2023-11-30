#pragma once

namespace vkz
{
    struct PassCreateInfo
    {

    };

    struct ImageCreateInfo
    {

    };

    struct BufBucketInfo
    {
        uint16_t        bucketId;
        uint16_t        resNum;
        
        uint32_t        size;
        uint32_t        usage;
        uint32_t        memFlag;
        bool            forceAliased{ false };
    };

    class ResCreator_VK : public ResCreator
    {
    private:
        void createPass(MemoryReader& reader) override;
        void createImage(MemoryReader& reader) override;
        void createBuffer(MemoryReader& reader) override;

    private:
        VkDevice m_device;
        VkPhysicalDevice m_phyDevice;
        VkPhysicalDeviceMemoryProperties m_memProps;

        std::vector<uint16_t>   m_bufferResIds;
        std::vector<VK_Buffer>  m_buffers;

        std::vector<uint16_t>   m_imageResIds;
        std::vector<VK_Image>   m_images;
    };



    
} // namespace vkz