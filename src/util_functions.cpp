#include "util_functions.h"
#include "matrices.h"
#include "collisions.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <bits/stdc++.h>
using namespace std;

glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * p0;
    p += 3 * uu * t * p1;
    p += 3 * u * tt * p2;
    p += ttt * p3;

    return p;
}

glm::vec3 CalculateBezierTangent(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
    float u = 1.0f - t;
    float uu = u * u;
    float tt = t * t;

    glm::vec3 tangent = -3 * uu * p0;
    tangent += (3 * uu - 6 * u * t) * p1;
    tangent += (6 * u * t - 3 * tt) * p2;
    tangent += 3 * tt * p3;

    if (glm::length(tangent) > 0.0001f)
    {
        return glm::normalize(tangent);
    }
    return glm::vec3(0.0f, 0.0f, 1.0f);
}

void UpdateBunny(float time_diff, float &bezier_t, int &bezier_direction,
                 glm::mat4 &out_bunny_model, AABB &out_bunny_collider)
{
    extern float bezier_speed;
    extern glm::vec3 bezier_p0, bezier_p1, bezier_p2, bezier_p3;
    extern glm::vec3 local_bunny_min, local_bunny_max;

    bezier_t += bezier_direction * bezier_speed * time_diff;
    if (bezier_t > 1.0f)
    {
        bezier_t = 1.0f;
        bezier_direction = -1;
    }
    else if (bezier_t < 0.0f)
    {
        bezier_t = 0.0f;
        bezier_direction = 1;
    }

    glm::vec3 bunny_pos = CalculateBezierPoint(bezier_t, bezier_p0, bezier_p1, bezier_p2, bezier_p3);
    glm::vec3 bunny_dir = CalculateBezierTangent(bezier_t, bezier_p0, bezier_p1, bezier_p2, bezier_p3);
    if (bezier_direction == -1)
    {
        bunny_dir *= -1.0f;
    }

    float angle_y = atan2(bunny_dir.x, bunny_dir.z);
    float correction = glm::radians(90.0f);

    out_bunny_model = Matrix_Translate(bunny_pos.x, bunny_pos.y, bunny_pos.z) * Matrix_Rotate_Y(angle_y) * Matrix_Rotate_Y(correction);

    glm::vec3 corners[8] = {
        glm::vec3(local_bunny_min.x, local_bunny_min.y, local_bunny_min.z),
        glm::vec3(local_bunny_max.x, local_bunny_min.y, local_bunny_min.z),
        glm::vec3(local_bunny_min.x, local_bunny_max.y, local_bunny_min.z),
        glm::vec3(local_bunny_min.x, local_bunny_min.y, local_bunny_max.z),
        glm::vec3(local_bunny_max.x, local_bunny_max.y, local_bunny_min.z),
        glm::vec3(local_bunny_min.x, local_bunny_max.y, local_bunny_max.z),
        glm::vec3(local_bunny_max.x, local_bunny_min.y, local_bunny_max.z),
        glm::vec3(local_bunny_max.x, local_bunny_max.y, local_bunny_max.z)};

    glm::vec3 transformed_corner = out_bunny_model * glm::vec4(corners[0], 1.0f);
    out_bunny_collider.min = transformed_corner;
    out_bunny_collider.max = transformed_corner;

    for (int i = 1; i < 8; ++i)
    {
        transformed_corner = out_bunny_model * glm::vec4(corners[i], 1.0f);

        out_bunny_collider.min = glm::min(out_bunny_collider.min, transformed_corner);
        out_bunny_collider.max = glm::max(out_bunny_collider.max, transformed_corner);
    }
}

void UpdateSphere(glm::mat4 &out_sphere_model, Sphere &out_sphere_collider)
{
    extern float sphere_radius;

    out_sphere_model = Matrix_Translate(-1.0f, 0.0f, 0.0f);
    out_sphere_collider.center = glm::vec3(out_sphere_model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    out_sphere_collider.radius = sphere_radius;
}

void UpdatePlayerPosition(GLFWwindow *window, float time_diff, Player &player,
                          const std::vector<CollidableObject> &collidables)
{
    extern float normal_speed, run_speed;

    float camera_speed = normal_speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        camera_speed = run_speed;
    }

    glm::vec3 horizontal_front = glm::vec3(player.front_vector.x, 0.0f, player.front_vector.z);
    if (glm::length(horizontal_front) > 0.0001f)
    {
        horizontal_front = glm::normalize(horizontal_front);
    }
    glm::vec3 horizontal_sides = glm::normalize(glm::cross(horizontal_front, player.up_vector));

    glm::vec3 movements[] = {horizontal_front, -horizontal_front, -horizontal_sides, horizontal_sides};
    int keys[] = {GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D};

    for (int i = 0; i < 4; ++i)
    {
        if (glfwGetKey(window, keys[i]) == GLFW_PRESS)
        {

            glm::vec3 potential_pos = player.position + movements[i] * camera_speed * time_diff;
            AABB player_collider = {player.local_collider.min + potential_pos, player.local_collider.max + potential_pos};

            bool has_collided = false;
            for (const auto &object : collidables)
            {

                if (object.shape_type == ShapeType::SHAPE_AABB)
                {
                    if (TestAABBvsAABB(player_collider, object.aabb))
                    {
                        has_collided = true;
                        break;
                    }
                }
                else if (object.shape_type == ShapeType::SHAPE_SPHERE)
                {
                    if (TestAABBvsSphere(player_collider, object.sphere))
                    {
                        has_collided = true;
                        break;
                    }
                }
                else if (object.shape_type == ShapeType::SHAPE_PLANE)
                {
                    if (TestAABBvsPlane(player_collider, object.plane))
                    {
                        has_collided = true;
                        break;
                    }
                }
            }

            if (!has_collided)
                player.position = potential_pos;
        }
    }
}

bool RayIntersectsPlane(const glm::vec3 &ray_origin,
                        const glm::vec3 &ray_dir,
                        const Plane &plane,
                        float &t_out)
{
    float denom = glm::dot(plane.normal, ray_dir);

    // Se o denominador for próximo de 0, o raio é paralelo ao plano
    if (fabs(denom) < 1e-6f)
        return false;

    // Ponto qualquer no plano é dado por: P tal que dot(n, P) + d = 0
    // Para encontrar t tal que: dot(n, O + t*D) + d = 0
    t_out = -(glm::dot(plane.normal, ray_origin) + plane.distance) / denom;

    // Se t < 0, interseção está "atrás" da origem do raio
    return t_out >= 0;
}

string CheckRaycastFromCenter(const Player &player, const std::vector<CollidableObject> &objects)
{
    glm::vec3 ray_origin = player.position;
    glm::vec3 ray_dir = glm::normalize(player.front_vector);

    float closest_distance = std::numeric_limits<float>::max();
    const CollidableObject *hit_object = nullptr;

    for (const auto &obj : objects)
    {
        float t;
        if (obj.shape_type == ShapeType::SHAPE_PLANE)
        {
            if (RayIntersectsPlane(ray_origin, ray_dir, obj.plane, t))
            {
                if (t >= 0.0f && t < closest_distance)
                {
                    closest_distance = t;
                    hit_object = &obj;
                }
            }
        }
    }

    if (hit_object)
    {
        cout << hit_object->text << endl;
        return hit_object->text;
    }
    else
    {
        return "";
    }
}
