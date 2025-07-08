#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <GLFW/glfw3.h>

#include "collisions.h"

#include <bits/stdc++.h>
using namespace std;

// Curva de Bézier
glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
glm::vec3 CalculateBezierTangent(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

// Atualização dos objs
void UpdatePlayerPosition(GLFWwindow *window, float time_diff, Player &player,
                          const std::vector<CollidableObject> &collidables);

bool RayIntersectsPlane(const glm::vec3 &ray_origin,
                        const glm::vec3 &ray_dir,
                        const Plane &plane,
                        float &t_out);

bool RayIntersectsSphere(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir,
                         const Sphere &sphere, float &t_out);

bool RayIntersectsAABB(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir,
                       const AABB &box, float &t_out);

std::string CheckRaycastFromCenter(const Player &player, std::vector<CollidableObject> &objects, bool &is_player_asleep, bool &rotate_breads);

void UpdateRat(Rat &rat, const Player &player, const std::vector<CollidableObject> &other_collidables, float time_diff);