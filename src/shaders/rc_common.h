// ==============================================================================
#define sectorize(value) step(0.0, (value)) * 2.0 - 1.0
#define sum(value) dot(clamp((value), 1.0, 1.0), (value))
#define EPSILON 0.00001f
#define FLT_MAX 3.402823466e+38
#define MAX_RADIANCE_CASCADES 8

bool compare(float a, float b)
{
    return abs(a - b) < EPSILON;
}

bool compare(vec3 _a, vec3 _b)
{
    return compare(_a.x, _b.x) && compare(_a.y, _b.y) && compare(_a.z, _b.z);
}

bool compare(vec4 _a, vec4 _b)
{
    return compare(_a.x, _b.x) && compare(_a.y, _b.y) && compare(_a.z, _b.z) && compare(_a.w, _b.w);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
// following methods are from the above link
vec2 octWrap(vec2 _v)
{
	return (1.0 - abs(_v.yx)) * (_v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 octEncode(vec3 _n)
{
    _n /= (abs(_n.x) + abs(_n.y) + abs(_n.z));
    _n.xy = _n.z >= 0.0 ? _n.xy : octWrap(_n.xy);
    _n.xy = _n.xy * 0.5 + 0.5;
    return _n.xy;
}

vec3 octDecode(vec2 _uv)
{
	_uv = _uv * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3(_uv.x, _uv.y, 1.0 - abs(_uv.x) - abs(_uv.y));
    float t = clamp(-n.z, 0.f, 1.f);
    n.xy = vec2(n.xy.x >= 0.0 ? n.xy.x - t : n.xy.x + t, n.xy.y >= 0.0 ? n.xy.y - t : n.xy.y + t);
    return normalize(n);
}

// https://jcgt.org/published/0003/02/01/paper.pdf
// following methods are from the above link
// Returns ¡À1
vec2 signNotZero(vec2 v) {
    return vec2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}
// Assume normalized input. Output is on [-1, 1] for each component.
vec2 float32x3_to_oct(in vec3 v) {
    // Project the sphere onto the octahedron, and then onto the xy plane
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    // Reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

vec3 oct_to_float32x3(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    return normalize(v);
}


// vox ==============================================================================

// get 3d pos from linear index
ivec3 getWorld3DIdx(uint _idx, uint _sideCnt)
{
    uint z = _idx / (_sideCnt * _sideCnt);
    uint y = (_idx % (_sideCnt * _sideCnt)) / _sideCnt;
    uint x = _idx % _sideCnt;

    return ivec3(x, y, z);
}

// get 3d world pos based on the index
vec3 getCenterWorldPos(ivec3 _idx, float _sceneRadius, float _gridSideLen)
{
    // the voxel idx is from [0 ,sideCnt - 1]
    // the voxel in world space is [-_sceneRadius, _sceneRadius]
    vec3 pos = vec3(_idx) * _gridSideLen - vec3(_sceneRadius) + _gridSideLen * .5f;
    return pos;
}

// probe ============================================================================

// oct-tree =====================================================================

// 8 child
// zyx order from front to back, bottom to top, left to right
// [z][y][x]
// 0 = 000, 1 = 001, 2 = 010, 3 = 011
// 4 = 100, 5 = 101, 6 = 110, 7 = 111

// get the child 
uint getVoxChildGridIdx(ivec3 _vi, uint _voxSideCnt, uint _childIdx)
{
    uint actualSideCount = _voxSideCnt * 2;

    ivec3 cPos = _vi * 2 + ivec3(_childIdx & 1u, (_childIdx >> 1) & 1u, (_childIdx >> 2) & 1u);

    uint pos = cPos.z * actualSideCount * actualSideCount + cPos.y * actualSideCount + cPos.x;

    return pos;
}


// screen space to view space
vec3 screenSpaceToViewSpace(vec3 _uv, mat4 _invPorj)
{
    return (_invPorj * vec4(_uv, 1.f)).xyz;
}

// view space to world space
vec3 viewSpaceToWorldSpace(vec3 _coord, mat4 _invView)
{
    _coord.y = 1.f - _coord.y; // flip y
    _coord.xy = _coord.xy * 2.f - 1.f; // [-1, 1]

    vec4 worldPos = _invView * vec4(_coord, 1.f);
    worldPos.xyz /= worldPos.w;

    return worldPos.xyz;
}

vec2 getRCTexelPos(uint _idxType, uint _raySideCount, uint _probSideCount, ivec2 _probIdx, ivec2 _rayIdx)
{
    ivec2 texelPos = ivec2(0);
    switch (_idxType) {
    case 0:  // probe first index
        texelPos = _probIdx.xy * int(_raySideCount) + _rayIdx;
        break;
    case 1: // ray first index
        texelPos = _rayIdx * int(_probSideCount) + _probIdx.xy;
        break;
    }

    return texelPos;
}

// ==============================================================================
struct RadianceCascadesConfig
{
    uint    probe_sideCount;
    uint    ray_gridSideCount;
    uint    level;
    uint    layerOffset;

    float cx, cy, cz;

    float   rayLength;
    float   probeSideLen;
    float   radius;

    float brx_tmin;
    float brx_tmax;

    uint brx_offset;
    uint brx_startCas;
    uint brx_endCas;
    float brx_sdfEps;

    uint debug_idx_type;
    uint debug_color_type;
};

struct RCMergeData
{
    uint probe_sideCount;
    uint ray_sideCount;
    uint lv;
    uint idxType;
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
    float totalRadius;
    uint cascade_0_probGridCount;
    uint cascade_0_rayGridCount;
    uint debugIdxType;

    uint startCascade;
    uint endCascade;

    float w, h;
    float camx, camy, camz;
};

struct RCAccessData
{
    uint lv;
    uint raySideCount;
    uint probeSideCount;
    uint layerOffset;
    float rayLen;
    float probeSideLen;
};

struct VoxDraw
{
    vec3 pos;
    vec3 col;
};

struct ProbeDebugCmdConsts
{
    float rcRadius;
    float probeSideLen;
    float sphereRadius;
    uint probeSideCount;
    uint layerOffset;
    uint idxCnt;

    float P00, P11;
    float znear, zfar;
    float frustum[4];
    float pyramidWidth, pyramidHeight;

    float posOffsets[3];
};

struct ProbeDraw
{
    vec3 pos;
    ivec3 idx;
};

struct ProbeDebugDrawConsts
{
    float probeDebugRadius;
    float sphereRadius;

    uint probeSideCount;
    uint raySideCount;

    uint debugIdxType;
};

struct RadianceCascadesTransform
{
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
};