#pragma once

#include "ParticleSystem.hpp"

#include <cstddef>
#include <string>

struct AppSettings {
    std::size_t particleCount = 5200;
    float particleScale = 1.0f;
    float trailPersistence = 0.68f;
    float forceStrength = 1.0f;
    float backgroundIntensity = 1.0f;
    float cameraFov = 58.0f;
    float cameraSensitivity = 1.0f;
    bool showHud = true;
    bool dynamicBackground = true;
    bool verticalSync = true;
    std::size_t paletteIndex = 0;
    FlowMode flowMode = FlowMode::CosmicWeb;

    void reset();
    bool load(const std::string& path);
    bool save(const std::string& path) const;
};
