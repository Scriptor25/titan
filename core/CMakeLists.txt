file(GLOB_RECURSE SRC src/**.cxx)

find_package(glfw3 3.4 REQUIRED)

find_package(Vulkan REQUIRED)

find_package(OpenAL REQUIRED)
find_package(OpenXR REQUIRED)

find_package(glm CONFIG REQUIRED)

add_library(core ${SRC})
target_include_directories(core PUBLIC inc)
target_link_libraries(core PUBLIC glfw Vulkan::Vulkan OpenAL::OpenAL OpenXR::openxr_loader glm::glm)
