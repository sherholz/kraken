find_package(pxr CONFIG REQUIRED)

#find_package(nanogui CONFIG REQUIRED)

add_executable(viewer
  viewer.cpp
)


message(STATUS "PXR_CMAKE_DIR = ${PXR_CMAKE_DIR}")

target_link_libraries(viewer PRIVATE ${PXR_LIBRARIES} nanogui)

find_package(OpenGL REQUIRED)
add_dependencies(viewer nanogui)

# Adapt to variations in rpath handling on Linux and macOS
if (APPLE)
  set(VIEWER_ORIGIN "@loader_path")
elseif(UNIX)
  set(VIEWER_ORIGIN "$ORIGIN")
endif()

if (APPLE)
  set (VIEWER_RPATH
    "${PXR_CMAKE_DIR}/lib"
    "${CMAKE_INSTALL_PREFIX}/lib"
  )
    
  set_property(
    TARGET viewer 
    PROPERTY INSTALL_RPATH
    ${VIEWER_RPATH}
    )
else()
  set (VIEWER_RPATH
    "$ORIGIN:$ORIGIN/../lib:${PXR_CMAKE_DIR}/lib")

  set_target_properties(viewer PROPERTIES
    PREFIX ""
    INSTALL_RPATH ${VIEWER_RPATH}
    )
endif()

INSTALL(TARGETS viewer DESTINATION ${CMAKE_INSTALL_BINDIR})