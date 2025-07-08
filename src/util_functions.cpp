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
    float t2 = t * t;
    float u2 = u * u;
    float u3 = u2 * u;
    float t3 = t2 * t;

    glm::vec3 p = u3 * p0;
    p += 3 * u2 * t * p1;
    p += 3 * u * t2 * p2;
    p += t3 * p3;

    return p;
}

glm::vec3 CalculateBezierTangent(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
    float u = 1.0f - t;
    float u2 = u * u;
    float t2 = t * t;

    glm::vec3 tangent = -3 * u2 * p0;
    tangent += (3 * u2 - 6 * u * t) * p1;
    tangent += (6 * u * t - 3 * t2) * p2;
    tangent += 3 * t2 * p3;

    if (glm::length(tangent) > 0.0001f)
    {
        return glm::normalize(tangent);
    }
    // Se a tg tender a 0, retornamos um vetor padrão
    return glm::vec3(0.0f, 0.0f, 1.0f);
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

    // Raio paralelo ao plano
    if (fabs(denom) < 1e-6f)
        return false;

    // fórmula de um ponto no plano: P tal que dot(n, P) + d = 0
    // precisamos encontrar um t que: dot(n, O + t*D) + d = 0
    t_out = -(glm::dot(plane.normal, ray_origin) + plane.distance) / denom;

    // Se t < 0 significa que a interseção está "atrás" da origem do raio
    return t_out >= 0;
}

bool RayIntersectsSphere(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir,
                         const Sphere &sphere, float &t_out)
{
    glm::vec3 oc = ray_origin - sphere.center;
    float a = glm::dot(ray_dir, ray_dir);
    float b = 2.0f * glm::dot(oc, ray_dir);
    float c = glm::dot(oc, oc) - sphere.radius * sphere.radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
    {
        return false;
    }

    //  A intersecção mais próxima, com t > 0
    float t = (-b - sqrt(discriminant)) / (2.0f * a);

    if (t < 0) // Se ela está atrás do raio, tenta a segunda
    {
        t = (-b + sqrt(discriminant)) / (2.0f * a);
    }

    // Se ainda estiver atrás, não há intersecção
    if (t < 0)
    {
        return false;
    }

    t_out = t;
    return true;
}

bool RayIntersectsAABB(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir,
                       const AABB &box, float &t_out)
{
    // Inversa para transformar div em mult
    glm::vec3 invDir = 1.0f / ray_dir;

    // Calcula os tempos de intersecção com os planos da caixa em cada eixo
    glm::vec3 t1 = (box.min - ray_origin) * invDir;
    glm::vec3 t2 = (box.max - ray_origin) * invDir;

    glm::vec3 tmin_vec = glm::min(t1, t2);
    glm::vec3 tmax_vec = glm::max(t1, t2);

    // último tempo de saída e primeiro de entrada
    float t_near = max(max(tmin_vec.x, tmin_vec.y), tmin_vec.z);
    float t_far = min(min(tmax_vec.x, tmax_vec.y), tmax_vec.z);

    // Condicoes para falha
    if (t_near > t_far || t_far < 0)
    {
        return false;
    }

    t_out = t_near;
    return true;
};

std::string CheckRaycastFromCenter(const Player &player, std::vector<CollidableObject> &objects, bool &is_player_asleep, bool &rotate_breads)
{
    float closest_distance = std::numeric_limits<float>::max();
    int closest_index = -1;

    for (size_t i = 0; i < objects.size(); ++i)
    {
        float t = 0.0f;
        bool hit = false;

        if (objects[i].shape_type == ShapeType::SHAPE_PLANE)
        {
            hit = RayIntersectsPlane(player.position, player.front_vector, objects[i].plane, t);
        }
        else if (objects[i].shape_type == ShapeType::SHAPE_AABB)
        {
            hit = RayIntersectsAABB(player.position, player.front_vector, objects[i].aabb, t);
        }
        else if (objects[i].shape_type == ShapeType::SHAPE_SPHERE)
        {
            hit = RayIntersectsSphere(player.position, player.front_vector, objects[i].sphere, t);
        }

        if (hit && t < closest_distance)
        {
            closest_distance = t;
            closest_index = static_cast<int>(i);
        }
    }

    if (closest_index != -1 && objects[closest_index].is_interactive)
    {
        CollidableObject &obj = objects[closest_index];
        obj.is_interactive = false;

        if (obj.text == "cama")
        {
            is_player_asleep = true;
            obj.is_interactive = false;

            // Se ainda tem objetos que deveriam ter sido desativados
            for (size_t i = 0; i < objects.size(); ++i)
            {
                if (i != closest_index && objects[i].is_interactive && objects[i].text != "cama" && objects[i].text != "mesa")
                {
                    return "Voce morreu!";
                }
            }

            // senao, significa que terminamos o jogo
            return "Voce dorme tranquilamente!";
        }

        else if (obj.text == "mesa")
        {
            obj.is_interactive = true; // Queremos sempre poder interagir com a mesa
            rotate_breads = true;
        }

        return obj.text;
    }

    return "";
}

void UpdateRat(Rat &rat, const Player &player, const std::vector<CollidableObject> &other_collidables, float time_diff)
{
    float next_t = rat.t + rat.direction * rat.speed * time_diff;
    glm::vec3 potential_pos = CalculateBezierPoint(next_t, rat.p0, rat.p1, rat.p2, rat.p3);

    Sphere potential_collider = {potential_pos, rat.radius};

    bool has_collided = false;
    AABB player_collider = {player.local_collider.min + player.position, player.local_collider.max + player.position};

    if (TestAABBvsSphere(player_collider, potential_collider))
    {
        has_collided = true;
    }

    if (!has_collided)
    {
        for (const auto &obj : other_collidables)
        {
            // Como não há outros objetos esféricos, não há necessidade de teste de colisão Sphere vs Sphere
            if (obj.shape_type == ShapeType::SHAPE_PLANE && TestSphereVsPlane(potential_collider, obj.plane))
            {
                has_collided = true;
                break;
            }
            if (obj.shape_type == ShapeType::SHAPE_AABB && TestAABBvsSphere(obj.aabb, potential_collider))
            {
                has_collided = true;
                break;
            }
        }
    }

    if (has_collided)
    {
        rat.direction *= -1;
    }

    rat.t += rat.direction * rat.speed * time_diff;

    // Se o rato atingiu o comprimento max, inverte a direção para voltar a origem
    if (rat.t > 1.0f)
    {
        rat.t = 1.0f;
        rat.direction = -1;
    }
    if (rat.t < 0.0f)
    {
        rat.t = 0.0f;
        rat.direction = 1;
    }

    rat.position = CalculateBezierPoint(rat.t, rat.p0, rat.p1, rat.p2, rat.p3);
    glm::vec3 rat_dir = CalculateBezierTangent(rat.t, rat.p0, rat.p1, rat.p2, rat.p3);
    if (rat.direction == -1)
    {
        rat_dir *= -1.0f;
    }
    float angle_y = atan2(rat_dir.x, rat_dir.z);

    rat.model_matrix = Matrix_Translate(rat.position.x, rat.position.y, rat.position.z) * Matrix_Rotate_Y(angle_y) * Matrix_Scale(rat.scale, rat.scale, rat.scale);

    rat.collider.center = rat.position;
    rat.collider.radius = rat.radius;
}