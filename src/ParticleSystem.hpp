#pragma once

#include "Camera3D.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

enum class ToolMode {
    None,
    Attract,
    Repel,
    Vortex
};

enum class FlowMode {
    BinaryGalaxy,
    NebulaRiver,
    SolarBloom,
    CosmicWeb,
    AuroraDrift
};

enum class NodeType {
    Attractor,
    Repulsor,
    Vortex
};

const char* flowModeName(FlowMode mode);
const char* flowModeSubtitle(FlowMode mode);

struct Palette {
    const char* name;
    std::array<sf::Color, 4> colors;
};

class ParticleSystem {
public:
    explicit ParticleSystem(sf::Vector2u viewport, std::size_t count = 5200);

    void resize(sf::Vector2u viewport);
    void reset();
    void clearTrails();
    void update(float dt, float elapsed, const sf::Vector3f& mouseWorld, ToolMode tool, float radius);
    void bloom(const sf::Vector3f& origin);
    void setPalette(std::size_t index);
    void setParticleCount(std::size_t count);
    void setParticleScale(float scale);
    void setForceStrength(float strength);
    void setTrailPersistence(float persistence);
    void setFlowMode(FlowMode mode);

    void addNode(const sf::Vector3f& position, NodeType type);
    [[nodiscard]] int findNearestNodeScreen(sf::Vector2f screenPosition, const Camera3D& camera,
                                            float maxDistance = 28.0f) const;
    void moveNode(std::size_t index, const sf::Vector3f& position);
    void removeNode(std::size_t index);
    void clearNodes();

    void drawTrails(sf::RenderTarget& target, const Camera3D& camera) const;
    void drawParticles(sf::RenderTarget& target, const Camera3D& camera) const;
    void drawConnections(sf::RenderTarget& target, const Camera3D& camera,
                         const sf::Vector3f& mouseWorld, ToolMode tool, float radius) const;
    void drawBloomRings(sf::RenderTarget& target, const Camera3D& camera) const;
    void drawNodes(sf::RenderTarget& target, const Camera3D& camera, float elapsed) const;

    [[nodiscard]] std::size_t particleCount() const { return particles_.size(); }
    [[nodiscard]] std::size_t paletteIndex() const { return paletteIndex_; }
    [[nodiscard]] const Palette& palette() const { return palettes_[paletteIndex_]; }
    [[nodiscard]] FlowMode flowMode() const { return flowMode_; }
    [[nodiscard]] std::size_t nodeCount() const { return nodes_.size(); }

private:
    static constexpr std::size_t TrailSamples = 14;

    struct Particle {
        sf::Vector3f position;
        sf::Vector3f previous;
        sf::Vector3f velocity;
        std::array<sf::Vector3f, TrailSamples> history{};
        std::size_t historyHead = 0;
        float phase = 0.0f;
        float energy = 0.0f;
        bool comet = false;
    };

    struct BloomRing {
        sf::Vector3f origin;
        float age = 0.0f;
        float lifetime = 1.0f;
    };

    struct ForceNode {
        sf::Vector3f position;
        NodeType type = NodeType::Attractor;
        float radius = 245.0f;
        float phase = 0.0f;
    };

    sf::Vector2u viewport_;
    std::vector<Particle> particles_;
    std::vector<BloomRing> blooms_;
    std::vector<ForceNode> nodes_;
    mutable sf::VertexArray trailVertices_{sf::Lines};
    mutable sf::VertexArray glowVertices_{sf::Triangles};
    mutable sf::VertexArray headVertices_{sf::Quads};
    mutable sf::VertexArray connectionVertices_{sf::Lines};
    mutable std::vector<Projection3D> particleProjectionCache_;
    mutable std::uint64_t projectionCameraRevision_ = 0;
    mutable std::uint64_t projectionParticleRevision_ = 0;
    std::mt19937 rng_;
    std::size_t paletteIndex_ = 0;
    float particleScale_ = 1.0f;
    float forceStrength_ = 1.0f;
    float trailPersistence_ = 0.68f;
    FlowMode flowMode_ = FlowMode::CosmicWeb;
    float worldHalfX_ = 650.0f;
    float worldHalfY_ = 370.0f;
    float worldHalfZ_ = 470.0f;
    std::uint64_t particleRevision_ = 1;

    static const std::array<Palette, 6> palettes_;

    void respawn(Particle& particle, bool anywhere);
    [[nodiscard]] sf::Vector3f flowAt(const sf::Vector3f& position, float elapsed, float phase) const;
    [[nodiscard]] sf::Color particleColor(const Particle& particle, float elapsed) const;
    [[nodiscard]] bool outsideWorld(const sf::Vector3f& position) const;
    void ensureProjectionCache(const Camera3D& camera) const;
};
