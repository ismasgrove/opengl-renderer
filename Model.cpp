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
		shader.setBool("animated", true); // doesn't work yet

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
		//
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

	return Mesh(vertices, indices, textures, mesh_material);
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