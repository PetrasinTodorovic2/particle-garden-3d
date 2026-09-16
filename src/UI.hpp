#pragma once

#include "PixelText.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace ui {
inline bool contains(sf::FloatRect rect, sf::Vector2f point) {
    return point.x >= rect.left && point.x <= rect.left + rect.width &&
           point.y >= rect.top && point.y <= rect.top + rect.height;
}

inline float textWidth(const std::string& text, float scale) {
    return static_cast<float>(text.size()) * 6.0f * scale;
}

inline void panel(sf::RenderTarget& target, sf::FloatRect rect, sf::Color accent,
                  sf::Uint8 opacity = 220) {
    sf::RectangleShape shadow({rect.width + 12.0f, rect.height + 12.0f});
    shadow.setPosition(rect.left + 6.0f, rect.top + 7.0f);
    shadow.setFillColor({0, 0, 0, 90});
    target.draw(shadow);

    sf::RectangleShape body({rect.width, rect.height});
    body.setPosition(rect.left, rect.top);
    body.setFillColor({5, 10, 25, opacity});
    body.setOutlineThickness(1.0f);
    accent.a = 75;
    body.setOutlineColor(accent);
    target.draw(body);
}

inline void overlay(sf::RenderTarget& target, sf::Uint8 opacity = 145) {
    sf::RectangleShape shade({static_cast<float>(target.getSize().x),
                              static_cast<float>(target.getSize().y)});
    shade.setFillColor({1, 3, 12, opacity});
    target.draw(shade);
}

inline bool button(sf::RenderTarget& target, sf::FloatRect rect, const std::string& label,
                   sf::Vector2f mouse, sf::Color accent, bool selected = false) {
    const bool hovered = contains(rect, mouse);
    sf::RectangleShape body({rect.width, rect.height});
    body.setPosition(rect.left, rect.top);
    body.setFillColor(selected ? sf::Color(18, 34, 58, 242)
                               : hovered ? sf::Color(14, 28, 49, 238)
                                         : sf::Color(7, 14, 31, 218));
    body.setOutlineThickness(selected ? 2.0f : 1.0f);
    accent.a = static_cast<sf::Uint8>(selected ? 210 : hovered ? 145 : 58);
    body.setOutlineColor(accent);
    target.draw(body);

    if (hovered || selected) {
        sf::RectangleShape marker({4.0f, rect.height});
        marker.setPosition(rect.left, rect.top);
        accent.a = 220;
        marker.setFillColor(accent);
        target.draw(marker, sf::BlendAdd);
    }

    const float scale = 1.5f;
    const float x = rect.left + (rect.width - textWidth(label, scale)) * 0.5f;
    const float y = rect.top + (rect.height - 7.0f * scale) * 0.5f;
    PixelText::draw(target, label, {x, y}, scale,
                    hovered || selected ? sf::Color(235, 248, 255) : sf::Color(146, 174, 202));
    return hovered;
}

inline void slider(sf::RenderTarget& target, sf::FloatRect rect, const std::string& label,
                   float normalized, const std::string& value, sf::Vector2f mouse,
                   sf::Color accent) {
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    PixelText::draw(target, label, {rect.left, rect.top}, 1.25f, {171, 198, 222});
    const float valueX = rect.left + rect.width - textWidth(value, 1.1f);
    PixelText::draw(target, value, {valueX, rect.top + 1.0f}, 1.1f, {225, 241, 250});

    sf::FloatRect track{rect.left, rect.top + 25.0f, rect.width, 8.0f};
    sf::RectangleShape rail({track.width, track.height});
    rail.setPosition(track.left, track.top);
    rail.setFillColor({23, 37, 57, 230});
    target.draw(rail);

    sf::RectangleShape fill({track.width * normalized, track.height});
    fill.setPosition(track.left, track.top);
    accent.a = 175;
    fill.setFillColor(accent);
    target.draw(fill, sf::BlendAdd);

    const bool hovered = contains({track.left - 8.0f, track.top - 8.0f,
                                   track.width + 16.0f, track.height + 16.0f}, mouse);
    sf::CircleShape knob(hovered ? 8.0f : 6.0f);
    knob.setOrigin(knob.getRadius(), knob.getRadius());
    knob.setPosition(track.left + track.width * normalized, track.top + track.height * 0.5f);
    knob.setFillColor(hovered ? sf::Color(245, 252, 255) : sf::Color(191, 221, 235));
    target.draw(knob, sf::BlendAdd);
}

inline bool toggle(sf::RenderTarget& target, sf::FloatRect rect, const std::string& label,
                   bool enabled, sf::Vector2f mouse, sf::Color accent) {
    const bool hovered = contains(rect, mouse);
    PixelText::draw(target, label, {rect.left, rect.top + 8.0f}, 1.25f,
                    hovered ? sf::Color(222, 241, 250) : sf::Color(171, 198, 222));

    sf::FloatRect switchRect{rect.left + rect.width - 64.0f, rect.top, 64.0f, 30.0f};
    sf::RectangleShape track({switchRect.width, switchRect.height});
    track.setPosition(switchRect.left, switchRect.top);
    track.setFillColor(enabled ? sf::Color(20, 53, 64, 235) : sf::Color(27, 34, 49, 235));
    track.setOutlineThickness(1.0f);
    accent.a = enabled ? 170 : 45;
    track.setOutlineColor(accent);
    target.draw(track);

    sf::CircleShape knob(10.0f);
    knob.setOrigin(10.0f, 10.0f);
    knob.setPosition(switchRect.left + (enabled ? 48.0f : 16.0f), switchRect.top + 15.0f);
    knob.setFillColor(enabled ? sf::Color(210, 251, 245) : sf::Color(119, 137, 158));
    target.draw(knob, enabled ? sf::BlendAdd : sf::BlendAlpha);
    return hovered;
}

inline std::string fixed(float value, int decimals = 1) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    return stream.str();
}
}
