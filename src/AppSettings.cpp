#include "AppSettings.hpp"

#include <algorithm>
#include <fstream>
#include <string>

namespace {
bool parseBool(const std::string& value) {
    return value == "1" || value == "true" || value == "on";
}
}

void AppSettings::reset() {
    *this = AppSettings{};
}

bool AppSettings::load(const std::string& path) {
    std::ifstream file(path);
    if (!file) return false;

    std::string line;
    try {
        while (std::getline(file, line)) {
            const std::size_t separator = line.find('=');
            if (separator == std::string::npos) continue;
            const std::string key = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);

            if (key == "particle_count") particleCount = static_cast<std::size_t>(std::stoul(value));
            else if (key == "particle_scale") particleScale = std::stof(value);
            else if (key == "trail_persistence") trailPersistence = std::stof(value);
            else if (key == "force_strength") forceStrength = std::stof(value);
            else if (key == "background_intensity") backgroundIntensity = std::stof(value);
            else if (key == "camera_fov") cameraFov = std::stof(value);
            else if (key == "camera_sensitivity") cameraSensitivity = std::stof(value);
            else if (key == "show_hud") showHud = parseBool(value);
            else if (key == "dynamic_background") dynamicBackground = parseBool(value);
            else if (key == "vertical_sync") verticalSync = parseBool(value);
            else if (key == "palette") paletteIndex = static_cast<std::size_t>(std::stoul(value));
            else if (key == "flow_mode") flowMode = static_cast<FlowMode>(std::stoi(value));
        }
    } catch (...) {
        reset();
        return false;
    }

    particleCount = std::clamp<std::size_t>(particleCount, 1000, 12000);
    particleScale = std::clamp(particleScale, 0.6f, 2.2f);
    trailPersistence = std::clamp(trailPersistence, 0.10f, 0.95f);
    forceStrength = std::clamp(forceStrength, 0.45f, 2.2f);
    backgroundIntensity = std::clamp(backgroundIntensity, 0.15f, 1.6f);
    cameraFov = std::clamp(cameraFov, 42.0f, 78.0f);
    cameraSensitivity = std::clamp(cameraSensitivity, 0.35f, 2.2f);
    paletteIndex %= 6;
    const int mode = std::clamp(static_cast<int>(flowMode), 0, 4);
    flowMode = static_cast<FlowMode>(mode);
    return true;
}

bool AppSettings::save(const std::string& path) const {
    std::ofstream file(path, std::ios::trunc);
    if (!file) return false;

    file << "particle_count=" << particleCount << '\n';
    file << "particle_scale=" << particleScale << '\n';
    file << "trail_persistence=" << trailPersistence << '\n';
    file << "force_strength=" << forceStrength << '\n';
    file << "background_intensity=" << backgroundIntensity << '\n';
    file << "camera_fov=" << cameraFov << '\n';
    file << "camera_sensitivity=" << cameraSensitivity << '\n';
    file << "show_hud=" << (showHud ? 1 : 0) << '\n';
    file << "dynamic_background=" << (dynamicBackground ? 1 : 0) << '\n';
    file << "vertical_sync=" << (verticalSync ? 1 : 0) << '\n';
    file << "palette=" << paletteIndex << '\n';
    file << "flow_mode=" << static_cast<int>(flowMode) << '\n';
    return static_cast<bool>(file);
}
