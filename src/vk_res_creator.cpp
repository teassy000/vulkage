#include "common.h"
#include "macro.h"

#include "memory_operation.h"
#include "config.h"
#include "vkz.h"
#include "res_creator.h"
#include "vk_resource.h"
#include "vk_shaders.h"
#include "vk_res_creator.h"


namespace vkz
{
    void ResCreator_vk::createPass(MemoryReader& _reader)
    {
        PassCreateInfo_vk info;
        read(&_reader, info);

        // create pass
        if ((uint16_t)PassType::Compute == info.type)
        {
            createComputePass(_reader, info);
        }
        if ((uint16_t)PassType::Graphics == info.type)
        {
            createGraphicsPass(_reader, info);
        }
    }

    void ResCreator_vk::createImage(MemoryReader& _reader)
    {
        ImgBucketInfo_vk info;
        read(&_reader, info);

        // void createImage(Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProps, const VkFormat format, const ImgInitProps createInfo, ImageAliases = {}, const uint32_t aliasCount = 0 );
        ImgAliasInfo_vk* resArr = new ImgAliasInfo_vk[info.resNum];
        read(&_reader, resArr, sizeof(ImgAliasInfo_vk) * info.resNum);

        // so the res handle should map to the real buffer array
        std::vector<ImgAliasInfo_vk> infoList(resArr, resArr + info.resNum);

        ImgInitProps initPorps{};
        initPorps.level = info.level;
        initPorps.width = info.width;
        initPorps.height = info.height;
        initPorps.depth = info.depth;
        initPorps.format = (VkFormat)info.format;
        initPorps.usage = (VkImageUsageFlags)info.usage;
        initPorps.type = (VkImageType)info.type;
        initPorps.layout = (VkImageLayout)info.layout;
        initPorps.viewType = (VkImageViewType)info.viewType;

        std::vector<Image_vk> images;
        vkz::createImage(images, infoList, m_device, m_memProps, initPorps);

        assert(images.size() == info.resNum);
        assert(m_imageIds.size() == m_images.size());

        // fill buffers and IDs
        m_imageIds.resize(m_imageIds.size() + info.resNum);
        m_images.resize(m_images.size() + info.resNum);
        for (int i = 0; i < info.resNum; ++i)
        {
            m_imageIds[m_imageIds.size() - info.resNum + i] = resArr[i].resId;
            m_images[m_images.size() - info.resNum + i] = images[i];
        }
    }

    void ResCreator_vk::createBuffer(MemoryReader& _reader)
    {
        BufBucketInfo_vk info;
        read(&_reader, info);

        BufAliasInfo_vk* resArr = new BufAliasInfo_vk[info.resNum];
        read(&_reader, resArr, sizeof(BufAliasInfo_vk) * info.resNum);

        // so the res handle should map to the real buffer array
        std::vector<BufAliasInfo_vk> infoList(resArr, resArr + info.resNum);

        std::vector<Buffer_vk> buffers;
        vkz::createBuffer(buffers, infoList, m_memProps, m_device, (VkBufferUsageFlags)info.usage, (VkMemoryPropertyFlags)info.memFlag);

        assert(buffers.size() == info.resNum);
        assert(m_bufferIds.size() == m_buffers.size());

        // fill buffers and IDs
        m_bufferIds.resize(m_bufferIds.size() + info.resNum);
        m_buffers.resize(m_buffers.size() + info.resNum);
        for (int i = 0; i < info.resNum; ++i)
        {
            m_bufferIds[m_bufferIds.size() - info.resNum + i] = resArr[i].resId;
            m_buffers[m_buffers.size() - info.resNum + i] = buffers[i];
        }

        VKZ_DELETE_ARRAY(resArr);
    }


    void ResCreator_vk::createComputePass(MemoryReader& _reader, const PassCreateInfo_vk& _info)
    {
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;


        //VkPipeline createComputePipeline(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout layout, const Shader_vk & shader, Constants constants = {});
    }


    void ResCreator_vk::createGraphicsPass(MemoryReader& _reader, const PassCreateInfo_vk& _info)
    {

    }

} // namespace vkz