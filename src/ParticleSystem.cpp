#include "ParticleSystem.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;

sf::Vector3f add3(const sf::Vector3f& a, const sf::Vector3f& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

sf::Vector3f sub3(const sf::Vector3f& a, const sf::Vector3f& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

sf::Vector3f mul3(const sf::Vector3f& v, float scalar) {
    return {v.x * scalar, v.y * scalar, v.z * scalar};
}

float dot3(const sf::Vector3f& a, const sf::Vector3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float lengthSquared3(const sf::Vector3f& value) {
    return dot3(value, value);
}

float length3(const sf::Vector3f& value) {
    return std::sqrt(lengthSquared3(value));
}

sf::Vector3f normalized3(const sf::Vector3f& value) {
    const float magnitude = length3(value);
    return magnitude > 0.0001f ? mul3(value, 1.0f / magnitude) : sf::Vector3f{};
}

sf::Vector3f cross3(const sf::Vector3f& a, const sf::Vector3f& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

float length2(sf::Vector2f value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

sf::Vector2f normalized2(sf::Vector2f value) {
    const float magnitude = length2(value);
    return magnitude > 0.0001f ? sf::Vector2f{value.x / magnitude, value.y / magnitude}
                               : sf::Vector2f{1.0f, 0.0f};
}

sf::Color mixColor(sf::Color a, sf::Color b, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    const auto channel = [amount](sf::Uint8 x, sf::Uint8 y) {
        return static_cast<sf::Uint8>(static_cast<float>(x) +
            (static_cast<float>(y) - static_cast<float>(x)) * amount);
    };
    return {channel(a.r, b.r), channel(a.g, b.g), channel(a.b, b.b), channel(a.a, b.a)};
}

sf::Uint8 scaledAlpha(sf::Uint8 value, float amount) {
    return static_cast<sf::Uint8>(std::clamp(static_cast<float>(value) * amount, 0.0f, 255.0f));
}
}

const char* flowModeName(FlowMode mode) {
    switch (mode) {
        case FlowMode::BinaryGalaxy: return "BINARY GALAXY";
        case FlowMode::NebulaRiver: return "NEBULA RIVER";
        case FlowMode::SolarBloom: return "SOLAR BLOOM";
        case FlowMode::CosmicWeb: return "COSMIC WEB";
        case FlowMode::AuroraDrift: return "AURORA DRIFT";
    }
    return "COSMIC WEB";
}

const char* flowModeSubtitle(FlowMode mode) {
    switch (mode) {
        case FlowMode::BinaryGalaxy: return "TWIN ORBITS AND BRAIDED SPIRALS";
        case FlowMode::NebulaRiver: return "A DEEP MEANDERING CURRENT";
        case FlowMode::SolarBloom: return "PULSES FROM A LUMINOUS CORE";
        case FlowMode::CosmicWeb: return "FILAMENTS ACROSS THE VOLUME";
        case FlowMode::AuroraDrift: return "SLOW WAVES IN LAYERED LIGHT";
    }
    return "FILAMENTS ACROSS THE VOLUME";
}

const std::array<Palette, 6> ParticleSystem::palettes_ = {{
    {"AURORA", {{{66, 245, 209}, {95, 142, 255}, {195, 91, 255}, {255, 112, 207}}}},
    {"EMBER",  {{{255, 230, 120}, {255, 148, 61}, {255, 72, 82}, {185, 46, 116}}}},
    {"OCEAN",  {{{100, 255, 244}, {53, 187, 255}, {61, 104, 255}, {120, 77, 255}}}},
    {"MONO",   {{{245, 247, 255}, {186, 202, 226}, {119, 145, 181}, {72, 91, 121}}}},
    {"NEON",   {{{57, 255, 20}, {0, 240, 255}, {255, 41, 179}, {160, 56, 255}}}},
    {"SOLAR",  {{{255, 252, 183}, {255, 205, 64}, {255, 104, 46}, {229, 34, 91}}}}
}};

ParticleSystem::ParticleSystem(sf::Vector2u viewport, std::size_t count)
    : viewport_(viewport), rng_(std::random_device{}()) {
    resize(viewport);
    setParticleCount(count);
}

void ParticleSystem::resize(sf::Vector2u viewport) {
    viewport_ = viewport;
    const float aspect = viewport_.y == 0u ? 16.0f / 9.0f
                                           : static_cast<float>(viewport_.x) / static_cast<float>(viewport_.y);
    worldHalfX_ = 640.0f;
    worldHalfY_ = std::clamp(worldHalfX_ / std::max(aspect, 0.6f), 340.0f, 540.0f);
    worldHalfZ_ = 470.0f;
}

void ParticleSystem::reset() {
    blooms_.clear();
    for (auto& particle : particles_) respawn(particle, true);
    ++particleRevision_;
}

void ParticleSystem::clearTrails() {
    for (auto& particle : particles_) {
        particle.previous = particle.position;
        particle.historyHead = 0;
        for (auto& sample : particle.history) sample = particle.position;
    }
}

void ParticleSystem::setParticleCount(std::size_t count) {
    count = std::clamp<std::size_t>(count, 1000, 12000);
    if (count == particles_.size()) return;

    const std::size_t oldSize = particles_.size();
    particles_.resize(count);
    for (std::size_t i = oldSize; i < particles_.size(); ++i) respawn(particles_[i], true);
    trailVertices_.resize(particles_.size() * (TrailSamples - 1) * 2);
    glowVertices_.resize(particles_.size() * 12);
    headVertices_.resize(particles_.size() * 4);
    particleProjectionCache_.resize(particles_.size());
    ++particleRevision_;
}

void ParticleSystem::setPalette(std::size_t index) {
    paletteIndex_ = index % palettes_.size();
}

void ParticleSystem::setParticleScale(float scale) {
    particleScale_ = std::clamp(scale, 0.6f, 2.2f);
}

void ParticleSystem::setForceStrength(float strength) {
    forceStrength_ = std::clamp(strength, 0.45f, 2.2f);
}

void ParticleSystem::setTrailPersistence(float persistence) {
    trailPersistence_ = std::clamp(persistence, 0.10f, 0.95f);
}

void ParticleSystem::setFlowMode(FlowMode mode) {
    if (flowMode_ == mode) return;
    flowMode_ = mode;
    reset();
}

void ParticleSystem::addNode(const sf::Vector3f& position, NodeType type) {
    if (nodes_.size() >= 12) nodes_.erase(nodes_.begin());
    std::uniform_real_distribution<float> phase(0.0f, Pi * 2.0f);
    nodes_.push_back({position, type, 245.0f, phase(rng_)});
}

int ParticleSystem::findNearestNodeScreen(sf::Vector2f screenPosition, const Camera3D& camera,
                                          float maxDistance) const {
    int nearest = -1;
    float bestDistanceSquared = maxDistance * maxDistance;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const Projection3D projected = camera.project(nodes_[i].position);
        if (!projected.visible) continue;
        const float dx = projected.screen.x - screenPosition.x;
        const float dy = projected.screen.y - screenPosition.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            nearest = static_cast<int>(i);
        }
    }
    return nearest;
}

void ParticleSystem::moveNode(std::size_t index, const sf::Vector3f& position) {
    if (index < nodes_.size()) nodes_[index].position = position;
}

void ParticleSystem::removeNode(std::size_t index) {
    if (index < nodes_.size()) nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(index));
}

void ParticleSystem::clearNodes() {
    nodes_.clear();
}

void ParticleSystem::respawn(Particle& particle, bool anywhere) {
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> signedUnit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> angle(0.0f, Pi * 2.0f);

    const float u = unit(rng_);
    const float a = angle(rng_);
    particle.phase = angle(rng_);
    particle.energy = unit(rng_);
    particle.comet = unit(rng_) < 0.018f;

    if (flowMode_ == FlowMode::BinaryGalaxy) {
        const float side = unit(rng_) < 0.5f ? -1.0f : 1.0f;
        const sf::Vector3f center{side * 220.0f, side * 25.0f, -side * 90.0f};
        const float radius = 42.0f + std::sqrt(u) * 350.0f;
        particle.position = {center.x + std::cos(a) * radius,
                             center.y + signedUnit(rng_) * (26.0f + radius * 0.13f),
                             center.z + std::sin(a) * radius};
        const sf::Vector3f tangent{-std::sin(a), signedUnit(rng_) * 0.08f, std::cos(a)};
        particle.velocity = mul3(tangent, 24.0f + unit(rng_) * 36.0f);
    } else if (flowMode_ == FlowMode::NebulaRiver) {
        const float x = signedUnit(rng_) * worldHalfX_;
        const float z = signedUnit(rng_) * worldHalfZ_;
        const float centerY = std::sin(x * 0.008f + z * 0.003f) * 120.0f;
        particle.position = {x, centerY + signedUnit(rng_) * 105.0f, z};
        particle.velocity = {36.0f + unit(rng_) * 28.0f,
                             signedUnit(rng_) * 10.0f,
                             signedUnit(rng_) * 13.0f};
    } else if (flowMode_ == FlowMode::SolarBloom) {
        const float polar = std::acos(1.0f - 2.0f * unit(rng_));
        const float radius = 55.0f + std::cbrt(u) * 465.0f;
        const sf::Vector3f direction{
            std::sin(polar) * std::cos(a),
            std::cos(polar),
            std::sin(polar) * std::sin(a)
        };
        particle.position = mul3(direction, radius);
        particle.velocity = mul3(direction, 18.0f + unit(rng_) * 34.0f);
    } else if (flowMode_ == FlowMode::AuroraDrift) {
        const float x = signedUnit(rng_) * worldHalfX_;
        const float y = signedUnit(rng_) * worldHalfY_;
        const float sheet = std::sin(x * 0.010f + particle.phase) * 175.0f;
        particle.position = {x, y, sheet + signedUnit(rng_) * 85.0f};
        particle.velocity = {signedUnit(rng_) * 9.0f,
                             24.0f + unit(rng_) * 26.0f,
                             signedUnit(rng_) * 8.0f};
    } else {
        const float x = signedUnit(rng_) * worldHalfX_;
        const float z = signedUnit(rng_) * worldHalfZ_;
        const float filament = std::sin(x * 0.0105f + particle.phase) * 105.0f +
                               std::cos(z * 0.011f) * 82.0f;
        particle.position = {x, filament + signedUnit(rng_) * 95.0f, z};
        particle.velocity = {signedUnit(rng_) * 16.0f,
                             signedUnit(rng_) * 16.0f,
                             signedUnit(rng_) * 16.0f};
    }

    if (!anywhere) {
        particle.position.x = std::clamp(particle.position.x, -worldHalfX_, worldHalfX_);
        particle.position.y = std::clamp(particle.position.y, -worldHalfY_, worldHalfY_);
        particle.position.z = std::clamp(particle.position.z, -worldHalfZ_, worldHalfZ_);
    }

    if (particle.comet) particle.velocity = mul3(particle.velocity, 2.1f);
    particle.previous = particle.position;
    particle.historyHead = 0;
    for (auto& sample : particle.history) sample = particle.position;
}

sf::Vector3f ParticleSystem::flowAt(const sf::Vector3f& p, float elapsed, float phase) const {
    if (flowMode_ == FlowMode::BinaryGalaxy) {
        const sf::Vector3f c1{-220.0f, -25.0f, 90.0f};
        const sf::Vector3f c2{220.0f, 25.0f, -90.0f};
        const sf::Vector3f d1 = sub3(c1, p);
        const sf::Vector3f d2 = sub3(c2, p);
        const float d1Squared = lengthSquared3(d1);
        const float d2Squared = lengthSquared3(d2);
        const sf::Vector3f delta = d1Squared < d2Squared ? d1 : d2;
        const float distance = std::sqrt(std::max(d1Squared < d2Squared ? d1Squared : d2Squared, 0.000001f));
        const sf::Vector3f radial = mul3(delta, 1.0f / distance);
        sf::Vector3f tangent = normalized3(cross3({0.0f, 1.0f, 0.0f}, radial));
        if (lengthSquared3(tangent) < 0.0001f) tangent = {1.0f, 0.0f, 0.0f};
        const float arm = std::sin(std::atan2(delta.z, delta.x) * 3.0f - distance * 0.019f + elapsed * 0.55f);
        return normalized3(add3(mul3(tangent, 0.92f + arm * 0.18f),
                                add3(mul3(radial, 0.28f + arm * 0.08f),
                                     sf::Vector3f{0.0f, std::sin(elapsed * 0.42f + phase) * 0.16f, 0.0f})));
    }

    if (flowMode_ == FlowMode::NebulaRiver) {
        const float waveY = std::sin(p.x * 0.007f + p.z * 0.004f + elapsed * 0.32f + phase * 0.15f);
        const float waveZ = std::cos(p.x * 0.005f - p.y * 0.006f - elapsed * 0.24f);
        return normalized3({1.15f, waveY * 0.78f, waveZ * 0.62f});
    }

    if (flowMode_ == FlowMode::SolarBloom) {
        const float distance = std::max(std::sqrt(lengthSquared3(p)), 1.0f);
        const sf::Vector3f radial = mul3(p, 1.0f / distance);
        const sf::Vector3f tangent = normalized3(cross3({0.0f, 1.0f, 0.0f}, radial));
        const float pulse = std::sin(elapsed * 1.05f - distance * 0.018f + phase * 0.09f);
        return normalized3(add3(mul3(radial, 0.74f + pulse * 0.52f),
                                mul3(tangent, 0.34f + std::cos(elapsed * 0.35f) * 0.12f)));
    }

    if (flowMode_ == FlowMode::AuroraDrift) {
        const float wave = std::sin(p.x * 0.011f + elapsed * 0.44f + phase * 0.10f);
        const float fold = std::cos(p.y * 0.010f - elapsed * 0.26f + p.z * 0.003f);
        return normalized3({wave * 0.52f, 1.08f, fold * 0.74f});
    }

    const float a = std::sin(p.y * 0.010f + elapsed * 0.31f + phase * 0.14f);
    const float b = std::cos(p.x * 0.009f - elapsed * 0.27f - p.z * 0.004f);
    const float c = std::sin(p.z * 0.011f + p.x * 0.003f + elapsed * 0.22f);
    return normalized3({b + c * 0.34f, a - c * 0.28f, c + a * 0.31f});
}

sf::Color ParticleSystem::particleColor(const Particle& particle, float elapsed) const {
    const auto& colors = palette().colors;
    const float speed = std::clamp(length3(particle.velocity) / 235.0f, 0.0f, 1.0f);
    const float depthDrift = (particle.position.z / std::max(worldHalfZ_, 1.0f) + 1.0f) * 0.12f;
    const float drift = elapsed * (0.032f + particle.energy * 0.024f) +
                        particle.position.x * 0.00016f + depthDrift;
    const float value = std::fmod(std::abs(particle.energy * 0.56f + speed * 0.69f + drift), 1.0f) * 3.0f;
    const std::size_t first = std::min<std::size_t>(static_cast<std::size_t>(value), 2u);
    const std::size_t second = std::min<std::size_t>(first + 1, colors.size() - 1);
    sf::Color color = mixColor(colors[first], colors[second], value - static_cast<float>(first));
    color.a = static_cast<sf::Uint8>(105.0f + speed * 150.0f);
    return color;
}

bool ParticleSystem::outsideWorld(const sf::Vector3f& p) const {
    constexpr float margin = 130.0f;
    return p.x < -worldHalfX_ - margin || p.x > worldHalfX_ + margin ||
           p.y < -worldHalfY_ - margin || p.y > worldHalfY_ + margin ||
           p.z < -worldHalfZ_ - margin || p.z > worldHalfZ_ + margin;
}

void ParticleSystem::ensureProjectionCache(const Camera3D& camera) const {
    if (projectionCameraRevision_ == camera.revision() &&
        projectionParticleRevision_ == particleRevision_ &&
        particleProjectionCache_.size() == particles_.size()) {
        return;
    }

    particleProjectionCache_.resize(particles_.size());
    for (std::size_t i = 0; i < particles_.size(); ++i)
        particleProjectionCache_[i] = camera.project(particles_[i].position);

    projectionCameraRevision_ = camera.revision();
    projectionParticleRevision_ = particleRevision_;
}

void ParticleSystem::update(float dt, float elapsed, const sf::Vector3f& mouseWorld,
                            ToolMode tool, float radius) {
    dt = std::min(dt, 0.033f);

    for (auto bloomIt = blooms_.begin(); bloomIt != blooms_.end();) {
        bloomIt->age += dt;
        if (bloomIt->age >= bloomIt->lifetime) bloomIt = blooms_.erase(bloomIt);
        else ++bloomIt;
    }

    struct BloomFrame {
        sf::Vector3f origin;
        float ringRadius;
        float lowerSquared;
        float upperSquared;
        float fade;
    };
    std::vector<BloomFrame> bloomFrames;
    bloomFrames.reserve(blooms_.size());
    for (const BloomRing& bloomRing : blooms_) {
        const float ringRadius = 38.0f + bloomRing.age * 520.0f;
        const float lower = std::max(0.0f, ringRadius - 58.0f);
        const float upper = ringRadius + 58.0f;
        bloomFrames.push_back({bloomRing.origin, ringRadius, lower * lower, upper * upper,
                               1.0f - bloomRing.age / bloomRing.lifetime});
    }

    const float damping = std::pow(0.987f, dt * 60.0f);
    const float radiusSquared = radius * radius;
    constexpr float SpeedLimit = 310.0f;
    constexpr float SpeedLimitSquared = SpeedLimit * SpeedLimit;
    constexpr float CenterLinearFactor = 0.0032f;
    constexpr float CenterCap = 2.6f;
    constexpr float CenterCapDistance = CenterCap / CenterLinearFactor;
    constexpr float CenterCapDistanceSquared = CenterCapDistance * CenterCapDistance;

    for (Particle& particle : particles_) {
        particle.previous = particle.position;

        const float breathingFlow = 38.0f + std::sin(elapsed * 0.68f + particle.phase) * 8.0f;
        sf::Vector3f acceleration = mul3(flowAt(particle.position, elapsed, particle.phase), breathingFlow);

        if (flowMode_ == FlowMode::CosmicWeb) {
            const float targetY = std::sin(particle.position.x * 0.0105f + particle.phase) * 105.0f +
                                  std::cos(particle.position.z * 0.011f) * 82.0f;
            acceleration.y += (targetY - particle.position.y) * 0.032f;
        } else if (flowMode_ == FlowMode::AuroraDrift) {
            const float targetZ = std::sin(particle.position.x * 0.010f + elapsed * 0.35f + particle.phase) * 175.0f;
            acceleration.z += (targetZ - particle.position.z) * 0.036f;
        } else {
            const sf::Vector3f toCenter = mul3(particle.position, -1.0f);
            const float distanceSquared = lengthSquared3(toCenter);
            if (distanceSquared <= CenterCapDistanceSquared) {
                acceleration = add3(acceleration, mul3(toCenter, CenterLinearFactor));
            } else {
                const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                acceleration = add3(acceleration, mul3(toCenter, CenterCap * inverseDistance));
            }
        }

        if (tool != ToolMode::None) {
            const sf::Vector3f towardMouse = sub3(mouseWorld, particle.position);
            const float distanceSquared = lengthSquared3(towardMouse);
            if (distanceSquared < radiusSquared && distanceSquared > 1.0f) {
                const float distance = std::sqrt(distanceSquared);
                const float falloff = 1.0f - distance / radius;
                const float force = 780.0f * falloff * falloff * forceStrength_;
                const sf::Vector3f direction = mul3(towardMouse, 1.0f / distance);
                if (tool == ToolMode::Attract) acceleration = add3(acceleration, mul3(direction, force));
                if (tool == ToolMode::Repel) acceleration = add3(acceleration, mul3(direction, -force * 1.15f));
                if (tool == ToolMode::Vortex) {
                    sf::Vector3f tangent = normalized3(cross3({0.0f, 0.0f, 1.0f}, direction));
                    if (lengthSquared3(tangent) < 0.0001f)
                        tangent = normalized3(cross3({0.0f, 1.0f, 0.0f}, direction));
                    acceleration = add3(acceleration, mul3(tangent, force * 1.12f));
                    acceleration = add3(acceleration, mul3(direction, force * 0.10f));
                }
            }
        }

        for (const ForceNode& node : nodes_) {
            const sf::Vector3f towardNode = sub3(node.position, particle.position);
            const float distanceSquared = lengthSquared3(towardNode);
            const float nodeRadiusSquared = node.radius * node.radius;
            if (distanceSquared < nodeRadiusSquared && distanceSquared > 1.0f) {
                const float distance = std::sqrt(distanceSquared);
                const float falloff = 1.0f - distance / node.radius;
                const float force = 690.0f * falloff * falloff * forceStrength_;
                const sf::Vector3f direction = mul3(towardNode, 1.0f / distance);
                if (node.type == NodeType::Attractor) acceleration = add3(acceleration, mul3(direction, force));
                if (node.type == NodeType::Repulsor) acceleration = add3(acceleration, mul3(direction, -force * 1.12f));
                if (node.type == NodeType::Vortex) {
                    const sf::Vector3f tangent = normalized3(cross3({0.0f, 1.0f, 0.0f}, direction));
                    acceleration = add3(acceleration, mul3(tangent, force * 1.08f));
                    acceleration = add3(acceleration, mul3(direction, force * 0.08f));
                }
            }
        }

        for (const BloomFrame& bloom : bloomFrames) {
            const sf::Vector3f delta = sub3(particle.position, bloom.origin);
            const float distanceSquared = lengthSquared3(delta);
            if (distanceSquared <= 1.0f || distanceSquared < bloom.lowerSquared ||
                distanceSquared > bloom.upperSquared) {
                continue;
            }
            const float distance = std::sqrt(distanceSquared);
            const float band = std::abs(distance - bloom.ringRadius);
            const float strength = (1.0f - band / 58.0f) * bloom.fade;
            acceleration = add3(acceleration, mul3(delta, strength * 980.0f / distance));
        }

        particle.velocity = add3(particle.velocity, mul3(acceleration, dt));
        const float speedSquared = lengthSquared3(particle.velocity);
        if (speedSquared > SpeedLimitSquared) {
            particle.velocity = mul3(particle.velocity, SpeedLimit / std::sqrt(speedSquared));
        }
        particle.velocity = mul3(particle.velocity, damping);
        particle.position = add3(particle.position, mul3(particle.velocity, dt));

        if (outsideWorld(particle.position)) {
            respawn(particle, false);
        } else {
            particle.historyHead = (particle.historyHead + TrailSamples - 1) % TrailSamples;
            particle.history[particle.historyHead] = particle.position;
        }
    }

    ++particleRevision_;
}

void ParticleSystem::bloom(const sf::Vector3f& origin) {
    blooms_.push_back({origin, 0.0f, 1.0f});
}

void ParticleSystem::drawTrails(sf::RenderTarget& target, const Camera3D& camera) const {
    const std::size_t activeSamples = std::clamp<std::size_t>(
        3u + static_cast<std::size_t>(trailPersistence_ * static_cast<float>(TrailSamples - 3)),
        3u, TrailSamples);
    std::size_t vertex = 0;
    ensureProjectionCache(camera);

    for (std::size_t particleIndex = 0; particleIndex < particles_.size(); ++particleIndex) {
        const Particle& particle = particles_[particleIndex];
        const sf::Color base = particleColor(particle, 0.0f);
        std::array<Projection3D, TrailSamples> projectedHistory{};
        projectedHistory[0] = particleProjectionCache_[particleIndex];
        for (std::size_t sample = 1; sample < activeSamples; ++sample) {
            const std::size_t historyIndex = (particle.historyHead + sample) % TrailSamples;
            projectedHistory[sample] = camera.project(particle.history[historyIndex]);
        }

        for (std::size_t sample = 0; sample + 1 < activeSamples; ++sample) {
            const Projection3D& a = projectedHistory[sample];
            const Projection3D& b = projectedHistory[sample + 1];
            sf::Color colorA = base;
            sf::Color colorB = base;
            const float ageA = 1.0f - static_cast<float>(sample) / static_cast<float>(activeSamples);
            const float ageB = 1.0f - static_cast<float>(sample + 1) / static_cast<float>(activeSamples);
            const float depthFactor = a.visible ? std::clamp(1000.0f / a.depth, 0.42f, 1.45f) : 0.0f;
            colorA.a = scaledAlpha(base.a, ageA * ageA * depthFactor * (particle.comet ? 0.48f : 0.28f));
            colorB.a = scaledAlpha(base.a, ageB * ageB * depthFactor * (particle.comet ? 0.38f : 0.20f));
            if (!a.visible || !b.visible) colorA.a = colorB.a = 0;
            trailVertices_[vertex++] = {a.screen, colorA};
            trailVertices_[vertex++] = {b.screen, colorB};
        }
    }
    trailVertices_.resize(vertex);
    target.draw(trailVertices_, sf::BlendAdd);
    trailVertices_.resize(particles_.size() * (TrailSamples - 1) * 2);
}

void ParticleSystem::drawParticles(sf::RenderTarget& target, const Camera3D& camera) const {
    ensureProjectionCache(camera);
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        const Particle& particle = particles_[i];
        const Projection3D& projected = particleProjectionCache_[i];
        const Projection3D previous = camera.project(particle.previous);
        const std::size_t glowBase = i * 12;
        const std::size_t quadBase = i * 4;
        const sf::Color transparent{0, 0, 0, 0};

        if (!projected.visible || !previous.visible) {
            for (std::size_t j = 0; j < 12; ++j) glowVertices_[glowBase + j] = {projected.screen, transparent};
            for (std::size_t j = 0; j < 4; ++j) headVertices_[quadBase + j] = {projected.screen, transparent};
            continue;
        }

        const float speed = std::sqrt(lengthSquared3(particle.velocity));
        const float depthFactor = std::clamp(1020.0f / projected.depth, 0.52f, 2.25f);
        const float pulse = 0.86f + std::sin(particle.phase + particle.position.x * 0.004f) * 0.14f;
        float headSize = (1.05f + particle.energy * 1.22f + std::min(speed / 170.0f, 1.35f)) *
                         pulse * particleScale_ * depthFactor;
        if (particle.comet) headSize *= 1.85f;

        sf::Color color = particleColor(particle, particle.phase * 0.1f);
        color.a = scaledAlpha(color.a, std::clamp(depthFactor, 0.55f, 1.25f));
        sf::Color glowCenter = color;
        glowCenter.a = static_cast<sf::Uint8>(particle.comet ? 84 : 42);
        sf::Color glowEdge = color;
        glowEdge.a = 0;

        const float glowSize = headSize * (particle.comet ? 4.8f : 3.7f);
        const sf::Vector2f top{projected.screen.x, projected.screen.y - glowSize};
        const sf::Vector2f right{projected.screen.x + glowSize, projected.screen.y};
        const sf::Vector2f bottom{projected.screen.x, projected.screen.y + glowSize};
        const sf::Vector2f left{projected.screen.x - glowSize, projected.screen.y};
        const sf::Vector2f points[4] = {top, right, bottom, left};
        for (std::size_t triangle = 0; triangle < 4; ++triangle) {
            const std::size_t vertex = glowBase + triangle * 3;
            glowVertices_[vertex] = {projected.screen, glowCenter};
            glowVertices_[vertex + 1] = {points[triangle], glowEdge};
            glowVertices_[vertex + 2] = {points[(triangle + 1) % 4], glowEdge};
        }

        sf::Vector2f motion{projected.screen.x - previous.screen.x,
                            projected.screen.y - previous.screen.y};
        const sf::Vector2f direction = normalized2(motion);
        const sf::Vector2f perpendicular{-direction.y, direction.x};
        const float stretch = 1.0f + std::clamp(speed / 250.0f, 0.0f, 1.0f) * 1.8f +
                              (particle.comet ? 1.3f : 0.0f);
        const float halfLength = headSize * stretch;
        const float halfWidth = std::max(0.75f, headSize * 0.84f);
        const sf::Vector2f front{projected.screen.x + direction.x * halfLength,
                                 projected.screen.y + direction.y * halfLength};
        const sf::Vector2f back{projected.screen.x - direction.x * halfLength,
                                projected.screen.y - direction.y * halfLength};

        sf::Color core = mixColor(color, sf::Color(255, 255, 255), particle.comet ? 0.76f : 0.33f);
        core.a = particle.comet ? 255 : 232;
        headVertices_[quadBase] = {{front.x + perpendicular.x * halfWidth,
                                    front.y + perpendicular.y * halfWidth}, core};
        headVertices_[quadBase + 1] = {{front.x - perpendicular.x * halfWidth,
                                        front.y - perpendicular.y * halfWidth}, core};
        headVertices_[quadBase + 2] = {{back.x - perpendicular.x * halfWidth,
                                        back.y - perpendicular.y * halfWidth}, core};
        headVertices_[quadBase + 3] = {{back.x + perpendicular.x * halfWidth,
                                        back.y + perpendicular.y * halfWidth}, core};
    }

    target.draw(glowVertices_, sf::BlendAdd);
    target.draw(headVertices_, sf::BlendAdd);
}

void ParticleSystem::drawConnections(sf::RenderTarget& target, const Camera3D& camera,
                                     const sf::Vector3f& mouseWorld, ToolMode tool, float radius) const {
    connectionVertices_.clear();
    if (tool == ToolMode::None) return;

    const Projection3D mouseProjection = camera.project(mouseWorld);
    if (!mouseProjection.visible) return;
    ensureProjectionCache(camera);

    const float radiusSquared = radius * radius;
    std::size_t connections = 0;
    for (std::size_t i = 0; i < particles_.size() && connections < 150; i += 7) {
        const Particle& particle = particles_[i];
        const sf::Vector3f delta = sub3(mouseWorld, particle.position);
        const float distanceSquared = lengthSquared3(delta);
        if (distanceSquared >= radiusSquared) continue;
        const Projection3D& projected = particleProjectionCache_[i];
        if (!projected.visible) continue;

        const float distance = std::sqrt(distanceSquared);
        sf::Color lineColor = particleColor(particle, 0.0f);
        lineColor.a = static_cast<sf::Uint8>((1.0f - distance / radius) * 48.0f);
        sf::Color cursorColor = lineColor;
        cursorColor.a = 3;
        connectionVertices_.append({projected.screen, lineColor});
        connectionVertices_.append({mouseProjection.screen, cursorColor});
        ++connections;
    }
    target.draw(connectionVertices_, sf::BlendAdd);
}

void ParticleSystem::drawNodes(sf::RenderTarget& target, const Camera3D& camera, float elapsed) const {
    for (const ForceNode& node : nodes_) {
        const Projection3D projected = camera.project(node.position);
        if (!projected.visible) continue;
        const Projection3D edge = camera.project(add3(node.position, {node.radius, 0.0f, 0.0f}));
        const float screenRadius = edge.visible ? std::clamp(length2({edge.screen.x - projected.screen.x,
                                                                      edge.screen.y - projected.screen.y}),
                                                              18.0f, 230.0f)
                                                : 60.0f;
        const float pulse = 0.5f + 0.5f * std::sin(elapsed * 2.2f + node.phase);
        const std::size_t colorIndex = node.type == NodeType::Attractor ? 0 :
                                       node.type == NodeType::Repulsor ? 3 : 2;
        sf::Color color = palette().colors[colorIndex];

        sf::CircleShape range(screenRadius);
        range.setOrigin(screenRadius, screenRadius);
        range.setPosition(projected.screen);
        range.setFillColor(sf::Color::Transparent);
        color.a = static_cast<sf::Uint8>(17.0f + pulse * 18.0f);
        range.setOutlineColor(color);
        range.setOutlineThickness(1.0f);
        target.draw(range, sf::BlendAdd);

        const float haloRadius = (16.0f + pulse * 6.0f) * std::clamp(950.0f / projected.depth, 0.65f, 1.55f);
        sf::CircleShape halo(haloRadius);
        halo.setOrigin(haloRadius, haloRadius);
        halo.setPosition(projected.screen);
        color.a = 38;
        halo.setFillColor(color);
        target.draw(halo, sf::BlendAdd);

        const float coreRadius = (node.type == NodeType::Vortex ? 6.5f : 5.5f) *
                                 std::clamp(950.0f / projected.depth, 0.65f, 1.55f);
        sf::CircleShape core(coreRadius);
        core.setOrigin(coreRadius, coreRadius);
        core.setPosition(projected.screen);
        color.a = 238;
        core.setFillColor(color);
        core.setOutlineThickness(1.5f);
        core.setOutlineColor({235, 250, 255, 210});
        target.draw(core, sf::BlendAdd);
    }
}

void ParticleSystem::drawBloomRings(sf::RenderTarget& target, const Camera3D& camera) const {
    for (const BloomRing& bloomRing : blooms_) {
        const Projection3D center = camera.project(bloomRing.origin);
        if (!center.visible) continue;
        const float worldRadius = 38.0f + bloomRing.age * 520.0f;
        const Projection3D edge = camera.project(add3(bloomRing.origin, {worldRadius, 0.0f, 0.0f}));
        if (!edge.visible) continue;
        const float screenRadius = length2({edge.screen.x - center.screen.x, edge.screen.y - center.screen.y});
        if (screenRadius < 2.0f) continue;

        const float progress = bloomRing.age / bloomRing.lifetime;
        sf::CircleShape ring(screenRadius);
        ring.setOrigin(screenRadius, screenRadius);
        ring.setPosition(center.screen);
        ring.setFillColor(sf::Color::Transparent);
        sf::Color color = palette().colors[1];
        color.a = static_cast<sf::Uint8>((1.0f - progress) * 105.0f);
        ring.setOutlineColor(color);
        ring.setOutlineThickness(1.5f + (1.0f - progress) * 2.5f);
        target.draw(ring, sf::BlendAdd);
    }
}
