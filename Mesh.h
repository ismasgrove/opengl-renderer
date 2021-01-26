#pragma once

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <optional>
#include "Shader.h"
constexpr auto NUM_BONES_PER_VERTEX = 4;

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};

struct Material
{
	glm::vec3 ambient, diffuse, specular;
	float shininess;
};

class Mesh
{
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	Material material;

	Mesh(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices, std::vector<Texture> &textures, Material& material);
	~Mesh();
	void Draw(Shader& shader, bool textured);
private:
	unsigned int VAO, VBO, EBO;
	void setupMesh();
};


