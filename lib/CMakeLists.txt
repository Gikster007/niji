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
    GIT_TAG v0.9.0
)
FetchContent_MakeAvailable(fastgltf)
target_link_libraries(niji PRIVATE fastgltf)

# MikkTSpace
# https://github.com/mmikk/MikkTSpace
add_library(mikktspace STATIC
    ${CMAKE_SOURCE_DIR}/lib/mikktspace/mikktspace.c
)
target_include_directories(mikktspace PUBLIC ${CMAKE_SOURCE_DIR}/lib/mikktspace)
target_link_libraries(niji PRIVATE mikktspace)

# GLFW
# https://github.com/glfw/glfw
#target_include_directories(niji PRIVATE ${CMAKE_SOURCE_DIR}/lib/glfw/include)
#set(GLFW_LIBRARY ${CMAKE_SOURCE_DIR}/lib/glfw/glfw3.lib)
#target_link_libraries(niji PRIVATE ${GLFW_LIBRARY})
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/glfw/glfw3.lib
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/lib/glfw/include
)

# nlohmann::json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(niji PRIVATE nlohmann_json::nlohmann_json)

# STB_Image
# https://github.com/nothings/stb/blob/master/stb_image.h
target_include_directories(niji PRIVATE "./stb/")

# EnTT ECS
add_subdirectory("entt")
target_link_libraries(niji PUBLIC EnTT)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(niji PRIVATE Vulkan::Vulkan)

# Add ImGui
# https://github.com/ocornut/imgui/tree/docking & https://github.com/CedricGuillemet/ImGuizmo
add_library(imgui
    ${CMAKE_SOURCE_DIR}/lib/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/lib/imgui/ImGuizmo.cpp
)
target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/imgui
    ${CMAKE_SOURCE_DIR}/lib/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
target_link_libraries(niji PUBLIC imgui glfw)