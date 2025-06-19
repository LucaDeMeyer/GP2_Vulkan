#include "Scene.h"
#include "VulkanUtils.h"
#include "VulkanIndexBuffer.h"
#include "VulkanVertexBuffer.h"
#include "assimp/cimport.h"
#include "assimp/Importer.hpp"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <filesystem>

//*=============================================================
// MODEL LOADER
//*=============================================================

//what if there is no material list provided? -> overloaded function? or provide default material path input parameter with default value?
void ModelLoader::LoadModel(const std::string& path, std::vector<Mesh*>& meshes,VulkanContext* context)
{

    Assimp::Importer importer;

    // Add pre-read checks and logging
    std::cerr << "ModelLoader: Attempting to load model from: " << path << std::endl;

    if (!std::filesystem::exists(path)) {
        std::cerr << "ModelLoader ERROR: File does not exist at path: " << path << std::endl;
        return;
    }

    const struct aiScene* scene = importer.ReadFile(path.c_str(),
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace | 
        aiProcess_GenNormals |       
        aiProcess_FlipUVs            
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    // Get the base directory of the model file to resolve relative texture paths
    std::string directory = GetDirectoryPath(path);

    
    for (Mesh* m : meshes) {
        delete m; 
    }
    meshes.clear();

    // Iterate through each mesh in the loaded scene
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* assimpMesh = scene->mMeshes[i];

        // Create a NEW Mesh object for each Assimp mesh
        Mesh* newMesh = new Mesh(context); 

        // Populate vertices and indices
        for (unsigned int j = 0; j < assimpMesh->mNumVertices; j++) {
            Vertex vertex{};
            vertex.pos = { assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z };

            if (assimpMesh->HasNormals()) {
                vertex.normal = { assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z };
            }
            else {
                vertex.normal = { 0.0f, 0.0f, 0.0f }; // Default or handle missing normals
            }

            // Load Tangents
            if (assimpMesh->HasTangentsAndBitangents()) {
                vertex.tangent = { assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z };
            }
            else {
                vertex.tangent = { 0.0f, 0.0f, 0.0f }; // Default or handle missing tangents
            }

            if (assimpMesh->HasTextureCoords(0)) {
                vertex.texCoord = { assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y };

            }
            else {
                vertex.texCoord = { 0.0f, 0.0f }; // Default or handle missing UVs
            }

            newMesh->vertices.push_back(vertex);
        }

        for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
            aiFace face = assimpMesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                newMesh->indices.push_back(face.mIndices[k]);
            }
        }

        // --- Load PBR Materials ---
        aiMaterial* material = scene->mMaterials[assimpMesh->mMaterialIndex];

        // 1. Albedo / Base Color Map (aiTextureType_DIFFUSE or aiTextureType_BASE_COLOR for glTF)
        aiString texturePath;
        // Albedo Map
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context); // Create on heap
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::ALBEDO);
            newMesh->SetTexture(TextureType::ALBEDO, std::move(texture)); // Move unique_ptr
        }
        else if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context);
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::ALBEDO);
            newMesh->SetTexture(TextureType::ALBEDO, std::move(texture));
        }
        else {
            // Handle default albedo if no texture
			auto defaultTexture = std::make_unique<VulkanTexture>(context);
            defaultTexture->CreateTexture("Textures/default_albedo.png", TextureType::ALBEDO); // Provide a default texture path
			newMesh->SetTexture(TextureType::ALBEDO, std::move(defaultTexture));
        }
        // Normal Map
        if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context);
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::NORMAL);
            newMesh->SetTexture(TextureType::NORMAL, std::move(texture));
        }

        // Metallic Map
        if (material->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context);
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::METALLIC);
            newMesh->SetTexture(TextureType::METALLIC, std::move(texture));
        }
        // Roughness Map
        if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context);
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::ROUGHNESS);
            newMesh->SetTexture(TextureType::ROUGHNESS, std::move(texture));
        }

        // Ambient Occlusion Map
        if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context);
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::AO);
            newMesh->SetTexture(TextureType::AO, std::move(texture));
        }
        else if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &texturePath) == AI_SUCCESS) {
            auto texture = std::make_unique<VulkanTexture>(context);
            texture->CreateTexture(directory + texturePath.C_Str(),TextureType::AO);
            newMesh->SetTexture(TextureType::AO, std::move(texture));
        }


        // Add the newly created and populated mesh to the output vector
        meshes.push_back(newMesh);
    }

	
   // aiReleaseImport(scene);
    
}

//*=============================================================

//*=============================================================
// MESH
// *=============================================================
Mesh::Mesh(VulkanContext* context) : context(context)
{
	vertexBuffer = new VulkanVertexBuffer(context);
	indexBuffer = new VulkanIndexBuffer(context);
}

void Mesh::Bind(VkCommandBuffer commandBuffer,VkDeviceSize offsets)
{
	VkBuffer vbLocal{ vertexBuffer->GetVertexBuffer() };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbLocal, &offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::CreateBuffers()
{
    vertexBuffer->CreateVertexBuffer(vertices);
    indexBuffer->CreateIndexBuffer(indices);
}

void Mesh::SetTexture(TextureType type, std::unique_ptr<VulkanTexture> texture) {
    if (type >= TextureType::ALBEDO && type <= TextureType::AO) {
        textures[type] = std::move(texture);
    }
    else {
        std::cerr << "Mesh::SetTexture: Invalid texture type provided." << std::endl;
    }
}

void Mesh::CleanUpMesh()
{
    indexBuffer->CleanupIndexBuffer();
	vertexBuffer->CleanupVertexBuffer();

    for (auto& [type, texturePtr] : textures) 
    {
        if (&texturePtr) { 
            texturePtr->CleanupTexture(); 
        }
    }
    textures.clear(); 
}

const VulkanTexture& Mesh::GetTexture(TextureType type) const {
    auto it = textures.find(type);
    if (it != textures.end()) {
        return *(it->second);
    }
    else {
        std::cerr << "Mesh::GetTexture: Texture type not found." << std::endl;
        throw std::runtime_error("Texture type not found");
    }
}

Mesh::~Mesh()
{
	delete vertexBuffer;
	delete indexBuffer;
}
//*=============================================================
