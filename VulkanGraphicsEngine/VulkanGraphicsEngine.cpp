#include <exception>
#include <iostream>
#include "Vulkan/VulkanRenderer.h"
class VulkanApp
{
public:
	void run()
	{
		vulkanRenderer.CreateVulkanManagers();
		vulkanRenderer.InitVulkan();
		//vulkanRenderer.InitImGui();
		vulkanRenderer.MainLoop();
		vulkanRenderer.CleanupVulkan();
	}
private:
	VulkanRenderer vulkanRenderer{};
};

int main()
{
	VulkanApp app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

