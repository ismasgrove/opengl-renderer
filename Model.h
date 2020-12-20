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
	std::unordered_map<std::string, unsigned int> bone_mapping;
	std::vector<BoneInfo> bones_info;
	size_t n_bones = 0;
	glm::mat4 global_transform_inv;
	const aiScene* scene;
	Assimp::Importer importer;
	std::chrono::steady_clock::time_point start_time;
	bool textured = false;
	bool animated = false;

	void loadModel(std::string path);
	void processNode(aiNode* node);
	Mesh processMesh(aiMesh* mesh);
	Material loadMaterial(aiMaterial* mat);
	void boneTransform(float time_s, std::vector<glm::mat4>& transforms);
	void readNodeHeirarchy(float anim_time, const aiNode* node, const glm::mat4& parent);
	void interpolatedRot(aiQuaternion& out, float anim_time, const aiNodeAnim* node_anim);
	void interpolatedScal(aiVector3D& out, float anim_time, const aiNodeAnim* node_anim);
	void interpolatedPos(aiVector3D& out, float anim_time, const aiNodeAnim* node_anim);
	unsigned int findRot(float anim_time, const aiNodeAnim* node_anim);
	unsigned int findScal(float anim_time, const aiNodeAnim* node_anim);
	unsigned int findPos(float anim_time, const aiNodeAnim* node_anim);
	const aiNodeAnim* findNodeAnim(const aiAnimation* anim, std::string nodeName);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};