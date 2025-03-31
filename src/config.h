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

    constexpr unsigned int kInitialUniformMemSize = 16 * 1024; // 16k
    constexpr unsigned int kInitialStorageMemSize = 128 * 1024 * 1024; // 128M

    constexpr unsigned int kMaxPathLen = 256;
    constexpr unsigned int kMaxNumOfStageInPorgram = 6;

    constexpr unsigned int kMaxNumOfPushConstant = 16;

    constexpr unsigned int kMaxNumOfShaderInProgram = 6;

    constexpr unsigned int kMaxNumOfBackBuffers = 16;
    constexpr unsigned int kMaxNumFrameLatency = 3;
    constexpr unsigned int kMaxNumFrameBuffers = 128;

    constexpr unsigned int kMaxDrawCalls = ((64 << 10) - 1); // 65535

    // bind-less setting
    constexpr unsigned int kMaxNumOfBindlessResHandle = 16;

    constexpr unsigned int kMaxDescriptorCount = 65535;
    constexpr unsigned int kImageSamplerBindingSlot = 0;
    constexpr unsigned int kMaxNumOfSamplerDescSets = 65535;

    // rendering config
    constexpr char kRegularLod = (char)1;
    constexpr char kSeamlessLod = !kRegularLod;

    constexpr size_t kClusterSize = 64; // triangle count in each cluster
    constexpr size_t kMaxVtxInCluster = 64;
    constexpr size_t kGroupSize = 8;
    constexpr bool kUseNormals = true;

    constexpr bool kUseMetisPartition = true;

    // radiance cascade config, it's a 3d grid of probes, each probe has a 2d grid of rays
    // ray grid is a 2d grid of rays encoded by octahedron mapping
    // following probe lv decreasing, probe count for each level decreates by 8, ray count for each level increases by 8
    constexpr unsigned int k_rclv0_cascadeLv = 4;
    constexpr unsigned int k_rclv0_probeSideCount = 32;
    constexpr unsigned int k_rclv0_rayGridSideCount = 16;
}