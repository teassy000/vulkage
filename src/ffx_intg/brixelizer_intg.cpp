#include "brixelizer_intg.h"
#include "debug.h"


constexpr uint32_t c_brixelizerCascadeCount = (FFX_BRIXELIZER_MAX_CASCADES / 3);

struct BrixelizerConfig
{
    float meshUnitSize{ .2f };
    float cascadeSizeRatio{ 2.f };
};

const static BrixelizerConfig s_conf{};

void createBrixelizerCtx(FFX_Brixelizer_Impl& _ffx)
{
    _ffx.initDesc.sdfCenter[0] = 0.f;
    _ffx.initDesc.sdfCenter[1] = 0.f;
    _ffx.initDesc.sdfCenter[2] = 0.f;
    _ffx.initDesc.numCascades = c_brixelizerCascadeCount;
    _ffx.initDesc.flags = FFX_BRIXELIZER_CONTEXT_FLAG_ALL_DEBUG;

    float voxSize = s_conf.meshUnitSize;
    for (uint32_t ii = 0; ii < _ffx.initDesc.numCascades; ++ii)
    {
        FfxBrixelizerCascadeDescription& refDesc = _ffx.initDesc.cascadeDescs[ii];
        
        refDesc.flags = (FfxBrixelizerCascadeFlag)(FFX_BRIXELIZER_CASCADE_STATIC | FFX_BRIXELIZER_CASCADE_DYNAMIC);
        refDesc.voxelSize = voxSize;

        voxSize *= s_conf.cascadeSizeRatio;
    }

    _ffx.initDesc.backendInterface;

    FfxErrorCode err = ffxBrixelizerContextCreate(&_ffx.initDesc, &_ffx.context);
    if (err != FFX_OK)
        kage::message(kage::error, "failed to create brixelizer context with error code %d.", err);
}

void initBrixelizerImpl(FFX_Brixelizer_Impl& _ffx)
{
    FfxInterface ffxInterface;
    kage::getFFXInterface((void*)&ffxInterface);

    _ffx.initDesc.backendInterface = ffxInterface;

    createBrixelizerCtx(_ffx);
}

void updateBrixellizerImpl(const FFX_Brixelizer_Impl& _ffx)
{

}
