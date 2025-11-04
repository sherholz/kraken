cmake_minimum_required(VERSION 3.12)

project(kraken)

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)
include(CMakeDependentOption)

option(BUILD_KRAKEN "Build the Kraken render engine" ON)
option(BUILD_HDKRAKEN "Build the Kraken USD Hydra delegate engine" ON)
option(BUILD_VIEWER "Build the USD viewer" ON)

option(BUILD_EXTERN "Build the Kraken render engine" ON)

message(STATUS "pxr_DIR = ${pxr_DIR}")

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