#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <GLFW/glfw3.h>

#include "collisions.h"

// Curva de Bézier
glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
glm::vec3 CalculateBezierTangent(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

// Atualização dos objs
void UpdateBunny(float time_diff, float &bezier_t, int &bezier_direction,
                 glm::mat4 &out_bunny_model, AABB &out_bunny_collider);

void UpdateSphere(glm::mat4 &out_sphere_model, Sphere &out_sphere_collider);

void UpdatePlayerPosition(GLFWwindow *window, float time_diff, Player &player,
                          const std::vector<CollidableObject> &collidables);