#include "collisions.h"
#include <glm/geometric.hpp>
#include <algorithm>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

bool CheckPointSphereCollision(glm::vec3 point, const Sphere &sphere)
{
    float diff_x = point.x - sphere.center.x;
    float diff_y = point.y - sphere.center.y;
    float diff_z = point.z - sphere.center.z;

    float square_distance = (diff_x * diff_x) + (diff_y * diff_y) + (diff_z * diff_z);

    return square_distance <= (sphere.radius * sphere.radius);
}

bool CheckPointAABBCollision(glm::vec3 point, const AABB &box)
{
    return (point.x >= box.min.x && point.x <= box.max.x) &&
           (point.y >= box.min.y && point.y <= box.max.y) &&
           (point.z >= box.min.z && point.z <= box.max.z);
}

bool CheckPointPlaneCollision(glm::vec3 point, const Plane &plane)
{
    return glm::dot(point, plane.normal) - plane.distance <= 0;
}

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
    float closestX = std::max(box.min.x, std::min(sphere.center.x, box.max.x));
    float closestY = std::max(box.min.y, std::min(sphere.center.y, box.max.y));
    float closestZ = std::max(box.min.z, std::min(sphere.center.z, box.max.z));

    float distanceSquared = (closestX - sphere.center.x) * (closestX - sphere.center.x) +
                            (closestY - sphere.center.y) * (closestY - sphere.center.y) +
                            (closestZ - sphere.center.z) * (closestZ - sphere.center.z);

    return distanceSquared < (sphere.radius * sphere.radius);
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