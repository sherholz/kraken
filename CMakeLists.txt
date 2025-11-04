cmake_minimum_required(VERSION 3.5)

project(kraken)

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)
include(CMakeDependentOption)

option(BUILD_KRAKEN "Build the Kraken render engine" ON)
option(BUILD_HDKRAKEN "Build the Kraken USD Hydra delegate engine" ON)
option(Viewer "Build the USD viewer" ON)

message(STATUS "pxr_DIR = ${pxr_DIR}")



if(BUILD_KRAKEN)
    add_subdirectory(kraken)
endif()

if(BUILD_HDKRAKEN)
    add_subdirectory(hdkraken)
endif()

if(BUILD_viewer)
    add_subdirectory(viewer)
endif()