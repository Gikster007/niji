# GLM
# https://github.com/g-truc/glm
add_subdirectory("glm")
target_link_libraries(niji PRIVATE glm)
target_include_directories(niji PRIVATE "./glm/")

# VMA
# https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
target_include_directories(niji PRIVATE "./vma/")

# FastGLTF
# https://github.com/spnda/fastgltf
include(FetchContent)
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG main
)
FetchContent_MakeAvailable(fastgltf)
target_link_libraries(niji PRIVATE fastgltf)

# GLFW
# https://github.com/glfw/glfw
target_include_directories(niji PRIVATE ${CMAKE_SOURCE_DIR}/lib/glfw/include)
set(GLFW_LIBRARY ${CMAKE_SOURCE_DIR}/lib/glfw/glfw3.lib)
target_link_libraries(niji PRIVATE ${GLFW_LIBRARY})

# STB_Image
# https://github.com/nothings/stb/blob/master/stb_image.h
target_include_directories(niji PRIVATE "./stb/")

# EnTT ECS
add_subdirectory("entt")
target_link_libraries(niji PUBLIC EnTT)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(niji PRIVATE Vulkan::Vulkan)