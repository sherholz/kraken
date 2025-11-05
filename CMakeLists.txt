cmake_minimum_required(VERSION 3.12)

project(kraken)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)
include(CMakeDependentOption)

option(BUILD_KRAKEN "Build the Kraken render engine" ON)
option(BUILD_HDKRAKEN "Build the Kraken USD Hydra delegate engine" ON)
option(BUILD_VIEWER "Build the USD viewer" ON)

option(BUILD_EXTERN "Build the Kraken render engine" ON)
option(DONWLOAD_DEPS "Download the USD build and its dependenzies" ON)

if(DONWLOAD_DEPS)
    include(dep_OpenUSD)
endif()

if(BUILD_EXTERN)
    add_subdirectory(extern)
endif()

if(BUILD_KRAKEN)
    add_subdirectory(kraken)
endif()

if(BUILD_HDKRAKEN)
    add_subdirectory(hdkraken)
endif()

if(BUILD_VIEWER)
    add_subdirectory(viewer)
endif()