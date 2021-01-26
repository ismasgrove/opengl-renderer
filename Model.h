#pragma once

#include "Shader.h"
#include "Mesh.h"
#include "stb_image.h"
#include <vector>
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <chrono>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/ext.hpp>

constexpr auto WEIGHTS_PER_VERTEX = 4;

class Model
{
public:
	Model(const char* path);
	~Model();
	void Draw(Shader& shader);
	
private:
	std::vector<Mesh> meshes;
	std::string directory;
	std::vector<Texture> textures_loaded;
	const aiScene* scene;
	Assimp::Importer importer;
	std::chrono::steady_clock::time_point start_time;
	bool textured = false;
	bool animated = false;

	void loadModel(std::string path);
	void processNode(aiNode* node);
	Mesh processMesh(aiMesh* mesh);
	Material loadMaterial(aiMaterial* mat);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};