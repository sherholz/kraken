cmake_minimum_required (VERSION 3.12)

set(NANOGUI_BUILD_EXAMPLES OFF)
set(NANOGUI_BUILD_PYTHON OFF)

add_subdirectory (nanogui)


set(GLM_BUILD_INSTALL ON)
add_subdirectory (glm)