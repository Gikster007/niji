cmake_minimum_required(VERSION 3.15)
project(niji)

# C++ standard version
set(CMAKE_CXX_STANDARD 17 CACHE STRING "" FORCE)

# Program executable target
add_executable(niji)
set_target_properties(niji PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

# Add compiler (debug & release) definitions
target_compile_definitions(niji PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)
target_compile_definitions(niji PRIVATE $<$<CONFIG:Release>:NDEBUG=1>)
# Add WINDOWS compiler macro
target_compile_definitions(niji PRIVATE "WINDOWS=$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>")

# Includes
target_include_directories(niji PRIVATE "./src/engine")

# Pre-compiled header
target_precompile_headers(niji PRIVATE "./src/precomp.hpp")

# Glob all source files
file(GLOB_RECURSE SOURCES
	"src/*.cpp"
)

# Add source files to the program
target_sources(niji PRIVATE
	${SOURCES}
)

set_target_properties(niji PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)

# Always copy the assets folder after building
add_custom_command(TARGET niji POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:niji>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:niji>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:niji>/textures
    COMMENT "Copying assets and shaders folders to output directory"
)

# Sub-directories
add_subdirectory("lib")