#define STB_IMAGE_IMPLEMENTATION
#include "Model.h"

unsigned int loadTextureFromFile(const char* path, const std::string& directory);
unsigned int loadEmbeddedTexture(const aiTexture* data);
inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from);
inline glm::mat4 aiMatrix3x3ToGlm(const aiMatrix3x3* from);

Model::Model(const char* path)
{
	start_time = std::chrono::steady_clock::now();
	loadModel(path);
}

Model::~Model(){
}

void Model::Draw(Shader& shader)
{
	if (animated)
	{
		shader.setBool("animated", true); // doesn't work yet
		std::vector<glm::mat4> transforms;
		auto runningTime = std::chrono::steady_clock::now() - start_time;
		auto time = std::chrono::duration<double, std::milli>(runningTime).count() / 1000;
		boneTransform((float)time, transforms);

		for (size_t i = 0; i < transforms.size(); i++)
		{
			std::string transform_entry = "bones[" + std::to_string(i) + ']';
			//std::cout << glm::to_string(transforms[i]) << std::endl;
			transforms[i] = glm::mat4(1.0f);
			glUniformMatrix4fv(glGetUniformLocation(shader.ID, transform_entry.c_str()), 1, GL_TRUE, (float*)glm::value_ptr(transforms[i]));
		}
	}
	else {
		shader.setBool("animated", false);
	}
	for (auto& m : meshes)
		m.Draw(shader, textured);
}

void Model::loadModel(std::string path)
{
	scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));

	if (scene->HasAnimations()) {
		animated = true;
		global_transform_inv = aiMatrix4x4ToGlm(&scene->mRootNode->mTransformation);
		global_transform_inv = glm::inverse(global_transform_inv);
	}

	processNode(scene->mRootNode);
}

void Model::processNode(aiNode* node)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh));
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i]);
	}
}
Mesh Model::processMesh(aiMesh* mesh)
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	std::vector<VertexBoneData> bones;

	Material mesh_material;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;

		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	if (scene->HasAnimations()) {
		bones.resize(mesh->mNumVertices);
		for (size_t i = 0; i < mesh->mNumBones; i++)
		{
			unsigned int boneIndex = 0;
			std::string boneName(mesh->mBones[i]->mName.data);

			if (bone_mapping.find(boneName) == bone_mapping.end())
			{
				boneIndex = n_bones++;
				BoneInfo bi;
				bones_info.push_back(bi);
			}
			else {
				boneIndex = bone_mapping[boneName];
			}

			bone_mapping[boneName] = boneIndex;
			bones_info[boneIndex].boneOffset = aiMatrix4x4ToGlm(&mesh->mBones[i]->mOffsetMatrix);

			for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; j++)
			{
				unsigned int vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
				float weight = mesh->mBones[i]->mWeights[j].mWeight;
				bones[vertexID].addBoneData(boneIndex, weight);
			}
		}
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		auto face = std::make_unique<aiFace>(mesh->mFaces[i]);
		for (unsigned int j = 0; j < face->mNumIndices; j++)
			indices.push_back(face->mIndices[j]);
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		mesh_material = loadMaterial(material);
		std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return Mesh(vertices, indices, textures, bones, mesh_material);
}

void Model::boneTransform(float time_s, std::vector<glm::mat4>& transforms)
{
	glm::mat4 identity = glm::mat4(1.0f);

	float ticks_per_second = scene->mAnimations[0]->mTicksPerSecond != 0 ?
		scene->mAnimations[0]->mTicksPerSecond : 25.0f;

	float time_ticks = time_s * ticks_per_second;

	float anim_time = fmod(time_ticks, scene->mAnimations[0]->mDuration);

	readNodeHeirarchy(anim_time, scene->mRootNode, identity);

	transforms.resize(n_bones);
	for (size_t i = 0; i < n_bones; i++)
	{
		transforms[i] = bones_info[i].finalTransformation;
	}

}
void Model::readNodeHeirarchy(float anim_time, const aiNode* node, const glm::mat4& parent_trans)
{
	std::string nodeName(node->mName.data);
	const aiAnimation* animation = scene->mAnimations[0];

	glm::mat4 node_transform(aiMatrix4x4ToGlm(&node->mTransformation));

	const aiNodeAnim* node_anim = findNodeAnim(animation, nodeName);

	if (node_anim)
	{
		aiVector3D scaling;
		interpolatedScal(scaling, anim_time, node_anim);
		glm::mat4 scaling_mat(1.0f);
		scaling_mat = glm::scale(scaling_mat, glm::vec3((float)scaling.x, (float)scaling.y, (float)scaling.z));

		aiQuaternion rot_q;
		interpolatedRot(rot_q, anim_time, node_anim);
		aiMatrix3x3 quat_mat = rot_q.GetMatrix();
		glm::mat4 rot_mat = aiMatrix3x3ToGlm(&quat_mat);

		aiVector3D translation;
		interpolatedPos(translation, anim_time, node_anim);
		glm::mat4 transl_mat(1.0f);
		transl_mat = glm::translate(transl_mat, glm::vec3((float)translation.x, (float)translation.y, (float)translation.z));

		node_transform = transl_mat * rot_mat * scaling_mat;
	}

	glm::mat4 global_transform = parent_trans * node_transform;

	if (bone_mapping.find(nodeName) != bone_mapping.end())
	{
		unsigned int boneIndex = bone_mapping[nodeName];
		bones_info[boneIndex].finalTransformation = global_transform_inv * global_transform * bones_info[boneIndex].boneOffset;
	}

	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		readNodeHeirarchy(anim_time, node->mChildren[i], global_transform);
	}

}
void Model::interpolatedRot(aiQuaternion& out, float anim_time, const aiNodeAnim* node_anim)
{
	if (node_anim->mNumRotationKeys == 1)
	{
		out = node_anim->mRotationKeys[0].mValue;
		return;
	}

	unsigned int rotIndex = findRot(anim_time, node_anim);
	unsigned int nextRotIndex = (rotIndex + 1);
	assert(nextRotIndex < node_anim->mNumRotationKeys);
	float deltaTime = node_anim->mRotationKeys[nextRotIndex].mTime - node_anim->mRotationKeys[rotIndex].mTime;
	float factor = (anim_time - (float)node_anim->mRotationKeys[rotIndex].mTime) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	const aiQuaternion& startQRot = node_anim->mRotationKeys[rotIndex].mValue;
	const aiQuaternion& endQRot = node_anim->mRotationKeys[nextRotIndex].mValue;
	aiQuaternion::Interpolate(out, startQRot, endQRot, factor);
	out = out.Normalize();
}
void Model::interpolatedScal(aiVector3D& out, float anim_time, const aiNodeAnim* node_anim)
{
	if (node_anim->mNumScalingKeys == 1)
	{
		out = node_anim->mScalingKeys[0].mValue;
		return;
	}

	unsigned int scalIndex = findScal(anim_time, node_anim);
	unsigned int nextScalIndex = (scalIndex + 1);
	assert(nextScalIndex < node_anim->mNumScalingKeys);
	float deltaTime = node_anim->mScalingKeys[nextScalIndex].mTime - node_anim->mScalingKeys[scalIndex].mTime;
	float factor = (anim_time - (float)node_anim->mScalingKeys[scalIndex].mTime) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);

	const aiVector3D& startQScal = node_anim->mScalingKeys[scalIndex].mValue;
	const aiVector3D& endQScal = node_anim->mScalingKeys[nextScalIndex].mValue;
	aiVector3D delta = endQScal - startQScal;
	out = startQScal + factor * delta;
}
void Model::interpolatedPos(aiVector3D& out, float anim_time, const aiNodeAnim* node_anim)
{
	if (node_anim->mNumPositionKeys == 1)
	{
		out = node_anim->mPositionKeys[0].mValue;
		return;
	}

	unsigned int posIndex = findPos(anim_time, node_anim);
	unsigned int nextPosIndex = (posIndex + 1);
	assert(nextPosIndex < node_anim->mNumPositionKeys);
	float deltaTime = node_anim->mPositionKeys[nextPosIndex].mTime - node_anim->mPositionKeys[posIndex].mTime;
	float factor = (anim_time - (float)node_anim->mPositionKeys[posIndex].mTime) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);

	const aiVector3D& startQPos = node_anim->mPositionKeys[posIndex].mValue;
	const aiVector3D& endQPos = node_anim->mPositionKeys[nextPosIndex].mValue;
	aiVector3D delta = endQPos - startQPos;
	out = startQPos + factor * delta;
}
unsigned int Model::findRot(float anim_time, const aiNodeAnim* node_anim)
{
	assert(node_anim->mNumRotationKeys > 0);
	for (size_t i = 0; i < node_anim->mNumRotationKeys - 1; i++)
	{
		if (anim_time < (float)node_anim->mRotationKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);
}
unsigned int Model::findScal(float anim_time, const aiNodeAnim* node_anim)
{
	assert(node_anim->mNumScalingKeys > 0);

	for (size_t i = 0; i < node_anim->mNumScalingKeys - 1; i++)
	{
		if (anim_time < (float)node_anim->mScalingKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);
}
unsigned int Model::findPos(float anim_time, const aiNodeAnim* node_anim)
{
	assert(node_anim->mNumPositionKeys > 0);

	for (size_t i = 0; i < node_anim->mNumPositionKeys - 1; i++)
	{
		if (anim_time < (float)node_anim->mPositionKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);
}

const aiNodeAnim* Model::findNodeAnim(const aiAnimation* anim, std::string nodeName)
{
	for (size_t i = 0; i < anim->mNumChannels; i++)
	{
		const aiNodeAnim* node_anim = anim->mChannels[i];
		if (std::string(node_anim->mNodeName.data) == nodeName)
		{
			return node_anim;
		}
	}

	return nullptr;
}

Material Model::loadMaterial(aiMaterial* mat)
{
	Material material;
	aiColor3D color(0.f, 0.f, 0.f);
	mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
	material.ambient = glm::vec3(color.r, color.g, color.b);
	mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
	material.diffuse = glm::vec3(color.r, color.g, color.b);
	mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
	material.specular = glm::vec3(color.r, color.g, color.b);
	float shininess;
	mat->Get(AI_MATKEY_SHININESS, shininess);
	material.shininess = shininess;
	
	return material;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString texture_path;
		mat->GetTexture(type, i, &texture_path);
		auto embedded = scene->GetEmbeddedTexture(texture_path.C_Str());
		if (embedded) {
			texture_path.Clear();
		}
		if (std::find_if(textures_loaded.begin(), textures_loaded.end(), [&](Texture const& t) {
			{return t.path == texture_path.C_Str(); }
			}) != textures_loaded.end()
			)
		{
			continue;
		}
		if (texture_path.length == 0
			|| std::filesystem::exists(texture_path.C_Str())
			|| std::filesystem::exists(directory + '/' + texture_path.C_Str())
			)
		{
		Texture texture;
		if (texture_path.length == 0)
		{
			texture.id = loadEmbeddedTexture(embedded);
			texture.path = "E";
		}
		else {
			texture.id = loadTextureFromFile(texture_path.C_Str(), this->directory);
			texture.path = texture_path.C_Str();
		}
		texture.type = typeName;
		textures.push_back(texture);
		textures_loaded.push_back(texture);
		}
		else {
			std::cout << "Texture \"" << texture_path.C_Str() << "\" of type " 
				<< typeName <<" index " << i << " is unavailable." << std::endl;
		}	
	}

	if (!textures.empty())
		textured = true;

	return textures;
}

unsigned int loadTextureFromFile(const char* path, const std::string& directory)
{
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "stbi_image loading failed. cannot find texture associated with file." << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

unsigned int loadEmbeddedTexture(const aiTexture* emb_texture)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	if (emb_texture->mHeight == 0)
	{
		GLenum format = GL_RGB;

		int width, height, nrComponents;
		unsigned char* image = stbi_load_from_memory((unsigned char*)emb_texture->pcData, emb_texture->mWidth, &width, &height, &nrComponents, 0);

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
	}
	else {
		std::cout << "not yet." << std::endl;
	}

	return textureID;
}

inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
	glm::mat4 to;

	to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
	to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
	to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
	to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

	return to;
}

inline glm::mat4 aiMatrix3x3ToGlm(const aiMatrix3x3* from)
{
	glm::mat4 to;

	to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1;
	to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2;
	to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3;

	return to;
}