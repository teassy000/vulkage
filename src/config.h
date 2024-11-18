#pragma once

namespace kage
{
    constexpr unsigned int kMaxNumOfShaderHandle = 1024;
    constexpr unsigned int kMaxNumOfPipelineHandle = 1024;
    constexpr unsigned int kMaxNumOfProgramHandle = 1024;

    constexpr unsigned int kMaxNumOfPassHandle = 1024;

    constexpr unsigned int kMaxNumOfImageHandle = 1024;
    constexpr unsigned int kMaxNumOfSamplerHandle = 1024;
    constexpr unsigned int kMaxNumOfBufferHandle = 1024;


    constexpr unsigned int kInitialFrameGraphMemSize = 16 * 1024; // 16k

    constexpr unsigned int kInitialConstantsTotalMemSize = 1024; // 1k
    constexpr unsigned int kInitialUniformMemSize = 16 * 1024; // 16k
    constexpr unsigned int kInitialStorageMemSize = 128 * 1024 * 1024; // 128M

    constexpr unsigned int kMaxPathLen = 256;
    constexpr unsigned int kMaxNumOfStageInPorgram = 6;

    constexpr unsigned int kMaxNumOfVertexAttribute = 16;
    constexpr unsigned int kMaxNumOfVertexBinding = 16;

    constexpr unsigned int kMaxNumOfDescriptorSet = 16;
    constexpr unsigned int kMaxNumOfDescriptorSetLayout = 16;

    constexpr unsigned int kMaxNumOfPushConstant = 16;

    constexpr unsigned int kMaxNumOfShaderInProgram = 6;

    constexpr unsigned int kMaxNumOfImageMipLevel = 16;

    constexpr unsigned int kMaxNumOfBackBuffers = 16;
    constexpr unsigned int kMaxNumFrameLatency = 3;
    constexpr unsigned int kMaxNumFrameBuffers = 128;

    constexpr unsigned int kScratchBufferSize = 128*1024*1024; // 128m
    constexpr unsigned int kMaxNumBufferInScratchBuffer = 1024;

    constexpr unsigned int kCommandBufferInitSize = 4 * 1024; // 4k

    constexpr unsigned int kMaxDrawCalls = ((64 << 10) - 1); // 65535

    // rendering config
    constexpr char kRegularLod = (char)0;
    constexpr char kSeamlessLod = !kRegularLod & (char)1;

    constexpr size_t kClusterSize = 64; // triangle count in each cluster
    constexpr size_t kMaxVtxInCluster = 64;
    constexpr size_t kGroupSize = 8;
    constexpr bool kUseNormals = true;

    constexpr bool kUseMetisPartition = true;
}