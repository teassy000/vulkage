#pragma once

namespace vkz
{
    constexpr unsigned int kMaxNumOfShaderHandle = 1024;
    constexpr unsigned int kMaxNumOfPipelineHandle = 1024;
    constexpr unsigned int kMaxNumOfProgramHandle = 1024;

    constexpr unsigned int kMaxNumOfPassHandle = 1024;

    constexpr unsigned int kMaxNumOfImageHandle = 1024;
    constexpr unsigned int kMaxNumOfImageViewHandle = 1024;
    constexpr unsigned int kMaxNumOfSamplerHandle = 1024;
    constexpr unsigned int kMaxNumOfBufferHandle = 1024;


    constexpr unsigned int kInitialFrameGraphMemSize = 16 * 1024; // 16k

    constexpr unsigned int kInitialUniformTotalMemSize = 16 * 1024; // 16k

    constexpr unsigned int kMaxPathLen = 256;
    constexpr unsigned int kMaxNumOfStageInPorgram = 6;

    constexpr unsigned int kMaxNumOfVertexAttribute = 16;
    constexpr unsigned int kMaxNumOfVertexBinding = 16;

    constexpr unsigned int kMaxNumOfDescriptorSet = 16;
    constexpr unsigned int kMaxNumOfDescriptorSetLayout = 16;

    constexpr unsigned int kMaxNumOfPushConstant = 16;

    constexpr unsigned int kMaxNumOfShaderInProgram = 6;

    constexpr unsigned int kMaxNumOfImageMipLevel = 16;
    constexpr unsigned int kAllMipLevel = ~0;
}