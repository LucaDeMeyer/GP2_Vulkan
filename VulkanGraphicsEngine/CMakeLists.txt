cmake_minimum_required(VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(VulkanGraphicsEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Subdirectories
add_subdirectory(Shaders)
add_subdirectory(Vulkan) # Vulkan now compiles separately into VulkanLib

include(FetchContent)

FetchContent_Declare(
    asset_repo
    GIT_REPOSITORY https://github.com/LucaDeMeyer/VulkanGraphicsEngine_Resources
    GIT_TAG main
)

FetchContent_MakeAvailable(asset_repo)

# Correct usage of the FetchContent source directory
set(MODELS_SOURCE_DIR "${asset_repo_SOURCE_DIR}/Models")
set(TEXTURES_SOURCE_DIR "${asset_repo_SOURCE_DIR}/Textures")

# Define the destination directories where models and textures will be copied
set(MODELS_DEST_DIR "${CMAKE_BINARY_DIR}/Models")
set(TEXTURES_DEST_DIR "${CMAKE_BINARY_DIR}/Textures")

# Create custom targets to copy the directories
add_custom_target(copy_models ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODELS_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${MODELS_SOURCE_DIR}"
        "${MODELS_DEST_DIR}/"
    COMMENT "Copying model files to build directory"
)

add_custom_target(copy_textures ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEXTURES_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${TEXTURES_SOURCE_DIR}"
        "${TEXTURES_DEST_DIR}/"
    COMMENT "Copying texture files to build directory"
)


# Vulkan Engine Executable
add_executable(VulkanGraphicsEngine VulkanGraphicsEngine.cpp VulkanGraphicsEngine.h)

# Find Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(VulkanGraphicsEngine PRIVATE ${Vulkan_INCLUDE_DIRS})

# Link Libraries for VulkanGraphicsEngine
target_link_libraries(VulkanGraphicsEngine
    PRIVATE
        VulkanLib    # Linking to VulkanLib static library
        glfw         # Link GLFW
        Vulkan::Vulkan
     
)

# Dependencies for VulkanGraphicsEngine
add_dependencies(VulkanGraphicsEngine copy_models)
add_dependencies(VulkanGraphicsEngine copy_textures)
add_dependencies(VulkanGraphicsEngine Shaders)
