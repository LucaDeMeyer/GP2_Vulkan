# Shader configuration
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/Shaders")

# Create output directory
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

# Find all shader files
file(GLOB SHADER_FILES
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
)

# Compile shaders
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_FILE "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BINARY_DIR}"
        COMMAND glslc "${SHADER}" -o "${SPIRV_FILE}"
        DEPENDS ${SHADER}
        VERBATIM
        COMMENT "Compiling shader ${SHADER_NAME}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV_FILE})
endforeach()

add_custom_target(Shaders ALL
    DEPENDS ${SPIRV_BINARY_FILES}
    COMMENT "Building all shaders"
)