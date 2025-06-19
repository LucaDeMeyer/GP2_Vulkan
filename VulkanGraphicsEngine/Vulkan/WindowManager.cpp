#include "WindowManager.h"
#include <stdexcept>  // For throwing exceptions

void WindowManager::InitWindow(int width, int height, const char* title) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(width, width, "Vulkan Graphics Engine", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
}