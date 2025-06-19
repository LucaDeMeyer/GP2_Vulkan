
#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "VulkanRenderer.h"
#include "GLFW/glfw3.h"

class WindowManager {
public:
    static WindowManager& GetInstance() {
        static WindowManager instance;
        return instance;
    }

    void InitWindow(int width, int height, const char* title);
    GLFWwindow* GetWindow() const { return window; }

    void SetRenderer(VulkanRenderer* renderer) {
        this->renderer = renderer;
    }

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window)); // Or your app class
        app->framebufferResized = true;
    }
private:
    WindowManager() = default;
    ~WindowManager() = default;

    GLFWwindow* window = nullptr;
    VulkanRenderer* renderer = nullptr;
};

#endif