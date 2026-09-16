#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <cctype>
#include <string>
#include <unordered_map>

class PixelText {
public:
    static void draw(sf::RenderTarget& target, const std::string& text, sf::Vector2f position,
                     float scale, sf::Color color) {
        sf::VertexArray pixels(sf::Quads);
        float cursorX = position.x;
        float cursorY = position.y;

        for (char raw : text) {
            if (raw == '\n') {
                cursorX = position.x;
                cursorY += 8.0f * scale;
                continue;
            }

            const char character = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
            const auto it = glyphs().find(character);
            const auto& rows = it != glyphs().end() ? it->second : glyphs().at('?');

            for (std::size_t y = 0; y < rows.size(); ++y) {
                for (int x = 0; x < 5; ++x) {
                    if ((rows[y] & (1 << (4 - x))) == 0) continue;
                    const float left = cursorX + static_cast<float>(x) * scale;
                    const float top = cursorY + static_cast<float>(y) * scale;
                    pixels.append({{left, top}, color});
                    pixels.append({{left + scale, top}, color});
                    pixels.append({{left + scale, top + scale}, color});
                    pixels.append({{left, top + scale}, color});
                }
            }
            cursorX += 6.0f * scale;
        }
        target.draw(pixels);
    }

private:
    using Glyph = std::array<unsigned char, 7>;

    static const std::unordered_map<char, Glyph>& glyphs() {
        static const std::unordered_map<char, Glyph> data = {
            {' ', {0,0,0,0,0,0,0}}, {'?', {14,17,1,2,4,0,4}},
            {'A', {14,17,17,31,17,17,17}}, {'B', {30,17,17,30,17,17,30}},
            {'C', {14,17,16,16,16,17,14}}, {'D', {30,17,17,17,17,17,30}},
            {'E', {31,16,16,30,16,16,31}}, {'F', {31,16,16,30,16,16,16}},
            {'G', {14,17,16,23,17,17,15}}, {'H', {17,17,17,31,17,17,17}},
            {'I', {14,4,4,4,4,4,14}}, {'J', {7,2,2,2,18,18,12}},
            {'K', {17,18,20,24,20,18,17}}, {'L', {16,16,16,16,16,16,31}},
            {'M', {17,27,21,21,17,17,17}}, {'N', {17,25,21,19,17,17,17}},
            {'O', {14,17,17,17,17,17,14}}, {'P', {30,17,17,30,16,16,16}},
            {'Q', {14,17,17,17,21,18,13}}, {'R', {30,17,17,30,20,18,17}},
            {'S', {15,16,16,14,1,1,30}}, {'T', {31,4,4,4,4,4,4}},
            {'U', {17,17,17,17,17,17,14}}, {'V', {17,17,17,17,17,10,4}},
            {'W', {17,17,17,21,21,21,10}}, {'X', {17,17,10,4,10,17,17}},
            {'Y', {17,17,10,4,4,4,4}}, {'Z', {31,1,2,4,8,16,31}},
            {'0', {14,17,19,21,25,17,14}}, {'1', {4,12,4,4,4,4,14}},
            {'2', {14,17,1,2,4,8,31}}, {'3', {30,1,1,14,1,1,30}},
            {'4', {2,6,10,18,31,2,2}}, {'5', {31,16,16,30,1,1,30}},
            {'6', {14,16,16,30,17,17,14}}, {'7', {31,1,2,4,8,8,8}},
            {'8', {14,17,17,14,17,17,14}}, {'9', {14,17,17,15,1,1,14}},
            {'-', {0,0,0,31,0,0,0}}, {'+', {0,4,4,31,4,4,0}},
            {'/', {1,2,2,4,8,8,16}}, {':', {0,4,4,0,4,4,0}},
            {'.', {0,0,0,0,0,12,12}}, {'[', {14,8,8,8,8,8,14}},
            {']', {14,2,2,2,2,2,14}}, {'|', {4,4,4,4,4,4,4}}
        };
        return data;
    }
};
