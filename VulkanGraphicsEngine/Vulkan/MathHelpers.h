#ifndef MATHHELPERS_H
#define MATHHELPERS_H
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace MathHelpers
{

    inline static glm::mat4 CreateLookatLH(const glm::vec3& origin, const glm::vec3& fwrd, const glm::vec3& up)
    {
        // 1. Normalize the forward vector.
        glm::vec3 zaxis = glm::normalize(fwrd);
        // 2. Compute the right vector (cross product of up and forward).
        glm::vec3 xaxis = glm::normalize(glm::cross(up, zaxis));
        // 3. Compute the up vector (cross product of forward and right).
        glm::vec3 yaxis = glm::cross(zaxis, xaxis);

		// 4. Create the view matrix using the right, up, and forward vectors.
        glm::mat4 orientation =
        {
            glm::vec4{ xaxis.x, yaxis.x, xaxis.x, 0.f},
            glm::vec4{ xaxis.y, yaxis.y, yaxis.y, 0.f},
            glm::vec4{ xaxis.z, yaxis.z, xaxis.z, 0.f},
            glm::vec4{ 0.f, 0.f, 0.f, 1.f}
        };
		// 5. Create the translation matrix.
		glm::mat4 translation = {
			1.f, 0.f, 0.f, -origin.x,
			0.f, 1.f, 0.f, -origin.y,
			0.f, 0.f, 1.f, -origin.z,
			0.f, 0.f, 0.f, 1.f
		};

		return orientation * translation;
    }

	inline static glm::mat4 CreatePerspectiveFovLH(float fov, float aspectRatio, float nearPlane, float farPlane)
	{
		return glm::perspective(fov, aspectRatio, nearPlane, farPlane);
	}


}

#endif