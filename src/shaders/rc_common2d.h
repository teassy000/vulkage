
struct Rc2dData
{
    uint width;
    uint height;

    uint c0_dRes;
    uint nCascades;
};


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
