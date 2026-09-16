#pragma once

#include "ParticleSystem.hpp"

#include <SFML/Graphics.hpp>
#include <random>
#include <vector>

class DynamicBackground {
public:
    explicit DynamicBackground(sf::Vector2u bounds);

    void resize(sf::Vector2u bounds);
    void update(float dt, sf::Vector2f mouse);
    void draw(sf::RenderTarget& target, float elapsed, const Palette& palette, ToolMode tool,
              float intensity = 1.0f, bool animated = true) const;

private:
    struct Star {
        sf::Vector2f position;
        float depth = 0.0f;
        float phase = 0.0f;
        float size = 1.0f;
    };

    sf::Vector2u bounds_;
    sf::Vector2f smoothMouse_;
    std::vector<Star> stars_;
    std::mt19937 rng_;

    void regenerateStars();
};
