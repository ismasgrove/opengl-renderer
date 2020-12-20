#pragma once

#include <glm/glm.hpp>
#include <glm/matrix.hpp>

struct Spotlight {
	glm::vec3 position, direction, ambient, diffuse, specular, color;
	float innerCutoff, outerCutoff, constant, linear, quadratic;
};

struct Pointlight {
	glm::vec3 position, ambient, diffuse, specular, color;
	float constant, linear, quadratic;
};

struct Dirlight {
	glm::vec3 direction, ambient, diffuse, specular, color;
};