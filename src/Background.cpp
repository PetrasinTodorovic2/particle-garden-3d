#include "Background.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;

sf::Color withAlpha(sf::Color color, float alpha) {
    color.a = static_cast<sf::Uint8>(std::clamp(alpha, 0.0f, 255.0f));
    return color;
}

void setQuad(sf::VertexArray& vertices, std::size_t index, sf::Vector2f center,
             float halfSize, sf::Color color) {
    const std::size_t first = index * 4;
    vertices[first] = {{center.x - halfSize, center.y - halfSize}, color};
    vertices[first + 1] = {{center.x + halfSize, center.y - halfSize}, color};
    vertices[first + 2] = {{center.x + halfSize, center.y + halfSize}, color};
    vertices[first + 3] = {{center.x - halfSize, center.y + halfSize}, color};
}

void drawRadialGlow(sf::RenderTarget& target, sf::Vector2f center, float radius,
                    sf::Color color, float alpha) {
    constexpr std::size_t Segments = 64;
    sf::VertexArray fan(sf::TriangleFan, Segments + 2);
    fan[0] = {center, withAlpha(color, alpha)};
    for (std::size_t i = 0; i <= Segments; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(Segments) * Pi * 2.0f;
        fan[i + 1] = {{center.x + std::cos(angle) * radius,
                       center.y + std::sin(angle) * radius},
                      withAlpha(color, 0.0f)};
    }
    target.draw(fan, sf::BlendAdd);
}

void drawRibbon(sf::RenderTarget& target, sf::Vector2u size, float elapsed,
                float baseY, float amplitude, float speed, sf::Color color, float phase,
                float intensity) {
    constexpr std::size_t Points = 72;
    sf::VertexArray upper(sf::TriangleStrip, Points * 2);
    sf::VertexArray lower(sf::TriangleStrip, Points * 2);

    for (std::size_t i = 0; i < Points; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(Points - 1);
        const float x = (t * 1.12f - 0.06f) * static_cast<float>(size.x);
        const float wave = std::sin(t * 8.0f + elapsed * speed + phase) * amplitude +
                           std::sin(t * 18.0f - elapsed * speed * 0.43f + phase) * amplitude * 0.24f;
        const float center = baseY * static_cast<float>(size.y) + wave;
        const float width = 48.0f + 26.0f * (0.5f + 0.5f * std::sin(t * 11.0f + elapsed * 0.31f));
        const float pulse = 0.72f + 0.28f * std::sin(elapsed * 0.55f + phase);
        const sf::Color edge = withAlpha(color, 0.0f);
        const sf::Color middle = withAlpha(color, 18.0f * pulse * intensity);

        upper[i * 2] = {{x, center - width}, edge};
        upper[i * 2 + 1] = {{x, center}, middle};
        lower[i * 2] = {{x, center}, middle};
        lower[i * 2 + 1] = {{x, center + width}, edge};
    }

    target.draw(upper, sf::BlendAdd);
    target.draw(lower, sf::BlendAdd);
}
}

DynamicBackground::DynamicBackground(sf::Vector2u bounds)
    : bounds_(bounds), smoothMouse_(bounds.x * 0.5f, bounds.y * 0.5f),
      rng_(std::random_device{}()) {
    regenerateStars();
}

void DynamicBackground::resize(sf::Vector2u bounds) {
    bounds_ = bounds;
    smoothMouse_ = {bounds.x * 0.5f, bounds.y * 0.5f};
    regenerateStars();
}

void DynamicBackground::regenerateStars() {
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    stars_.resize(190);
    for (auto& star : stars_) {
        star.position = {unit(rng_) * static_cast<float>(bounds_.x),
                         unit(rng_) * static_cast<float>(bounds_.y)};
        star.depth = 0.18f + unit(rng_) * 0.82f;
        star.phase = unit(rng_) * Pi * 2.0f;
        star.size = 0.55f + unit(rng_) * 1.55f;
    }
}

void DynamicBackground::update(float dt, sf::Vector2f mouse) {
    const float response = 1.0f - std::pow(0.002f, std::min(dt, 0.05f));
    smoothMouse_ += (mouse - smoothMouse_) * response;
}

void DynamicBackground::draw(sf::RenderTarget& target, float elapsed,
                             const Palette& palette, ToolMode tool,
                             float intensity, bool animated) const {
    const sf::Vector2u size = target.getSize();
    intensity = std::clamp(intensity, 0.15f, 1.6f);
    const float visualTime = animated ? elapsed : 0.0f;
    const sf::Vector2f center{size.x * 0.5f, size.y * 0.5f};
    const sf::Vector2f parallax = smoothMouse_ - center;

    sf::VertexArray gradient(sf::Quads, 4);
    gradient[0] = {{0.0f, 0.0f}, {3, 7, 18}};
    gradient[1] = {{static_cast<float>(size.x), 0.0f}, {9, 7, 28}};
    gradient[2] = {{static_cast<float>(size.x), static_cast<float>(size.y)}, {3, 15, 29}};
    gradient[3] = {{0.0f, static_cast<float>(size.y)}, {1, 6, 16}};
    target.draw(gradient);

    drawRibbon(target, size, visualTime, 0.28f, 54.0f, 0.23f, palette.colors[1], 0.0f, intensity);
    drawRibbon(target, size, visualTime, 0.68f, 72.0f, -0.17f, palette.colors[2], 2.1f, intensity);
    drawRibbon(target, size, visualTime, 0.48f, 38.0f, 0.31f, palette.colors[0], 4.2f, intensity);

    drawRadialGlow(target,
        {size.x * (0.28f + std::sin(visualTime * 0.13f) * 0.11f),
         size.y * (0.40f + std::cos(visualTime * 0.11f) * 0.12f)},
        std::min(size.x, size.y) * 0.42f, palette.colors[2], 24.0f * intensity);
    drawRadialGlow(target,
        {size.x * (0.72f + std::cos(visualTime * 0.10f) * 0.10f),
         size.y * (0.58f + std::sin(visualTime * 0.15f) * 0.10f)},
        std::min(size.x, size.y) * 0.36f, palette.colors[0], 18.0f * intensity);

    sf::VertexArray stars(sf::Quads, stars_.size() * 4);
    for (std::size_t i = 0; i < stars_.size(); ++i) {
        const auto& star = stars_[i];
        sf::Vector2f position = star.position - parallax * (0.008f + star.depth * 0.018f);
        if (position.x < 0.0f) position.x += static_cast<float>(size.x);
        if (position.x > size.x) position.x -= static_cast<float>(size.x);
        if (position.y < 0.0f) position.y += static_cast<float>(size.y);
        if (position.y > size.y) position.y -= static_cast<float>(size.y);
        const float twinkle = 0.5f + 0.5f * std::sin(visualTime * (0.7f + star.depth) + star.phase);
        sf::Color color = palette.colors[i % palette.colors.size()];
        color.a = static_cast<sf::Uint8>((18.0f + twinkle * 60.0f * star.depth) * intensity);
        setQuad(stars, i, position, star.size * (0.7f + twinkle * 0.45f), color);
    }
    target.draw(stars, sf::BlendAdd);

    const float mouseGlow = tool == ToolMode::None ? 12.0f : 34.0f;
    const float mouseRadius = tool == ToolMode::None ? 150.0f : 245.0f;
    const sf::Color mouseColor = tool == ToolMode::Repel ? palette.colors[3] : palette.colors[0];
    drawRadialGlow(target, smoothMouse_, mouseRadius, mouseColor, mouseGlow * intensity);
}
