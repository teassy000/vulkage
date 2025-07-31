#ifndef SMAA_THRESHOLD
#define SMAA_THRESHOLD 0.1
#endif

#ifndef SMAA_DEPTH_THRESHOLD
#define SMAA_DEPTH_THRESHOLD (0.1 * SMAA_THRESHOLD)
#endif

#ifndef SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0
#endif




// 6 neighbors to check
vec2 offsets[6] = vec2[6](
    vec2(-1.0, 0.0)
    , vec2(0.0, -1.0)
    , vec2(1.0, 0.0)
    , vec2(0.0, 1.0)
    , vec2(-2.0, 0.0)
    , vec2(0.0, -2.0)
    );

vec2 pos2uv(vec2 _pos, vec2 _imageSize)
{
    return (_pos + 0.5) / _imageSize;
}
