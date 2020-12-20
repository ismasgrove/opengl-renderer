#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;

out vec2 TexCoords;
out vec4 Normal;
out vec3 FragPos;

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 bones[MAX_BONES];
uniform bool animated;

void main()
{

    mat4 skin = bones[BoneIDs[0]] * Weights[0];
    skin += bones[BoneIDs[1]] * Weights[1];
    skin += bones[BoneIDs[2]] * Weights[2];
    skin += bones[BoneIDs[3]] * Weights[3];
    vec4 bone_pos;
    if (animated)
    bone_pos = skin * vec4(aPos, 1.0f);
    else 
    bone_pos = vec4(aPos, 1.0f);
    
    FragPos = vec3(model * bone_pos);
    TexCoords = aTexCoords;
    vec3 n_matrix = mat3(transpose(inverse(model))) * aNormal;
    Normal = normalize(vec4(n_matrix, 0.0));

    gl_Position = (projection * view * model) * bone_pos;
}