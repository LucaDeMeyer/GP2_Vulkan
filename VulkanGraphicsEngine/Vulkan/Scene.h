#ifndef SCENE_H
#define SCENE_H
#include <string>
#include <vector>

#include "VulkanTexture.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "VulkanUtils.h"
#include "VulkanContext.h"
#include <map>
class VulkanVertexBuffer;
class VulkanIndexBuffer;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 tangent;

	static std::vector<VkVertexInputBindingDescription> GetBindingDescription()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescription(1);
		bindingDescription[0].binding = 0;
		bindingDescription[0].stride = sizeof(Vertex);
		bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({
			/*location*/0,
			/*binding*/0,
			/*format*/VK_FORMAT_R32G32B32_SFLOAT,
			/*offset*/offsetof(Vertex,pos) });

		attributeDescriptions.push_back({
			/*location*/1,
			/*binding*/0,
			/*format*/VK_FORMAT_R32G32B32_SFLOAT,
			/*offset*/offsetof(Vertex,color) });

		attributeDescriptions.push_back({
			/*location*/2,
			/*binding*/0,
			/*format*/VK_FORMAT_R32G32_SFLOAT,
			/*offset*/offsetof(Vertex,texCoord) });

		attributeDescriptions.push_back({
			/*location*/3,
			/*binding*/0,
			/*format*/VK_FORMAT_R32G32B32_SFLOAT,
			/*offset*/offsetof(Vertex,normal) });
            

		attributeDescriptions.push_back({
			/*location*/4,
			/*binding*/0,
			/*format*/VK_FORMAT_R32G32B32_SFLOAT,
			/*offset*/offsetof(Vertex,tangent) });


		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1)
				^ (hash<glm::vec2>()(vertex.texCoord) << 1)
				^ (hash<glm::vec3>()(vertex.normal) << 1); 
		}
	};
};



 inline  std::string GetDirectoryPath(const std::string& filePath) {
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash == std::string::npos) {
            return ""; 
        }
        return filePath.substr(0, lastSlash + 1);
    }


struct Mesh
{

   

	Mesh(VulkanContext* context);
	~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept
        : vertices(std::move(other.vertices)), indices(std::move(other.indices)),
        textures(std::move(other.textures)), context(other.context),
        vertexBuffer(other.vertexBuffer), indexBuffer(other.indexBuffer)
    {
        other.vertexBuffer = nullptr;
        other.indexBuffer = nullptr;
    }
    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            if (vertexBuffer) { /* delete vertexBuffer; */ }
            if (indexBuffer) { /* delete indexBuffer; */ }
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            textures = std::move(other.textures);
            context = other.context;
            vertexBuffer = other.vertexBuffer;
            indexBuffer = other.indexBuffer;
            other.vertices.clear();
            other.indices.clear();
            other.vertexBuffer = nullptr;
            other.indexBuffer = nullptr;
        }
        return *this;
    }

	std::vector<Vertex> vertices;				//mesh data
	std::vector<uint32_t> indices;				//mesh data
    std::map<TextureType, std::unique_ptr<VulkanTexture>> textures;		//mesh data

	VulkanContext* context;
	VulkanVertexBuffer* vertexBuffer;			// mesh buffers
	VulkanIndexBuffer* indexBuffer;				// mesh buffers

	void CreateBuffers();
    void SetTexture(TextureType type, std::unique_ptr<VulkanTexture> texture);
    const VulkanTexture& GetTexture(TextureType type) const;
    const VulkanTexture& GetDefaultTexture(TextureType type) const;
	void Bind(VkCommandBuffer commandBuffer,VkDeviceSize offsets);
	void CleanUpMesh();
};
class ModelLoader {
public:
    static ModelLoader& GetInstance() {
        static ModelLoader instance; 
        return instance;
    }

    void LoadModel(const std::string& path, std::vector<Mesh*>& meshes, VulkanContext* context);
	void LoadModel(const std::string& modelPath, std::vector<Mesh*>& meshes, const std::string& materialPaths, VulkanContext* context);
private:
    ModelLoader() = default;
    ~ModelLoader() = default;
	//ModelLoader(const ModelLoader&) = delete;
	//ModelLoader& operator=(const ModelLoader&) = delete;
};

namespace Scene 
{
    struct Camera
    {
        Camera() = default;
        Camera(const glm::vec3 origin, float fovAngle) : origin(origin), fovAngle(fovAngle) {}
        ~Camera() = default;
        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        glm::mat4 projectionMatrix{ 1.0f };
        glm::mat4 viewMatrix{ 1.0f };
        glm::mat4 inverseViewMatrix{ 1.0f };

        glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        glm::vec3 forward{ 0.0f, 0.0f, -1.0f };
        glm::vec3 right{ 1.0f, 0.0f, 0.0f };
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        float fovAngle{ 45.0f };
        float fov{}; 
        float aspectRatio{ 16.0f / 9.0f };
        float nearplane{ 0.1f };
        float farplane{ 500.0f };
        float speed{ .1f };
        float sensitivity{ 0.1f };

        float yaw{ -90.0f };  
        float pitch{ 0.0f };

        bool firstMouse{ true };
        double lastX{ 0.0 };
        double lastY{ 0.0 };

        void InitCamera(float ar, float _fovAngle, glm::vec3 _origin = { 0.0f, 0.0f, 0.0f })
        {
            fovAngle = _fovAngle;
            fov = tanf(glm::radians(fovAngle) / 2.0f);
            aspectRatio = ar;
            origin = _origin;
            position = _origin; 
            updateCameraVectors();
        }

        void CalcViewMatrix()
        {
            glm::vec3 target = origin + forward;
            viewMatrix = glm::lookAt(origin, target, up);
            inverseViewMatrix = glm::inverse(viewMatrix); 
        }

        void calculateProjectionMatrix()
        {
            projectionMatrix = glm::perspective(glm::radians(fovAngle), aspectRatio, nearplane, farplane);
            projectionMatrix[1][1] *= -1; // Flip the Y-axis for Vulkan
        }

        glm::mat4 getProjection() const { return projectionMatrix; }
        glm::mat4 getView() const { return viewMatrix; }
        glm::vec3 getPosition() const { return position; }
        glm::vec3 getForward() const { return forward; }
        glm::vec3 getRight() const { return right; }
        glm::vec3 getUp() const { return up; }

        void ProcessKeyboard(GLFWwindow* window, float deltaTime)
        {
            float velocity = speed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                origin += forward * velocity;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                origin -= forward * velocity;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                origin -= right * velocity;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                origin += right * velocity;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                origin += up * velocity;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                origin -= up * velocity;
            position = origin; 
        }

        void ProcessMouseMovement(GLFWwindow* window, double xposIn, double yposIn, bool rightButtonDown, bool constrainPitch = true)
        {
            if (rightButtonDown)
            {
                float xpos = static_cast<float>(xposIn);
                float ypos = static_cast<float>(yposIn);

                if (firstMouse)
                {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                float xoffset = static_cast<float>(xpos - lastX);
                float yoffset = static_cast<float>(lastY - ypos);

                lastX = xpos;
                lastY = ypos;

                xoffset *= sensitivity;
                yoffset *= sensitivity;

                yaw += xoffset;
                pitch += yoffset;

                if (constrainPitch)
                {
                    pitch = glm::clamp(pitch, -89.0f, 89.0f);
                }

                updateCameraVectors();
            }
            else
            {
                firstMouse = true; // Reset firstMouse when the button is not down
            }
        }

        void ProcessMouseScroll(double yoffset)
        {
            fovAngle -= static_cast<float>(yoffset);
            fovAngle = glm::clamp(fovAngle, 1.0f, 45.0f);
            fov = tanf(glm::radians(fovAngle) / 2.0f); 
            calculateProjectionMatrix(); 
        }

        static bool IsOutsideFrustum(const glm::vec3& point, const glm::vec3& frustumCenter, float frustumRadius)
        {
            return glm::distance(point, frustumCenter) > frustumRadius;
        }

    private:
        void updateCameraVectors()
        {
            // Calculate the new Front vector
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            forward = glm::normalize(front);
            // Also re-calculate the Right and Up vector
            right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))); 
            up = glm::normalize(glm::cross(right, forward));
        }
    };





}
#endif