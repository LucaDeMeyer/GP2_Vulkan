

# Fetch External Libraries
include(FetchContent)

## GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 1.0.0  
)
FetchContent_MakeAvailable(glm)

## GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

## STB
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb
  GIT_TAG 5736b15
)
FetchContent_MakeAvailable(stb)

#tinyobjloader
FetchContent_Declare(
  tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader
  GIT_TAG release
)
FetchContent_MakeAvailable(tinyobjloader)

FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp
  GIT_TAG v5.3.1
)
FetchContent_MakeAvailable(assimp)

FetchContent_Declare(
    Imgui_external
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.8
)
FetchContent_MakeAvailable(Imgui_external)

add_library(imguilib
	${imgui_external_SOURCE_DIR}/imgui.cpp
	${imgui_external_SOURCE_DIR}/imgui_draw.cpp
	${imgui_external_SOURCE_DIR}/imgui_tables.cpp
	${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_external_SOURCE_DIR}/imgui_demo.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_external_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  
)

FetchContent_Declare(
    Vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.2.1
)
FetchContent_MakeAvailable(Vma)

add_library(vma_lib
    ${vma_SOURCE_DIR}/include/vk_mem_alloc.h
    ${vma_SOURCE_DIR}/src/VmaUsage.h
    ${vma_SOURCE_DIR}/src/VmaUsage.cpp
)

target_include_directories(imguilib PUBLIC ${imgui_external_SOURCE_DIR})

target_include_directories(vma_lib PUBLIC ${vma_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Buffers ${CMAKE_CURRENT_SOURCE_DIR}/Commands )



set(VULKAN_SOURCES 
VulkanContext.cpp
 VulkanUtils.cpp
 VulkanSwapChain.cpp
 WindowManager.cpp
VulkanPipeline.cpp 
Commands/VulkanCommandBuffer.cpp 
VulkanTexture.cpp 
VulkanSyncObjects.cpp
VulkanRenderer.cpp 
Buffers/VulkanVertexBuffer.cpp
Buffers/VulkanIndexBuffer.cpp 
Buffers/VulkanUniformBuffer.cpp 
VulkanDescriptorManager.cpp
Buffers/VulkanDepthBuffer.cpp  
Scene.cpp
Image.cpp
HDRManager.cpp
GBufferManager.cpp)

set(VULKAN_HEADERS
VulkanContext.h
VulkanUtils.h
WindowManager.h
VulkanPipeline.h
Commands/VulkanCommandBuffer.h 
VulkanTexture.h
VulkanSyncObjects.h
VulkanRenderer.h 
Buffers/VulkanVertexBuffer.h 
Buffers/VulkanIndexBuffer.h 
Buffers/VulkanUniformBuffer.h
VulkanDescriptorManager.h
Buffers/VulkanDepthBuffer.h
Scene.h
Image.h
MathHelpers.h
HDRManager.h
GBufferManager.h)


# Create a static library for the Vulkan utilities
add_library(VulkanLib STATIC ${VULKAN_SOURCES} ${VULKAN_HEADERS})

# Include the Vulkan library directory
target_include_directories(VulkanLib PUBLIC
    ${glm_SOURCE_DIR}        # GLM
    ${glfw_SOURCE_DIR}       # GLFW
    ${tinyobjloader_SOURCE_DIR} # tinyobjloader
    ${assimp_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${imgui_external_SOURCE_DIR}/backends
    ${assimp_SOURCE_DIR}/contrib
    ${imgui_external_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}  # Vulkan folder itself
    ${CMAKE_CURRENT_SOURCE_DIR}/Buffers
    ${CMAKE_CURRENT_SOURCE_DIR}/Commands
)




# Link Vulkan library
find_package(Vulkan REQUIRED)

target_link_libraries(vma_lib PRIVATE Vulkan::Vulkan)

target_link_libraries(imguilib PRIVATE Vulkan::Vulkan glfw)

target_link_libraries(VulkanLib PRIVATE Vulkan::Vulkan glfw assimp imguilib vma_lib)

