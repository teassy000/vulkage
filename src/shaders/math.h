#define PI 3.14159265359
#define GOLD vec3(1.0, 0.765557, 0.336057)

#define sectorize(value) step(0.0, (value)) * 2.0 - 1.0
#define sum(value) dot(clamp((value), 1.0, 1.0), (value))
#define EPSILON 0.00001f
#define FLT_MAX 3.402823466e+38

bool compare(float a, float b)
{
    return abs(a - b) < EPSILON;
}

bool compare(vec2 _a, vec2 _b)
{
    return compare(_a.x, _b.x) && compare(_a.y, _b.y);
}

bool compare(vec3 _a, vec3 _b)
{
    return compare(_a.x, _b.x) && compare(_a.y, _b.y) && compare(_a.z, _b.z);
}

bool compare(vec4 _a, vec4 _b)
{
    return compare(_a.x, _b.x) && compare(_a.y, _b.y) && compare(_a.z, _b.z) && compare(_a.w, _b.w);
}


// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool projectSphere(vec3 c, float r, float znear, float P00, float P11, out vec4 aabb)
{
    if (c.z < r + znear) // (znear > c.z - r)
        return false;

    vec3 cr = c * r;
    float czr2 = c.z * c.z - r * r;

    float vx = sqrt(c.x * c.x + czr2);
    float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
    float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

    float vy = sqrt(c.y * c.y + czr2);
    float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
    float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

    aabb = vec4(minx * P00, miny * P11, maxx * P00, maxy * P11);
    aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

    return true;
}

bool coneCull(vec3 center, float radius, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
    return dot(center - camera_position, cone_axis) >= (cone_cutoff * length(center - camera_position) + radius);
}

vec3 rotateQuat(vec3 v, vec4 q)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

float boundsError(LodBounds bounds, vec3 cam_pos, float cam_proj_1_1, float cam_znear)
{
    float dx = bounds.center.x - cam_pos.x;
    float dy = bounds.center.y - cam_pos.y;
    float dz = bounds.center.z - cam_pos.z;
    float d = sqrt(dx * dx + dy * dy + dz * dz) - bounds.radius;

    float limd = max(d, cam_znear);
    return bounds.error / (limd * cam_proj_1_1 * 0.5);
}

float maxElem(vec3 _v)
{
    return max(max(_v.x, _v.y), _v.z);
}