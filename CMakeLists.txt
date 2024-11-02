cmake_minimum_required(VERSION 3.15)
project(molten)

# C++ standard version
set(CMAKE_CXX_STANDARD 17 CACHE STRING "" FORCE)

# Program executable target
add_executable(molten)
set_target_properties(molten PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

# Add DEBUG compiler macro
# target_compile_definitions(main PRIVATE "DEBUG=$<CONFIG:Debug>")
# Add NDEBUG compiler macro
# target_compile_definitions(main PRIVATE "NDEBUG=$<CONFIG:Release>")
# Add WINDOWS compiler macro
target_compile_definitions(molten PRIVATE "WINDOWS=$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>")

# Post build command (copies the asset files)
add_custom_command(TARGET molten PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
    ${PROJECT_SOURCE_DIR}/public 
    ${PROJECT_BINARY_DIR}/$<CONFIG>/public || (exit 0) # ignore failed copy.
)

# Includes
target_include_directories(molten PRIVATE "./src/")

# Pre-compiled header
target_precompile_headers(molten PRIVATE "./src/precomp.h")

# Glob all source files
file(GLOB_RECURSE SOURCES
	"src/*.cpp"
)

# Add source files to the program
target_sources(molten PRIVATE
	${SOURCES}
)

# Sub-directories
add_subdirectory("lib")