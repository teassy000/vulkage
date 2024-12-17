#include "common.h"
#include "kage.h"

struct SMAAEdgeDepth
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle prog{ kage::kInvalidHandle };
    kage::ShaderHandle cs{ kage::kInvalidHandle };

    kage::ImageHandle inDepth{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    kage::ImageHandle depth{ kage::kInvalidHandle };
    kage::ImageHandle depthOutAlias{ kage::kInvalidHandle };
};

struct SMAAEdgeColor
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle prog{ kage::kInvalidHandle };
    kage::ShaderHandle cs{ kage::kInvalidHandle };

    kage::ImageHandle inColor{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    kage::ImageHandle color{ kage::kInvalidHandle };
    kage::ImageHandle colorOutAlias{ kage::kInvalidHandle };
};

struct SMAAEdgeLuma
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle prog{ kage::kInvalidHandle };
    kage::ShaderHandle cs{ kage::kInvalidHandle };

    kage::ImageHandle inColor{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    kage::ImageHandle luma{ kage::kInvalidHandle };
    kage::ImageHandle lumaOutAlias{ kage::kInvalidHandle };
};

struct SMAAWeight
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle prog{ kage::kInvalidHandle };
    kage::ShaderHandle cs{ kage::kInvalidHandle };

    kage::ImageHandle inEdge{ kage::kInvalidHandle };
    kage::SamplerHandle edgeSampler{ kage::kInvalidHandle };

    kage::ImageHandle areaImg{ kage::kInvalidHandle };
    kage::ImageHandle searchImg{ kage::kInvalidHandle };

    kage::SamplerHandle areaSampler{ kage::kInvalidHandle };
    kage::SamplerHandle searchSampler{ kage::kInvalidHandle };

    kage::ImageHandle weight{ kage::kInvalidHandle };
    kage::ImageHandle weightOutAlias{ kage::kInvalidHandle };
};

struct SMAABlend
{
    kage::PassHandle pass{ kage::kInvalidHandle };
    kage::ProgramHandle prog{ kage::kInvalidHandle };
    kage::ShaderHandle cs{ kage::kInvalidHandle };

    kage::ImageHandle inWeight{ kage::kInvalidHandle };
    kage::SamplerHandle weightSamper{ kage::kInvalidHandle };

    kage::ImageHandle inColor{ kage::kInvalidHandle };
    kage::SamplerHandle colorSampler{ kage::kInvalidHandle };

    kage::ImageHandle blend{ kage::kInvalidHandle };
    kage::ImageHandle blendOutAlias{ kage::kInvalidHandle };
};

struct SMAA
{
    SMAAEdgeDepth m_edgeDepth;
    SMAAEdgeColor m_edgeColor;
    SMAAEdgeLuma m_edgeLuma;
    SMAAWeight m_weight;
    SMAABlend m_blend;

    uint32_t m_w{ 0 };
    uint32_t m_h{ 0 };

    kage::ImageHandle m_outAliasImg{ kage::kInvalidHandle };

    void prepare(uint32_t _width, uint32_t _height, kage::ImageHandle _inColor, kage::ImageHandle _inDepth);
    void update(uint32_t _rtWidth, uint32_t _rtHeight);
};

struct AntiAliasingPass
{
    kage::PassHandle pass{ kage::kInvalidHandle };


    kage::ProgramHandle edgeProg{ kage::kInvalidHandle };
    kage::ShaderHandle edgeCs{ kage::kInvalidHandle };

    kage::ImageHandle inColor{ kage::kInvalidHandle };
    kage::SamplerHandle sampler{ kage::kInvalidHandle };

    kage::ImageHandle edge{ kage::kInvalidHandle };
    kage::ImageHandle blend{ kage::kInvalidHandle };
    kage::ImageHandle color{ kage::kInvalidHandle };

    kage::ImageHandle colorOutAlias{ kage::kInvalidHandle };


    uint32_t w{ 0 };
    uint32_t h{ 0 };
};