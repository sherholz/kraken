find_package(pxr CONFIG REQUIRED)

#find_package(nanogui CONFIG REQUIRED)

add_executable(viewer
  viewer.cpp
)


message(STATUS "PXR_CMAKE_DIR = ${PXR_CMAKE_DIR}")

target_link_libraries(viewer PRIVATE ${PXR_LIBRARIES} nanogui)

find_package(OpenGL REQUIRED)
add_dependencies(viewer nanogui)

set (VIEWER_RPATH
  "$ORIGIN:$ORIGIN/../lib:${PXR_CMAKE_DIR}/lib")

set_target_properties(viewer PROPERTIES
  PREFIX ""
  INSTALL_RPATH ${VIEWER_RPATH}
  )

INSTALL(TARGETS viewer DESTINATION ${CMAKE_INSTALL_BINDIR})