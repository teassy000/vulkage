#include "brixelizer_intg.h"
#include "debug.h"


#include "scene.h"
#include "mesh.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_major_storage.hpp>


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

// grab from shader/math.h
vec3 rotateQuat(const vec3 _v, const quat _q) 
{
    return _v + 2.f * glm::cross(vec3(_q.x, _q.y, _q.z), glm::cross(vec3(_q.x, _q.y, _q.z), _v) + _q.w * _v);
}

mat4 modelMatrix(const vec3& _pos, const quat& _orit, const vec3& _scale, bool _rowMajor = false)
{
    mat4 model = mat4(1.f);

    model = glm::scale(model, _scale);
    model = glm::toMat4(_orit) * model;
    model = glm::translate(model, _pos);

    if (_rowMajor)
        model = glm::rowMajor4(model);

    return model;
}

void registerBrixelizerBuffers(FFX_Brixelizer_Impl& _ffx, const Scene& _scene, const kage::BufferHandle _vtxBuf, const kage::BufferHandle _idxBuf)
{
    FfxBrixelizerBufferDescription bufferDescs[2] = {};
    typedef struct FfxBrixelizerBufferDescription {
        FfxResource  buffer;        ///< An <c><i>FfxResource</i></c> of the buffer.
        uint32_t* outIndex;      ///< A pointer to a <c><i>uint32_t</i></c> to receive the index assigned to the buffer.
    } FfxBrixelizerBufferDescription;

    FfxResource vtxRes;
    vtxRes.resource = kage::getRHIResource(_vtxBuf);
    vtxRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
    vtxRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
    vtxRes.description.size = uint32_t(_scene.geometry.vertices.size() * sizeof(Vertex));
    vtxRes.description.stride = sizeof(Vertex);
    vtxRes.description.alignment = 0;
    vtxRes.description.mipCount = 0;
    vtxRes.description.flags = FFX_RESOURCE_FLAGS_NONE;
    vtxRes.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
    vtxRes.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
    std::string nameStr = "brixelizer vertex buffer";
    std::copy(nameStr.begin(), nameStr.end(), vtxRes.name);

    FfxResource idxRes;
    idxRes.resource = kage::getRHIResource(_idxBuf);
    idxRes.description.type = FFX_RESOURCE_TYPE_BUFFER;
    idxRes.description.format = FFX_SURFACE_FORMAT_UNKNOWN;
    idxRes.description.size = uint32_t(_scene.geometry.indices.size() * sizeof(uint32_t));
    idxRes.description.stride = sizeof(uint32_t);
    idxRes.description.alignment = 0;
    idxRes.description.mipCount = 0;
    idxRes.description.flags = FFX_RESOURCE_FLAGS_NONE;
    idxRes.description.usage = FFX_RESOURCE_USAGE_READ_ONLY;
    idxRes.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
    nameStr = "brixelizer index buffer";
    std::copy(nameStr.begin(), nameStr.end(), idxRes.name);

    bufferDescs[0].buffer = vtxRes;
    bufferDescs[1].buffer = idxRes;

    ffxBrixelizerRegisterBuffers(&_ffx.context, bufferDescs, COUNTOF(bufferDescs));
}

void createBrixelizerInstances(FFX_Brixelizer_Impl& _ffx, const Scene& _scene, bool _seamless = false)
{
    assert(!_seamless); // not support seamless yet

    std::vector<FfxBrixelizerInstanceDescription> instDescs;
    std::vector<FfxBrixelizerInstanceID> instIds;
    instDescs.reserve(_scene.drawCount);
    instDescs.reserve(_scene.drawCount);


    for (uint32_t ii = 0; ii < _scene.drawCount; ++ ii)
    {
        const MeshDraw& mdraw = _scene.meshDraws[ii];
        const Mesh& mesh = _scene.geometry.meshes[mdraw.meshIdx];

        vec3 center = mesh.center;
        float radius = mesh.radius;
        vec3 oriAabbMin = center - radius;
        vec3 oriAabbMax = center + radius;
        vec3 aabbExtent = oriAabbMax - oriAabbMin;

        const vec3 aabbCorners[8] = {
            oriAabbMin + vec3(0.f),
            oriAabbMin + vec3(aabbExtent.x, 0.f, 0.f),
            oriAabbMin + vec3(0.f, 0.f, aabbExtent.z),
            oriAabbMin + vec3(aabbExtent.x, 0.f, aabbExtent.z),
            oriAabbMin + vec3(0.f, aabbExtent.y, 0.f),
            oriAabbMin + vec3(aabbExtent.x, aabbExtent.y, 0.f),
            oriAabbMin + vec3(0.f, aabbExtent.y, aabbExtent.z),
            oriAabbMin + aabbExtent,
        };

        vec3 minAABB = vec3(FLT_MAX);
        vec3 maxAABB = vec3(FLT_MIN);
        for (uint32_t jj = 0; jj < 8; ++jj) {
            vec3 worldPos = rotateQuat(aabbCorners[ii], mdraw.orit)* mdraw.scale + mdraw.pos;
            minAABB = glm::min(minAABB, worldPos);
            maxAABB = glm::max(maxAABB, worldPos);
        }

        glm::mat4 transform = modelMatrix(mdraw.pos, mdraw.orit, vec3(mdraw.scale), true);

        FfxBrixelizerInstanceDescription instDesc = {};
        FfxBrixelizerInstanceID instId = FFX_BRIXELIZER_INVALID_ID;

        instDesc.maxCascade = c_brixelizerCascadeCount;
        for (uint32_t jj = 0; jj < 3; ++jj) {
            instDesc.aabb.min[jj] = minAABB[jj];
            instDesc.aabb.max[jj] = maxAABB[jj];
        }

        for (uint32_t jj = 0; jj < 3; ++jj)
            for (uint32_t kk = 0; kk < 4; ++kk)
                instDesc.transform[jj * 4 + kk] = transform[jj][kk];

        instDesc.indexFormat = FFX_INDEX_TYPE_UINT32;
        instDesc.indexBuffer = 0; // now we only have one buffer
        instDesc.indexBufferOffset = mesh.lods[0].indexOffset;
        instDesc.triangleCount = mesh.lods[0].indexCount / 3;
        instDesc.vertexBuffer = 0; // now we only have one buffer
        instDesc.vertexStride = sizeof(Vertex);
        instDesc.vertexBufferOffset = mesh.vertexOffset;
        instDesc.vertexCount = mesh.vertexCount;
        instDesc.vertexFormat = FFX_SURFACE_FORMAT_R32G32B32_FLOAT;
        instDesc.flags = FFX_BRIXELIZER_INSTANCE_FLAG_NONE; //static

        instIds.emplace_back(instId);

        instDesc.outInstanceID = &(instIds.back());

        instDescs.emplace_back(instDesc);
    }

    ffxBrixelizerCreateInstances(&_ffx.context, instDescs.data(), (uint32_t)instDescs.size());
}

void initBrixelizerImpl(FFX_Brixelizer_Impl& _ffx, const kage::BufferHandle _vtxBuf, const kage::BufferHandle _idxBuf)
{
    kage::getFFXInterface(static_cast<void*>(&_ffx.initDesc.backendInterface));

    createBrixelizerCtx(_ffx);

    _ffx.vtxBuf = _vtxBuf;
    _ffx.idxBuf = _idxBuf;
}

void postInitBrixelizerImpl(FFX_Brixelizer_Impl& _ffx, const Scene& _scene)
{
    registerBrixelizerBuffers(_ffx, _scene, _ffx.vtxBuf, _ffx.idxBuf);
    createBrixelizerInstances(_ffx, _scene);
}

void destroyBrixelizerImpl(FFX_Brixelizer_Impl& _ffx)
{
    ffxBrixelizerContextDestroy(&_ffx.context);
}

void updateBrixellizerImpl(const FFX_Brixelizer_Impl& _ffx)
{

}
