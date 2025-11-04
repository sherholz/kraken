cmake_minimum_required(VERSION 3.12)

project(hdkaken VERSION 1.0 LANGUAGES C CXX)

find_package(pxr CONFIG REQUIRED)

set(HDKRAKEN_SRC)


add_library(hdKraken SHARED
    ${HDKRAKEN_SRC}
        src/renderDelegate.cpp
  )


if(USE_HOUDINI_USD)
set (HDKRAKEN_RPATH
  "$ORIGIN:${HOUDINI_ROOT}/dsolib:$ORIGIN/../../lib")
else()
set (HDKRAKEN_RPATH
  "$ORIGIN:${pxr_DIR}/lib:$ORIGIN/../../lib")
endif()

set_target_properties(hdKraken PROPERTIES
  PREFIX ""
  INSTALL_RPATH ${HDKRAKEN_RPATH}
)
install(TARGETS hdKraken DESTINATION plugin/usd)

# plugInfo.json
set(PLUG_CLASS "HdKrakenRendererPlugin")
set(PLUG_DISPLAYNAME "Kraken")
set(PLUG_NAME "hdKraken")
set(PLUG_INFO_LIBRARY_PATH "../${PLUG_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(PLUG_INFO_RESOURCE_PATH "resources")
set(PLUG_INFO_ROOT "..")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in
  ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json" DESTINATION "plugin/usd/${PLUG_NAME}/resources")