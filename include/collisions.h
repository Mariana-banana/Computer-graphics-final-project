#pragma once

#include <glm/vec3.hpp>
#include <string>

enum class ShapeType
{
    SHAPE_AABB,
    SHAPE_SPHERE,
    SHAPE_PLANE
};

struct Sphere
{
    glm::vec3 center;
    float radius;
};

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

struct Plane
{
    glm::vec3 normal;
    float distance;
};

struct Player
{
    glm::vec3 position;
    glm::vec3 front_vector;
    glm::vec3 up_vector;
    AABB local_collider;
};

struct CollidableObject
{
    ShapeType shape_type;
    AABB aabb;
    Sphere sphere;
    Plane plane;
    std::string text;
    bool is_interactive;
    std::string interactive_text;
};

bool CheckPointSphereCollision(glm::vec3 point, const Sphere &sphere);

bool CheckPointAABBCollision(glm::vec3 point, const AABB &box);

bool CheckPointPlaneCollision(glm::vec3 point, const Plane &plane);

bool TestAABBvsAABB(const AABB &a, const AABB &b);

bool TestAABBvsSphere(const AABB &box, const Sphere &sphere);

bool TestAABBvsPlane(const AABB &box, const Plane &plane);
