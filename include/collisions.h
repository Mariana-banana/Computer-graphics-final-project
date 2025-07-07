#pragma once

#include <glm/vec3.hpp>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

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
struct Rat
{
    float t = 0.0f;
    int direction = 1;
    float speed = 0.15f;
    float scale = 1.75f;
    float radius = 1.5f;

    glm::vec3 p0 = glm::vec3(-18.0f, -8.0f, -18.0f);
    glm::vec3 p1 = glm::vec3(18.0f, -8.0f, -18.0f);
    glm::vec3 p2 = glm::vec3(18.0f, -8.0f, 18.0f);
    glm::vec3 p3 = glm::vec3(-18.0f, -8.0f, 18.0f);

    glm::vec3 position;
    glm::mat4 model_matrix;
    Sphere collider;

    std::string text = "Voce se livrou do rato";
    bool is_interactive = true;
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

bool TestSphereVsPlane(const Sphere &sphere, const Plane &plane);