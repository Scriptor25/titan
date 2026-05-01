file(GLOB_RECURSE LIB_SRC src/lib/*.cxx)

add_library(pkg_common ${LIB_SRC})
add_library(pkg::common ALIAS pkg_common)
target_include_directories(pkg_common PUBLIC inc)
target_link_libraries(pkg_common PUBLIC toolkit::json)

add_executable(pkg src/main.cxx)
target_include_directories(pkg PRIVATE inc)
target_link_libraries(pkg PRIVATE pkg::common)
