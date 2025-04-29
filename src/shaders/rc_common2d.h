
struct Rc2dData
{
    uint width;
    uint height;

    uint c0_dRes;
    uint nCascades;

    float mpx;
    float mpy;
};

struct Rc2dMergeData
{
    Rc2dData rc;
    uint lv;
};


struct ProbeSamp
{
    ivec2 baseIdx;
    vec2 ratio;
};

ivec2 getOffsets(uint _idx)
{
    switch (_idx)
    {
    case 0: return ivec2(0, 0);
    case 1: return ivec2(1, 0);
    case 2: return ivec2(0, 1);
    case 3: return ivec2(1, 1);
    }

    return ivec2(0, 0);
}

vec4 getWeights(vec2 _ratio)
{
    vec4 weights;
    weights[0] = (1.0 - _ratio.x) * (1.0 - _ratio.y);
    weights[1] = _ratio.x * (1.0 - _ratio.y);
    weights[2] = (1.0 - _ratio.x) * _ratio.y;
    weights[3] = _ratio.x * _ratio.y;

    return weights;
}

ProbeSamp getProbSamp(ivec2 _pixIdx, uint _dRes)
{
    vec2 area = _pixIdx / float(_dRes);

    ProbeSamp samp;
    samp.baseIdx = ivec2(floor(area));
    samp.ratio = fract(area - 0.5f);

    return samp;
}


vec4 opU(vec4 d1, vec4 d2)
{
    return (d1.w < d2.w) ? d1 : d2;
}

float sdBox(in vec2 p, in vec2 b)
{
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// sdf is in pixel space
vec4 sdf(in vec2 _origin, in vec2 _res)
{
    vec2 center = _res / 2.f;

    vec4 distance;
    distance = opU(distance, vec4(1.0, 0.4, 0.0, sdBox(_origin - vec2(_res.x * .5f, _res.y * .2f ), vec2(40.f, 60.f))));
    distance = opU(distance, vec4(0.01, 0.4, 3.0, sdBox(_origin - _res * .5f, vec2(200.f, 10.f))));
    return distance;
}

// ------------------------------------------------------------------------------------------------
float m_stretch(float point, float stretch) {
    return .5 * (sign(point) * stretch - point) * (sign(abs(point) - stretch) + 1.);
}

#define m_stretch_neg(p, st) (.5 * m_stretch(2. * p + st, st))  /* Stretch-negative: positive values are fixed */
#define m_stretch_pos(p, st) (.5 * m_stretch(2. * p, st))  /* Stretch-positive: negative values are fixed */

// A fast "manual" lossy rotate function by ollj.
// Control the angle of rotation by specifying the ROTATE_PARAM constants.
// https://en.wikipedia.org/wiki/Trigonometric_constants_expressed_in_real_radicals
float ollj_rotate(vec2 uv) {
    const float ROTATE_PARAM0 = sqrt(1.);  // Try changing these!
    const float ROTATE_PARAM1 = sqrt(.0);
    return dot(uv, vec2(ROTATE_PARAM0 + ROTATE_PARAM1, ROTATE_PARAM0 - ROTATE_PARAM1));
}

// NOTE: Very slightly modified - Mytino.
// Optimized arrow SDF, by ollj
// @head_len isn't actual uv units, but the unit size depends on ROTATE_PARAMs
float sdf_arrow(vec2 uv, float norm, vec2 dir, float head_height, float stem_width) {
    uv = vec2(dir.x * uv.x + dir.y * uv.y, -dir.y * uv.x + dir.x * uv.y);

    norm -= head_height;  // Make sure the norm INCLUDES the arrow head
    uv.x -= norm;  // Place the arrow's origin at the stem's base!

    uv.y = abs(uv.y);
    float head = max(ollj_rotate(uv) - head_height, -uv.x);

    uv.x = m_stretch_neg(uv.x, norm);
    uv.y = m_stretch_pos(uv.y, stem_width);
    float stem = length(uv);

    return min(head, stem);  // Join head and stem!
}
