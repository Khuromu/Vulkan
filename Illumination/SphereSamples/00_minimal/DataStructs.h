#pragma once

#include <glm/glm/glm.hpp>

struct Transform
{
	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(0.f, 0.f, 0.f);
};