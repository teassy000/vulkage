#include "common.h"
#include "macro.h"
#include "vk_resource.h"

#include "memory_operation.h"
#include "config.h"
#include "vkz.h"
#include "res_creator.h"
#include "vk_res_creator.h"


namespace vkz
{
    void ResCreator_VK::createPass(MemoryReader& reader)
    {
        PassCreateInfo info;
        read(&reader, info);

    }

    void ResCreator_VK::createImage(MemoryReader& reader)
    {
        ImageCreateInfo info;
        read(&reader, info);

        // void createImage(Image& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProps, const VkFormat format, const ImgInitProps createInfo, ImageAliases = {}, const uint32_t aliasCount = 0 );

    }

    void ResCreator_VK::createBuffer(MemoryReader& _reader)
    {
        BufBucketInfo info;
        read(&_reader, info);

        VK_BufAliasInfo* resArr = new VK_BufAliasInfo[info.resNum];
        read(&_reader, resArr, sizeof(VK_BufAliasInfo) * info.resNum);

        // so the res handle should map to the real buffer array
        
        std::vector<VK_BufAliasInfo> infoList(resArr, resArr + info.resNum);

        std::vector<VK_Buffer> buffers;
        vkz::createBuffer(buffers, infoList, m_memProps, m_device, (VkBufferUsageFlags)info.usage, (VkMemoryPropertyFlags)info.memFlag);
    }

} // namespace vkz