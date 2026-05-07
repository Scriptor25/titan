set(RESOURCE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(RESOURCE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

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
        COMMENT "Compiling resources"
)
add_custom_target(resources SOURCES ${RESOURCES} DEPENDS ${RESOURCE_BINARY_DIR}/index)

file(GLOB_RECURSE SRC src/*.cxx)

add_executable(game ${SRC})
add_dependencies(game resources)
target_include_directories(game PRIVATE inc)
target_link_libraries(game PRIVATE core)
