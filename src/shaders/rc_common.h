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
    float   ot_sceneSideLen;
};

struct VoxelizationConfig
{
    mat4 proj;
    uint edgeLen;
};

// ==============================================================================

#define INVALID_OCT_IDX 0xFFFFFFFFu

struct OctTreeNode
{
    uint voxIdx;
    uint lv;
    uint childs[8]; // if is leaf, it's the index of voxel, else the index in the octTree;
};

struct OctTreeProcessConfig
{
    uint lv;
    uint voxLen;
    uint readOffset;
    uint writeOffset;
};