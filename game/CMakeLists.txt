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
        get_filename_component(SOURCE_DIRECTORY ${SOURCE_ABSOLUTE} DIRECTORY)
        get_filename_component(SOURCE_FILENAME ${SOURCE_ABSOLUTE} NAME_WLE)

        file(RELATIVE_PATH RELATIVE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCE_DIRECTORY})

        set(BINARY_ABSOLUTE ${CMAKE_CURRENT_BINARY_DIR}/${RELATIVE_DIRECTORY}/${SOURCE_FILENAME}.spv)

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

set(RESOURCE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(RESOURCE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/res)

file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS ${RESOURCE_SOURCE_DIR}/*.json)

set(RESOURCES_RELATIVE)
foreach (path ${RESOURCES})
    file(RELATIVE_PATH relative ${RESOURCE_SOURCE_DIR} ${path})
    list(APPEND RESOURCES_RELATIVE "${relative}")
    message(STATUS "${relative}")
endforeach ()

add_custom_command(
        OUTPUT ${RESOURCE_BINARY_DIR}/index
        COMMAND pkg ${RESOURCE_BINARY_DIR} ${RESOURCES_RELATIVE}
        WORKING_DIRECTORY ${RESOURCE_SOURCE_DIR}
        DEPENDS pkg
        COMMENT "package resources"
)
add_custom_target(resources SOURCES ${RESOURCES} DEPENDS ${RESOURCE_BINARY_DIR}/index)

file(GLOB_RECURSE SRC src/*.cxx)

add_executable(game ${SRC})
add_dependencies(game resources)
target_include_directories(game PRIVATE inc)
target_link_libraries(game PRIVATE core)
