include(FetchContent)

set(COMPONENT_NAME openusd)

message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

set(COMPONENT_PATH ${CMAKE_INSTALL_PREFIX})

set(OPENUSD_VERSION "25.11")

if (APPLE)
  set(OPENUSD_OSSUFFIX "macos-arm64.tgz")
elseif(WIN32)
  set(OPENUSD_OSSUFFIX "win11-x64.zip")
else()
    set(OPENUSD_OSSUFFIX "linux-x64.tar.xz")
endif()

set(OPENUSD_URL "https://github.com/sherholz/kraken/releases/download/OpenUSD-v${OPENUSD_VERSION}/OpenUSD-v${OPENUSD_VERSION}-${OPENUSD_OSSUFFIX}")

#set(OpenUSD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/pxr")
#set(openusd_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/pxr")

FetchContent_Populate(
  ${COMPONENT_NAME}
  URL ${OPENUSD_URL}
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/deps/pxr
  )