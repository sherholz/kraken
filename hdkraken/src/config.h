//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_KRAKEN_CONFIG_H
#define PXR_IMAGING_PLUGIN_HD_KRAKEN_CONFIG_H

#include <pxr/pxr.h>
#include <pxr/base/tf/singleton.h>

PXR_NAMESPACE_OPEN_SCOPE

// NOTE: types here restricted to bool/int/string, as also used for
// TF_DEFINE_ENV_SETTING
constexpr int HdKrakenDefaultSamplesToConvergence = 100;
constexpr int HdKrakenDefaultTileSize = 8;
constexpr int HdKrakenDefaultAmbientOcclusionSamples = 16;
constexpr bool HdKrakenDefaultJitterCamera = true;
constexpr bool HdKrakenDefaultUseFaceColors = true;
constexpr int HdKrakenDefaultCameraLightIntensity = 300;
constexpr int HdKrakenDefaultRandomNumberSeed = -1;
constexpr bool HdKrakenDefaultUseLighting = false;

/// \class HdKrakenConfig
///
/// This class is a singleton, holding configuration parameters for HdKraken.
/// Everything is provided with a default, but can be overridden using
/// environment variables before launching a hydra process.
///
/// Many of the parameters can be used to control quality/performance
/// tradeoffs, or to alter how HdKraken takes advantage of parallelism.
///
/// At startup, this class will print config parameters if
/// *HdKraken_PRINT_CONFIGURATION* is true. Integer values greater than zero
/// are considered "true".
///
class HdKrakenConfig {
public:

    /// \brief Return the configuration singleton.
    static const HdKrakenConfig &GetInstance();

    /// How many samples do we need before a pixel is considered
    /// converged?
    ///
    /// Override with *HdKraken_SAMPLES_TO_CONVERGENCE*.
    unsigned int samplesToConvergence = HdKrakenDefaultSamplesToConvergence;

    /// How many pixels are in an atomic unit of parallel work?
    /// A work item is a square of size [tileSize x tileSize] pixels.
    ///
    /// Override with *HdKraken_TILE_SIZE*.
    unsigned int tileSize = HdKrakenDefaultTileSize;

    /// How many ambient occlusion rays should we generate per
    /// camera ray?
    ///
    /// Override with *HdKraken_AMBIENT_OCCLUSION_SAMPLES*.
    unsigned int ambientOcclusionSamples = HdKrakenDefaultAmbientOcclusionSamples;

    /// Should the renderpass jitter camera rays for antialiasing?
    ///
    /// Override with *HdKraken_JITTER_CAMERA*. The case-insensitive strings
    /// "true", "yes", "on", and "1" are considered true; an empty value uses
    /// the default, and all other values are false.
    bool jitterCamera = HdKrakenDefaultJitterCamera;

    /// Should the renderpass use the color primvar, or flat white colors?
    /// (Flat white shows off ambient occlusion better).
    ///
    /// Override with *HdKraken_USE_FACE_COLORS*.  The case-insensitive strings
    /// "true", "yes", "on", and "1" are considered true; an empty value uses
    /// the default, and all other values are false.
    bool useFaceColors = HdKrakenDefaultUseFaceColors;

    /// What should the intensity of the camera light be, specified as a
    /// percent of <1, 1, 1>.  For example, 300 would be <3, 3, 3>.
    ///
    /// Override with *HdKraken_CAMERA_LIGHT_INTENSITY*.
    float cameraLightIntensity = HdKrakenDefaultCameraLightIntensity;

    /// Seed to give to the random number generator. A value of anything other
    /// than -1, combined with setting PXR_WORK_THREAD_LIMIT=1, should give
    /// deterministic / repeatable results. A value of -1 (the default) will
    /// allow the implementation to set a value that varies from invocation to
    /// invocation and thread to thread.
    ///
    /// Override with *HdKraken_RANDOM_NUMBER_SEED*.
    int randomNumberSeed = HdKrakenDefaultRandomNumberSeed;

    /// Should the renderpass use scene lights (in particular, UsdLux-compliant
    /// area lights)?  Note that if scene lights and ambient occlusion are both
    /// enabled, the renderer will choose scene lights rather than ambient
    /// occlusion.
    ///
    /// Override with *HdKraken_USE_LIGHTING*.
    bool useLighting = HdKrakenDefaultUseLighting;

private:
    // The constructor initializes the config variables with their
    // default or environment-provided override, and optionally prints
    // them.
    HdKrakenConfig();
    ~HdKrakenConfig() = default;

    HdKrakenConfig(const HdKrakenConfig&) = delete;
    HdKrakenConfig& operator=(const HdKrakenConfig&) = delete;

    friend class TfSingleton<HdKrakenConfig>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_KRAKEN_CONFIG_H
