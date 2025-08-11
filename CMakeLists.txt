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

# Shader directories
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_SOURCE_DIR}/shaders/spirv")

file(GLOB SLANG_FILES "${SHADER_SRC_DIR}/*.slang")
set(SLANG_SPV_OUTPUTS "")

foreach(SLANG_FILE ${SLANG_FILES})
    get_filename_component(FILE_NAME ${SLANG_FILE} NAME_WE)

   if(FILE_NAME MATCHES "_cs$")
        # This is a compute shader
        set(COMPUTE_OUT "${SHADER_OUT_DIR}/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${COMPUTE_OUT}
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling compute shader: ${FILE_NAME}"
            COMMAND slangc ${SLANG_FILE} -entry compute_main -profile cs_6_0 -target spirv -o ${COMPUTE_OUT} -g
            DEPENDS ${SLANG_FILE}
            VERBATIM
        )
        list(APPEND SLANG_SPV_OUTPUTS ${COMPUTE_OUT})

    elseif(FILE_NAME MATCHES "_frag$")
        # This is a compute shader
        set(FRAGMENT_OUT "${SHADER_OUT_DIR}/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${FRAGMENT_OUT}
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling Fragment shader: ${FILE_NAME}"
            COMMAND slangc ${SLANG_FILE} -entry fragment_main -profile ps_6_0 -target spirv -o ${FRAGMENT_OUT} -g
            DEPENDS ${SLANG_FILE}
            VERBATIM
        )
        list(APPEND SLANG_SPV_OUTPUTS ${FRAGMENT_OUT})

    else()
        # Vertex shader output
        set(VERT_OUT "${SHADER_OUT_DIR}/${FILE_NAME}.vert.spv")
        add_custom_command(
            OUTPUT ${VERT_OUT}
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling vertex shader: ${FILE_NAME}"
            COMMAND slangc ${SLANG_FILE} -entry vertex_main -profile vs_6_0 -target spirv -o ${VERT_OUT} -g 
            DEPENDS ${SLANG_FILE}
            VERBATIM
        )
        list(APPEND SLANG_SPV_OUTPUTS ${VERT_OUT})

        # Fragment shader output
        set(FRAG_OUT "${SHADER_OUT_DIR}/${FILE_NAME}.frag.spv")
        add_custom_command(
            OUTPUT ${FRAG_OUT}
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling fragment shader: ${FILE_NAME}"
            COMMAND slangc ${SLANG_FILE} -entry fragment_main -profile ps_6_0 -target spirv -o ${FRAG_OUT} -g
            DEPENDS ${SLANG_FILE}
            VERBATIM
        )
        list(APPEND SLANG_SPV_OUTPUTS ${FRAG_OUT})
    endif()
endforeach()

# CompileShaders target that depends on all SPIR-V outputs
add_custom_target(CompileShaders ALL
    DEPENDS ${SLANG_SPV_OUTPUTS}
)

# Make sure shaders compile before niji builds
add_dependencies(niji CompileShaders)

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
    COMMENT "Copying assets and shaders folders to output directory"
)

# Sub-directories
add_subdirectory("lib")