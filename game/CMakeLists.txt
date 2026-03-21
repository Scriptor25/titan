function(add_shaders TARGET_NAME)
    set(ARGS ${ARGN})

    if (NOT ARGS)
        message(FATAL_ERROR "No SOURCES given to target: ${TARGET_NAME}")
    endif ()

    set(SOURCES)
    set(BINARIES)

    while (ARGS)
        list(POP_FRONT ARGS STAGE)
        list(POP_FRONT ARGS SOURCE)

        get_filename_component(SOURCE_ABSOLUTE ${SOURCE} REALPATH ${CMAKE_CURRENT_SOURCE_DIR})
        file(RELATIVE_PATH FILENAME ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCE_ABSOLUTE})

        set(BINARY_ABSOLUTE ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv)

        list(APPEND SOURCES ${SOURCE_ABSOLUTE})
        list(APPEND BINARIES ${BINARY_ABSOLUTE})

        add_custom_command(
                DEPENDS ${SOURCE_ABSOLUTE}
                OUTPUT ${BINARY_ABSOLUTE}
                COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=${STAGE} -o ${BINARY_ABSOLUTE} ${SOURCE_ABSOLUTE}
                COMMENT "Building SPV object ${BINARY_ABSOLUTE}"
                VERBATIM
        )
    endwhile ()

    add_custom_target(${TARGET_NAME} SOURCES ${SOURCES} DEPENDS ${BINARIES})
endfunction()

add_shaders(shaders
        vert res/shader/vert.glsl
        frag res/shader/frag.glsl
)

file(GLOB_RECURSE SRC src/*.cxx)

add_executable(game ${SRC})
add_dependencies(game shaders)
target_include_directories(game PRIVATE inc)
target_link_libraries(game PRIVATE core)
