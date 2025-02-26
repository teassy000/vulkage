// ==============================================================================
#define sectorize(value) step(0.0, (value)) * 2.0 - 1.0
#define sum(value) dot(clamp((value), 1.0, 1.0), (value))


// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec2 octWrap(vec2 _v)
{
	return (1.0 - abs(_v.yx)) * (_v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 octEncode(vec3 _n)
{
    _n /= sum(_n);
    _n.xy = _n.z >= 0.0 ? _n.xy : octWrap(_n.xy);
    _n.xy = _n.xy * 0.5 + 0.5;
    return _n.xy;
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec3 octDecode(vec2 _uv)
{
	_uv = _uv * 2.0 - 1.0;

	vec2 suv = sectorize(_uv);
    vec2 auv = abs(_uv);

    float l = sum(auv);

	if (l > 1.0)
	{
		_uv = (1. - auv.yx) * suv;
	}

	return normalize(vec3(_uv.x, 1. - l, _uv.y));
}

// ==============================================================================

// get 3d pos from linear index
ivec3 getWorld3DIdx(uint _idx, uint _sideCnt)
{
    uint z = _idx / (_sideCnt * _sideCnt);
    uint y = (_idx % (_sideCnt * _sideCnt)) / _sideCnt;
    uint x = _idx % _sideCnt;

    return ivec3(x, y, z);
}

// get 3d world pos based on the index
vec3 getCenterWorldPos(ivec3 _idx, float _sceneRadius, float _voxSideLen)
{
    // the voxel idx is from [0 ,sideCnt - 1]
    // the voxel in world space is [-_sceneRadius, _sceneRadius]
    vec3 pos = vec3(_idx) * _voxSideLen - vec3(_sceneRadius) - vec3(_voxSideLen) * 0.5f;
    return pos;
}

// ==============================================================================
struct RadianceCascadesConfig
{
    uint    probe_sideCount;
    uint    ray_gridSideCount;
    uint    level;
    uint    layerOffset;

    float   rayLength;
    float   probeSideLen;

    // oct-tree data
    uint    ot_voxSideCount;
    float   ot_sceneRadius;
};

struct VoxelizationConsts
{
    mat4 proj;
    uint voxGridCount;
    float voxCellLen;
    float sceneRadius;
};

// ==============================================================================

#define INVALID_OCT_IDX 0xFFFFFFFFu
#define INVALID_VOX_ID 0xFFFFFFFFu

struct OctTreeNode
{
    uint dataIdx;
    uint lv;
    uint childs[8]; // if is leaf, it's the index of voxel, else the index in the octTree;
};

struct OctTreeProcessConfig
{
    uint lv;
    uint voxGridSideCount;
    uint readOffset;
    uint writeOffset;
};

struct VoxDebugConsts
{
    float voxRadius; // the radius of circumcircle of the voxel
    float voxSideLen;
    float sceneRadius;
    uint sceneSideCnt;

    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
};

struct DeferredConstants
{
    float sceneRadius;
    uint cascade_lv;
    uint cascade_0_probGridCount;
    uint cascade_0_rayGridCount;
    vec2 imageSize;
    vec3 camPos;
};


struct VoxDraw
{
    vec3 pos;
    vec3 col;
};

struct ProbeDebugCmdConsts
{
    float sceneRadius;
    float probeSideLen;
    float sphereRadius;
    uint probeSideCount;
    uint layerOffset;
    uint idxCnt;

    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;
};

struct ProbeDraw
{
    vec3 pos;
    ivec3 idx;
};

struct ProbeDebugDrawConsts
{
    float sceneRadius;
    float sphereRadius;

    uint probeSideCount;
};