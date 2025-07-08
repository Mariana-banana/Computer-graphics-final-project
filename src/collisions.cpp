#include "collisions.h"
#include <glm/geometric.hpp>
#include <algorithm>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

bool TestAABBvsAABB(const AABB &a, const AABB &b)
{
    if (a.max.x < b.min.x || a.min.x > b.max.x)
        return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y)
        return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z)
        return false;

    return true;
}

bool TestAABBvsSphere(const AABB &box, const Sphere &sphere)
{
    float closest_x = std::max(box.min.x, std::min(sphere.center.x, box.max.x));
    float closest_y = std::max(box.min.y, std::min(sphere.center.y, box.max.y));
    float closest_z = std::max(box.min.z, std::min(sphere.center.z, box.max.z));

    float sqr_distance = (closest_x - sphere.center.x) * (closest_x - sphere.center.x) +
                         (closest_y - sphere.center.y) * (closest_y - sphere.center.y) +
                         (closest_z - sphere.center.z) * (closest_z - sphere.center.z);

    return sqr_distance < (sphere.radius * sphere.radius);
}

bool TestAABBvsPlane(const AABB &box, const Plane &plane)
{
    glm::vec3 center = (box.max + box.min) * 0.5f;
    glm::vec3 extents = box.max - center;

    float r = extents.x * std::abs(plane.normal.x) +
              extents.y * std::abs(plane.normal.y) +
              extents.z * std::abs(plane.normal.z);

    float s = glm::dot(plane.normal, center) - plane.distance;

    return std::abs(s) <= r;
}

bool TestSphereVsPlane(const Sphere &sphere, const Plane &plane)
{
    float s = glm::dot(plane.normal, sphere.center) - plane.distance;

    return std::abs(s) <= sphere.radius;
}