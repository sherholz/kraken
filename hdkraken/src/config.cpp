//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "config.h"

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/instantiateSingleton.h>

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Instantiate the config singleton.
TF_INSTANTIATE_SINGLETON(HdKrakenConfig);

// Each configuration variable has an associated environment variable.
// The environment variable macro takes the variable name, a default value,
// and a description...
TF_DEFINE_ENV_SETTING(
    HDKRAKEN_SAMPLES_TO_CONVERGENCE,
    HdKrakenDefaultSamplesToConvergence,
    "Samples per pixel before we stop rendering (must be >= 1)");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_TILE_SIZE,
    HdKrakenDefaultTileSize,
    "Size (per axis) of threading work units (must be >= 1)");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_AMBIENT_OCCLUSION_SAMPLES,
    HdKrakenDefaultAmbientOcclusionSamples,
    "Ambient occlusion samples per camera ray (must be >= 0;"
    " a value of 0 disables ambient occlusion)");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_JITTER_CAMERA,
    HdKrakenDefaultJitterCamera,
    "Should HdEmbree jitter camera rays while rendering?");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_USE_FACE_COLORS,
    HdKrakenDefaultUseFaceColors,
    "Should HdEmbree use face colors while rendering?");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_CAMERA_LIGHT_INTENSITY,
    HdKrakenDefaultCameraLightIntensity,
    "Intensity of the camera light, specified as a percentage of <1,1,1>.");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_RANDOM_NUMBER_SEED,
    HdKrakenDefaultRandomNumberSeed,
    "Seed to give to the random number generator. A value of anything other"
        " than -1, combined with setting PXR_WORK_THREAD_LIMIT=1, should"
        " give deterministic / repeatable results. A value of -1 (the"
        " default) will allow the implementation to set a value that varies"
        " from invocation to invocation and thread to thread.");

TF_DEFINE_ENV_SETTING(
    HDKRAKEN_USE_LIGHTING,
    HdKrakenDefaultUseLighting,
    "Should HdEmbree use scene lights while rendering?");

TF_DEFINE_ENV_SETTING(HDKRAKEN_PRINT_CONFIGURATION,
    false,
    "Should HdEmbree print configuration on startup?");

HdKrakenConfig::HdKrakenConfig()
{
    // Read in values from the environment, clamping them to valid ranges.
    samplesToConvergence = std::max(1,
            TfGetEnvSetting(HDKRAKEN_SAMPLES_TO_CONVERGENCE));
    tileSize = std::max(1,
            TfGetEnvSetting(HDKRAKEN_TILE_SIZE));
    ambientOcclusionSamples = std::max(0,
            TfGetEnvSetting(HDKRAKEN_AMBIENT_OCCLUSION_SAMPLES));
    jitterCamera = (TfGetEnvSetting(HDKRAKEN_JITTER_CAMERA));
    useFaceColors = (TfGetEnvSetting(HDKRAKEN_USE_FACE_COLORS));
    cameraLightIntensity = (std::max(100,
            TfGetEnvSetting(HDKRAKEN_CAMERA_LIGHT_INTENSITY)) / 100.0f);
    randomNumberSeed = TfGetEnvSetting(HDKRAKEN_RANDOM_NUMBER_SEED);
    useLighting = (TfGetEnvSetting(HDKRAKEN_USE_LIGHTING));

    if (TfGetEnvSetting(HDKRAKEN_PRINT_CONFIGURATION)) {
        std::cout
            << "HdEmbree Configuration: \n"
            << "  samplesToConvergence       = "
            <<    samplesToConvergence    << "\n"
            << "  tileSize                   = "
            <<    tileSize                << "\n"
            << "  ambientOcclusionSamples    = "
            <<    ambientOcclusionSamples << "\n"
            << "  jitterCamera               = "
            <<    jitterCamera            << "\n"
            << "  useFaceColors              = "
            <<    useFaceColors           << "\n"
            << "  cameraLightIntensity      = "
            <<    cameraLightIntensity    << "\n"
            << "  randomNumberSeed          = "
            <<    randomNumberSeed        << "\n"
            << "  useLighting               = "
            <<    useLighting             << "\n"
            ;
    }
}

/*static*/
const HdKrakenConfig&
HdKrakenConfig::GetInstance()
{
    return TfSingleton<HdKrakenConfig>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
