#include "AppSettings.hpp"
#include "Background.hpp"
#include "Camera3D.hpp"
#include "ParticleSystem.hpp"
#include "PixelText.hpp"
#include "UI.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
constexpr unsigned InitialWidth = 1280;
constexpr unsigned InitialHeight = 720;
constexpr const char* SettingsFile = "particle_garden.cfg";

enum class Screen { MainMenu, Simulation, Pause, Settings, Gallery, Controls };

struct Layout {
    sf::Vector2u size;

    sf::FloatRect mainButton(std::size_t index) const {
        const float startY = std::max(245.0f, static_cast<float>(size.y) * 0.34f);
        return {72.0f, startY + static_cast<float>(index) * 60.0f, 360.0f, 46.0f};
    }

    sf::FloatRect pauseButton(std::size_t index) const {
        const float width = 360.0f;
        const float x = (static_cast<float>(size.x) - width) * 0.5f;
        const float startY = (static_cast<float>(size.y) - 390.0f) * 0.5f + 82.0f;
        return {x, startY + static_cast<float>(index) * 52.0f, width, 40.0f};
    }

    sf::FloatRect settingsPanel() const {
        const float width = std::min(960.0f, static_cast<float>(size.x) - 36.0f);
        const float height = std::min(620.0f, static_cast<float>(size.y) - 30.0f);
        return {(static_cast<float>(size.x) - width) * 0.5f,
                (static_cast<float>(size.y) - height) * 0.5f, width, height};
    }

    sf::FloatRect galleryCard(std::size_t index) const {
        const bool threeColumns = size.x >= 1050u;
        const std::size_t columns = threeColumns ? 3u : 2u;
        const float gap = 18.0f;
        const float side = threeColumns ? 110.0f : 34.0f;
        const float available = static_cast<float>(size.x) - side * 2.0f - gap * static_cast<float>(columns - 1u);
        const float width = available / static_cast<float>(columns);
        const bool compactHeight = size.y < 650u;
        const float height = compactHeight ? 112.0f : (threeColumns ? 172.0f : 146.0f);
        const float startY = compactHeight ? 112.0f : (threeColumns ? 150.0f : 128.0f);
        const std::size_t column = index % columns;
        const std::size_t row = index / columns;
        return {side + static_cast<float>(column) * (width + gap),
                startY + static_cast<float>(row) * (height + gap), width, height};
    }
};

bool controlPressed() {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
}

bool altPressed() {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);
}

bool shiftPressed() {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
}

const char* toolName(ToolMode tool) {
    if (tool == ToolMode::Attract) return "ATTRACT";
    if (tool == ToolMode::Repel) return "REPEL";
    if (tool == ToolMode::Vortex) return "VORTEX";
    return "FLOW";
}

void applySettings(const AppSettings& settings, ParticleSystem& garden,
                   Camera3D& camera, sf::RenderWindow& window) {
    garden.setParticleCount(settings.particleCount);
    garden.setParticleScale(settings.particleScale);
    garden.setTrailPersistence(settings.trailPersistence);
    garden.setForceStrength(settings.forceStrength);
    garden.setPalette(settings.paletteIndex);
    garden.setFlowMode(settings.flowMode);
    camera.setFov(settings.cameraFov);
    camera.setSensitivity(settings.cameraSensitivity);
    window.setVerticalSyncEnabled(settings.verticalSync);
}

std::string captureArtwork(sf::RenderWindow& window) {
    namespace fs = std::filesystem;
    std::error_code error;
    fs::create_directories("captures", error);
    if (error) return {};

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif

    std::ostringstream filename;
    filename << "captures/particle_garden_" << std::put_time(&local, "%Y%m%d_%H%M%S") << ".png";
    sf::Texture capture;
    if (!capture.create(window.getSize().x, window.getSize().y)) return {};
    capture.update(window);
    return capture.copyToImage().saveToFile(filename.str()) ? filename.str() : std::string{};
}

float sliderNormalized(float value, float minimum, float maximum) {
    return (value - minimum) / (maximum - minimum);
}

void drawBrand(sf::RenderTarget& target, sf::Vector2f origin, sf::Color accent, float titleScale) {
    PixelText::draw(target, "PARTICLE GARDEN", origin, titleScale, {229, 246, 255});
    PixelText::draw(target, "SCULPT A LIVING UNIVERSE IN 3D",
                    {origin.x + 2.0f, origin.y + 43.0f}, 1.2f, accent);
}

void drawMainMenu(sf::RenderTarget& target, sf::Vector2f mouse, sf::Color accent,
                  const Layout& layout) {
    ui::overlay(target, 82);
    drawBrand(target, {72.0f, 88.0f}, accent, 5.0f);
    PixelText::draw(target, "A C++ GENERATIVE PARTICLE INSTRUMENT",
                    {75.0f, 165.0f}, 1.25f, {111, 148, 181});
    const std::array<const char*, 5> labels = {
        "ENTER GARDEN", "SCENE GALLERY", "SETTINGS", "CONTROLS", "EXIT"
    };
    for (std::size_t i = 0; i < labels.size(); ++i)
        ui::button(target, layout.mainButton(i), labels[i], mouse, accent);

    PixelText::draw(target, "VERSION 4  PARALLAX",
                    {74.0f, static_cast<float>(layout.size.y) - 36.0f}, 1.0f, {74, 105, 134});
}

void drawPauseMenu(sf::RenderTarget& target, sf::Vector2f mouse, sf::Color accent,
                   const Layout& layout, bool cinematic) {
    ui::overlay(target, 176);
    sf::FloatRect panel{(static_cast<float>(layout.size.x) - 450.0f) * 0.5f,
                        (static_cast<float>(layout.size.y) - 450.0f) * 0.5f, 450.0f, 450.0f};
    ui::panel(target, panel, accent, 238);
    const std::string title = cinematic ? "CINEMATIC PAUSED" : "GARDEN PAUSED";
    PixelText::draw(target, title,
                    {panel.left + (panel.width - ui::textWidth(title, 2.2f)) * 0.5f,
                     panel.top + 28.0f}, 2.2f, {230, 246, 255});
    const std::array<const char*, 6> labels = {
        "RESUME", "SCENE GALLERY", "SETTINGS", "CONTROLS", "MAIN MENU", "EXIT"
    };
    for (std::size_t i = 0; i < labels.size(); ++i)
        ui::button(target, layout.pauseButton(i), labels[i], mouse, accent);
}

std::array<sf::FloatRect, 7> settingsSliders(sf::FloatRect panel) {
    const float columnWidth = (panel.width - 150.0f) * 0.5f;
    const float left = panel.left + 52.0f;
    const float right = panel.left + panel.width * 0.5f + 24.0f;
    const float top = panel.top + 108.0f;
    const float step = 66.0f;
    return {
        sf::FloatRect{left, top, columnWidth, 42.0f},
        sf::FloatRect{left, top + step, columnWidth, 42.0f},
        sf::FloatRect{left, top + step * 2.0f, columnWidth, 42.0f},
        sf::FloatRect{left, top + step * 3.0f, columnWidth, 42.0f},
        sf::FloatRect{right, top, columnWidth, 42.0f},
        sf::FloatRect{right, top + step, columnWidth, 42.0f},
        sf::FloatRect{right, top + step * 2.0f, columnWidth, 42.0f}
    };
}

void drawSettings(sf::RenderTarget& target, sf::Vector2f mouse, sf::Color accent,
                  const Layout& layout, const AppSettings& settings, const Palette& palette) {
    ui::overlay(target, 182);
    const sf::FloatRect panel = layout.settingsPanel();
    ui::panel(target, panel, accent, 244);
    PixelText::draw(target, "SETTINGS", {panel.left + 42.0f, panel.top + 28.0f},
                    2.5f, {232, 247, 255});
    PixelText::draw(target, "SIMULATION  CAMERA  AND PRESENTATION",
                    {panel.left + 43.0f, panel.top + 58.0f}, 1.0f, {97, 137, 169});

    const auto sliders = settingsSliders(panel);
    ui::slider(target, sliders[0], "PARTICLE COUNT",
               sliderNormalized(static_cast<float>(settings.particleCount), 1000.0f, 12000.0f),
               std::to_string(settings.particleCount), mouse, accent);
    ui::slider(target, sliders[1], "PARTICLE SIZE",
               sliderNormalized(settings.particleScale, 0.6f, 2.2f),
               ui::fixed(settings.particleScale) + "X", mouse, accent);
    ui::slider(target, sliders[2], "TRAIL LENGTH",
               sliderNormalized(settings.trailPersistence, 0.10f, 0.95f),
               std::to_string(static_cast<int>(settings.trailPersistence * 100.0f)), mouse, accent);
    ui::slider(target, sliders[3], "ORBIT SPEED",
               sliderNormalized(settings.cameraSensitivity, 0.35f, 2.2f),
               ui::fixed(settings.cameraSensitivity) + "X", mouse, accent);
    ui::slider(target, sliders[4], "FORCE STRENGTH",
               sliderNormalized(settings.forceStrength, 0.45f, 2.2f),
               ui::fixed(settings.forceStrength) + "X", mouse, accent);
    ui::slider(target, sliders[5], "BACKGROUND",
               sliderNormalized(settings.backgroundIntensity, 0.15f, 1.6f),
               std::to_string(static_cast<int>(settings.backgroundIntensity * 100.0f)), mouse, accent);
    ui::slider(target, sliders[6], "CAMERA FOV",
               sliderNormalized(settings.cameraFov, 42.0f, 78.0f),
               std::to_string(static_cast<int>(settings.cameraFov)), mouse, accent);

    const float columnWidth = (panel.width - 150.0f) * 0.5f;
    const float left = panel.left + 52.0f;
    const float right = panel.left + panel.width * 0.5f + 24.0f;
    const float togglesTop = panel.top + 390.0f;
    ui::toggle(target, {left, togglesTop, columnWidth, 30.0f}, "SHOW HUD",
               settings.showHud, mouse, accent);
    ui::toggle(target, {left, togglesTop + 44.0f, columnWidth, 30.0f}, "DYNAMIC BACKGROUND",
               settings.dynamicBackground, mouse, accent);
    ui::button(target, {right, togglesTop - 5.0f, columnWidth, 42.0f},
               std::string("PALETTE  ") + palette.name, mouse, accent, true);
    ui::toggle(target, {right, togglesTop + 44.0f, columnWidth, 30.0f}, "VERTICAL SYNC",
               settings.verticalSync, mouse, accent);

    ui::button(target, {panel.left + 52.0f, panel.top + panel.height - 62.0f, 220.0f, 40.0f},
               "RESET DEFAULTS", mouse, accent);
    ui::button(target, {panel.left + panel.width - 272.0f, panel.top + panel.height - 62.0f,
                        220.0f, 40.0f}, "SAVE AND BACK", mouse, accent, true);
}

sf::Vector2f previewPoint(FlowMode mode, float t, float elapsed, sf::FloatRect rect, int lane) {
    const float x0 = rect.left + 12.0f;
    const float y0 = rect.top + 10.0f;
    const float w = rect.width - 24.0f;
    const float h = rect.height - 20.0f;
    const float phase = static_cast<float>(lane) * 1.73f;

    if (mode == FlowMode::BinaryGalaxy) {
        const float side = lane % 2 == 0 ? -1.0f : 1.0f;
        const float cx = x0 + w * (side < 0.0f ? 0.34f : 0.66f);
        const float cy = y0 + h * 0.50f;
        const float angle = t * 12.0f + elapsed * 0.20f * side + phase;
        const float radius = 9.0f + std::fmod(t * 71.0f + lane * 5.0f, 22.0f);
        return {cx + std::cos(angle) * radius, cy + std::sin(angle) * radius * 0.48f};
    }
    if (mode == FlowMode::NebulaRiver) {
        const float x = x0 + t * w;
        const float y = y0 + h * 0.5f + std::sin(t * 8.0f + elapsed * 0.22f + phase) * (10.0f + lane % 3 * 3.0f);
        return {x, y};
    }
    if (mode == FlowMode::SolarBloom) {
        const float angle = t * 13.0f + phase + elapsed * 0.18f;
        const float radius = 4.0f + t * std::min(w, h) * 0.48f;
        return {x0 + w * 0.5f + std::cos(angle) * radius,
                y0 + h * 0.5f + std::sin(angle) * radius * 0.60f};
    }
    if (mode == FlowMode::AuroraDrift) {
        const float x = x0 + t * w;
        const float y = y0 + h * (0.28f + 0.18f * static_cast<float>(lane % 3)) +
                        std::sin(t * 10.0f + elapsed * 0.25f + phase) * 9.0f;
        return {x, y};
    }

    const float x = x0 + t * w;
    const float y = y0 + h * 0.5f + std::sin(t * 11.0f + phase) * 19.0f +
                    std::cos(t * 4.0f + elapsed * 0.16f) * 8.0f;
    return {x, y};
}

void drawScenePreview(sf::RenderTarget& target, sf::FloatRect card, FlowMode mode,
                      const Palette& palette, float elapsed, bool selected) {
    sf::FloatRect preview{card.left + 12.0f, card.top + 12.0f, card.width - 24.0f,
                          std::max(54.0f, card.height * 0.48f)};
    sf::RectangleShape background({preview.width, preview.height});
    background.setPosition(preview.left, preview.top);
    background.setFillColor({2, 7, 18, 225});
    sf::Color border = palette.colors[0];
    border.a = selected ? 125 : 38;
    background.setOutlineThickness(1.0f);
    background.setOutlineColor(border);
    target.draw(background);

    sf::VertexArray filaments(sf::Lines);
    for (int lane = 0; lane < 5; ++lane) {
        sf::Vector2f previous{};
        for (int i = 0; i < 22; ++i) {
            const float t = static_cast<float>(i) / 21.0f;
            const sf::Vector2f p = previewPoint(mode, t, elapsed, preview, lane);
            if (i > 0 && (mode == FlowMode::NebulaRiver || mode == FlowMode::CosmicWeb || mode == FlowMode::AuroraDrift)) {
                sf::Color line = palette.colors[static_cast<std::size_t>(lane) % 4u];
                line.a = 28;
                filaments.append({previous, line});
                filaments.append({p, line});
            }
            if ((i + lane) % 2 == 0) {
                sf::CircleShape dot(1.0f + static_cast<float>((i + lane) % 3) * 0.42f);
                dot.setOrigin(dot.getRadius(), dot.getRadius());
                dot.setPosition(p);
                sf::Color color = palette.colors[static_cast<std::size_t>(i + lane) % 4u];
                color.a = static_cast<sf::Uint8>(115 + (i * 7) % 120);
                dot.setFillColor(color);
                target.draw(dot, sf::BlendAdd);
            }
            previous = p;
        }
    }
    target.draw(filaments, sf::BlendAdd);
}

void drawGallery(sf::RenderTarget& target, sf::Vector2f mouse, sf::Color accent,
                 const Layout& layout, FlowMode selected, const Palette& palette, float elapsed) {
    ui::overlay(target, 178);
    PixelText::draw(target, "SCENE GALLERY",
                    {(static_cast<float>(layout.size.x) - ui::textWidth("SCENE GALLERY", 3.0f)) * 0.5f, 48.0f},
                    3.0f, {230, 247, 255});
    PixelText::draw(target, "FIVE SPATIAL SYSTEMS  ONE C++ ENGINE",
                    {(static_cast<float>(layout.size.x) - ui::textWidth("FIVE SPATIAL SYSTEMS  ONE C++ ENGINE", 1.0f)) * 0.5f,
                     87.0f}, 1.0f, {112, 153, 184});

    for (std::size_t i = 0; i < 5; ++i) {
        const FlowMode mode = static_cast<FlowMode>(i);
        const sf::FloatRect card = layout.galleryCard(i);
        const bool hovered = ui::contains(card, mouse);
        const bool active = mode == selected;
        sf::Color cardAccent = palette.colors[i % palette.colors.size()];
        ui::panel(target, card, cardAccent, hovered ? 244 : 225);
        drawScenePreview(target, card, mode, palette, elapsed, active);

        const float titleY = card.top + card.height * 0.60f;
        PixelText::draw(target, flowModeName(mode), {card.left + 16.0f, titleY},
                        1.15f, hovered || active ? sf::Color(235, 249, 255) : sf::Color(176, 205, 226));
        PixelText::draw(target, flowModeSubtitle(mode), {card.left + 16.0f, titleY + 22.0f},
                        0.67f, {101, 142, 173});
        PixelText::draw(target, hovered ? "OPEN SCENE" : (active ? "ACTIVE" : "CLICK TO OPEN"),
                        {card.left + 16.0f, card.top + card.height - 20.0f}, 0.72f,
                        hovered || active ? cardAccent : sf::Color(78, 111, 139));
    }

    const sf::FloatRect back{(static_cast<float>(layout.size.x) - 220.0f) * 0.5f,
                             static_cast<float>(layout.size.y) - 62.0f, 220.0f, 38.0f};
    ui::button(target, back, "BACK", mouse, accent);
}

void drawControls(sf::RenderTarget& target, sf::Vector2f mouse, sf::Color accent,
                  const Layout& layout) {
    ui::overlay(target, 182);
    const float width = std::min(980.0f, static_cast<float>(layout.size.x) - 40.0f);
    const float height = std::min(620.0f, static_cast<float>(layout.size.y) - 34.0f);
    sf::FloatRect panel{(static_cast<float>(layout.size.x) - width) * 0.5f,
                        (static_cast<float>(layout.size.y) - height) * 0.5f, width, height};
    ui::panel(target, panel, accent, 244);
    PixelText::draw(target, "CONTROLS", {panel.left + 42.0f, panel.top + 30.0f},
                    2.5f, {232, 247, 255});
    PixelText::draw(target, "SIMULATION", {panel.left + 52.0f, panel.top + 82.0f},
                    1.25f, accent);
    PixelText::draw(target, "CAMERA", {panel.left + panel.width * 0.52f, panel.top + 82.0f},
                    1.25f, accent);

    const std::array<const char*, 9> left = {
        "LEFT DRAG       ATTRACT",
        "SHIFT + LEFT    VORTEX",
        "MIDDLE MOUSE    REPEL",
        "CTRL + MOUSE    PLACE FORCE NODE",
        "ALT + LEFT      DELETE NODE",
        "SPACE           ENERGY BLOOM",
        "F1-F5           CHANGE SCENE",
        "1-6             CHANGE PALETTE",
        "R / X           RESET / CLEAR NODES"
    };
    const std::array<const char*, 9> right = {
        "RIGHT DRAG      ORBIT CAMERA",
        "MOUSE WHEEL     CAMERA ZOOM",
        "CTRL + WHEEL    TOOL RADIUS",
        "HOME            RESET CAMERA",
        "C               CINEMATIC MODE",
        "T               CLEAR TRAIL HISTORY",
        "S               SAVE CLEAN PNG",
        "H               SHOW OR HIDE HUD",
        "ESC             PAUSE MENU"
    };

    for (std::size_t i = 0; i < left.size(); ++i) {
        PixelText::draw(target, left[i], {panel.left + 52.0f, panel.top + 116.0f + i * 34.0f},
                        0.88f, i < 3 ? sf::Color(190, 221, 238) : sf::Color(139, 178, 205));
        PixelText::draw(target, right[i], {panel.left + panel.width * 0.52f,
                                           panel.top + 116.0f + i * 34.0f},
                        0.88f, i < 4 ? sf::Color(190, 221, 238) : sf::Color(139, 178, 205));
    }

    PixelText::draw(target, "ALL PARTICLE PHYSICS  PROJECTION  CAMERA AND UI LOGIC RUN IN C++",
                    {panel.left + 52.0f, panel.top + panel.height - 102.0f},
                    0.78f, {90, 132, 164});
    ui::button(target, {panel.left + panel.width - 242.0f, panel.top + panel.height - 62.0f,
                        190.0f, 38.0f}, "BACK", mouse, accent);
}

void drawHud(sf::RenderTarget& target, const ParticleSystem& garden, ToolMode tool,
             const Camera3D& camera, sf::Color accent, bool cinematic,
             const std::string& toast, float toastAlpha, float fps, float frameMs) {
    ui::panel(target, {20.0f, 20.0f, 300.0f, 58.0f}, accent, 185);
    PixelText::draw(target, "PARTICLE GARDEN 3D", {36.0f, 34.0f}, 2.0f, {225, 244, 255});
    PixelText::draw(target, cinematic ? "CINEMATIC  C TO EXIT" : "ESC MENU  C CINEMATIC",
                    {37.0f, 62.0f}, 0.82f, {88, 130, 160});

    std::ostringstream status;
    status << garden.palette().name << " | " << flowModeName(garden.flowMode()) << " | "
           << garden.particleCount() << " P | " << garden.nodeCount() << " NODES | "
           << toolName(tool) << " | " << static_cast<int>(camera.distance()) << " CAM | "
           << static_cast<int>(std::round(fps)) << " FPS | "
           << std::fixed << std::setprecision(1) << frameMs << " MS";
    const float width = std::max(420.0f, ui::textWidth(status.str(), 0.84f) + 32.0f);
    ui::panel(target, {static_cast<float>(target.getSize().x) - width - 20.0f, 20.0f,
                       width, 34.0f}, accent, 185);
    PixelText::draw(target, status.str(),
                    {static_cast<float>(target.getSize().x) - width - 3.0f, 32.0f},
                    0.84f, {153, 203, 225});

    if (!toast.empty() && toastAlpha > 0.0f) {
        sf::Color toastColor{220, 246, 248, static_cast<sf::Uint8>(toastAlpha * 255.0f)};
        const float toastWidth = ui::textWidth(toast, 1.0f) + 44.0f;
        ui::panel(target, {(static_cast<float>(target.getSize().x) - toastWidth) * 0.5f,
                           static_cast<float>(target.getSize().y) - 66.0f, toastWidth, 36.0f}, accent,
                           static_cast<sf::Uint8>(toastAlpha * 210.0f));
        PixelText::draw(target, toast,
                        {(static_cast<float>(target.getSize().x) - ui::textWidth(toast, 1.0f)) * 0.5f,
                         static_cast<float>(target.getSize().y) - 54.0f}, 1.0f, toastColor);
    }
}

void drawInteractionCursor(sf::RenderTarget& target, const Camera3D& camera,
                           const sf::Vector3f& mouseWorld, float radius, sf::Color color) {
    const Projection3D center = camera.project(mouseWorld);
    const Projection3D edge = camera.project({mouseWorld.x + radius, mouseWorld.y, mouseWorld.z});
    if (!center.visible || !edge.visible) return;
    const float dx = edge.screen.x - center.screen.x;
    const float dy = edge.screen.y - center.screen.y;
    const float screenRadius = std::clamp(std::sqrt(dx * dx + dy * dy), 12.0f, 320.0f);
    sf::CircleShape cursor(screenRadius);
    cursor.setOrigin(screenRadius, screenRadius);
    cursor.setPosition(center.screen);
    cursor.setFillColor(sf::Color::Transparent);
    color.a = 62;
    cursor.setOutlineColor(color);
    cursor.setOutlineThickness(1.2f);
    target.draw(cursor, sf::BlendAdd);
}
}

int main() {
    sf::ContextSettings context;
    context.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(InitialWidth, InitialHeight), "Particle Garden 3D",
                            sf::Style::Default, context);
    window.setKeyRepeatEnabled(false);

    AppSettings settings;
    settings.load(SettingsFile);
    ParticleSystem garden(window.getSize(), settings.particleCount);
    DynamicBackground background(window.getSize());
    Camera3D camera(window.getSize());
    applySettings(settings, garden, camera, window);

    sf::Clock frameClock;
    sf::Clock totalClock;
    Screen screen = Screen::MainMenu;
    Screen returnScreen = Screen::MainMenu;
    int activeSlider = -1;
    int draggedNode = -1;
    float interactionRadius = 210.0f;
    bool captureRequested = false;
    bool cinematic = false;
    bool cameraDragging = false;
    sf::Vector2i cameraLastMouse{};
    std::string toast;
    float toastTimer = 0.0f;
    float smoothedFrameSeconds = 1.0f / 60.0f;

    auto updateSlider = [&](int index, float mouseX) {
        const Layout layout{window.getSize()};
        const auto rects = settingsSliders(layout.settingsPanel());
        if (index < 0 || index >= static_cast<int>(rects.size())) return;
        const float amount = std::clamp((mouseX - rects[static_cast<std::size_t>(index)].left) /
                                        rects[static_cast<std::size_t>(index)].width, 0.0f, 1.0f);
        if (index == 0) settings.particleCount = static_cast<std::size_t>(1000 + std::round(amount * 22.0f) * 500.0f);
        if (index == 1) settings.particleScale = 0.6f + amount * 1.6f;
        if (index == 2) settings.trailPersistence = 0.10f + amount * 0.85f;
        if (index == 3) settings.cameraSensitivity = 0.35f + amount * 1.85f;
        if (index == 4) settings.forceStrength = 0.45f + amount * 1.75f;
        if (index == 5) settings.backgroundIntensity = 0.15f + amount * 1.45f;
        if (index == 6) settings.cameraFov = 42.0f + amount * 36.0f;
        applySettings(settings, garden, camera, window);
    };

    while (window.isOpen()) {
        const float frameSeconds = std::max(frameClock.restart().asSeconds(), 0.000001f);
        const float dt = std::min(frameSeconds, 0.05f);
        smoothedFrameSeconds += (frameSeconds - smoothedFrameSeconds) * 0.08f;
        const float elapsed = totalClock.getElapsedTime().asSeconds();
        const Layout layout{window.getSize()};
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector3f mouseWorld = camera.screenToPlane(mouse, 0.0f);
        sf::Event event{};

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) {
                const unsigned width = std::max(event.size.width, 800u);
                const unsigned height = std::max(event.size.height, 560u);
                window.setView(sf::View(sf::FloatRect(0.0f, 0.0f,
                    static_cast<float>(width), static_cast<float>(height))));
                garden.resize({width, height});
                background.resize({width, height});
                camera.resize({width, height});
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    if (screen == Screen::Simulation) {
                        screen = Screen::Pause;
                        cameraDragging = false;
                    } else if (screen == Screen::Pause) {
                        screen = Screen::Simulation;
                    } else if (screen == Screen::Settings || screen == Screen::Gallery || screen == Screen::Controls) {
                        screen = returnScreen;
                    }
                }

                if (screen == Screen::Simulation) {
                    if (event.key.code == sf::Keyboard::H) settings.showHud = !settings.showHud;
                    if (event.key.code == sf::Keyboard::R) garden.reset();
                    if (event.key.code == sf::Keyboard::X) garden.clearNodes();
                    if (event.key.code == sf::Keyboard::S) captureRequested = true;
                    if (event.key.code == sf::Keyboard::Space) garden.bloom(mouseWorld);
                    if (event.key.code == sf::Keyboard::Home) camera.reset();
                    if (event.key.code == sf::Keyboard::C) {
                        cinematic = !cinematic;
                        cameraDragging = false;
                        toast = cinematic ? "CINEMATIC MODE ON" : "CINEMATIC MODE OFF";
                        toastTimer = 2.0f;
                    }
                    if (event.key.code == sf::Keyboard::T) {
                        garden.clearTrails();
                        toast = "TRAIL HISTORY CLEARED";
                        toastTimer = 1.8f;
                    }
                    if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num6) {
                        settings.paletteIndex = static_cast<std::size_t>(event.key.code - sf::Keyboard::Num1);
                        garden.setPalette(settings.paletteIndex);
                    }
                    if (event.key.code >= sf::Keyboard::F1 && event.key.code <= sf::Keyboard::F5) {
                        settings.flowMode = static_cast<FlowMode>(event.key.code - sf::Keyboard::F1);
                        garden.setFlowMode(settings.flowMode);
                    }
                    if (event.key.code == sf::Keyboard::LBracket) {
                        settings.particleCount = garden.particleCount() -
                            std::min<std::size_t>(1000, garden.particleCount() - 1000);
                        garden.setParticleCount(settings.particleCount);
                    }
                    if (event.key.code == sf::Keyboard::RBracket) {
                        settings.particleCount = std::min<std::size_t>(12000, garden.particleCount() + 1000);
                        garden.setParticleCount(settings.particleCount);
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                mouse = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
                mouseWorld = camera.screenToPlane(mouse, 0.0f);

                if (screen == Screen::Simulation && !cinematic) {
                    if (controlPressed()) {
                        if (event.mouseButton.button == sf::Mouse::Left) garden.addNode(mouseWorld, NodeType::Attractor);
                        if (event.mouseButton.button == sf::Mouse::Right) garden.addNode(mouseWorld, NodeType::Repulsor);
                        if (event.mouseButton.button == sf::Mouse::Middle) garden.addNode(mouseWorld, NodeType::Vortex);
                    } else if (altPressed() && event.mouseButton.button == sf::Mouse::Left) {
                        const int node = garden.findNearestNodeScreen(mouse, camera, 38.0f);
                        if (node >= 0) garden.removeNode(static_cast<std::size_t>(node));
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        cameraDragging = true;
                        cameraLastMouse = {event.mouseButton.x, event.mouseButton.y};
                    } else if (event.mouseButton.button == sf::Mouse::Left) {
                        draggedNode = garden.findNearestNodeScreen(mouse, camera, 28.0f);
                    }
                } else if (event.mouseButton.button == sf::Mouse::Left) {
                    if (screen == Screen::MainMenu) {
                        if (ui::contains(layout.mainButton(0), mouse)) screen = Screen::Simulation;
                        else if (ui::contains(layout.mainButton(1), mouse)) { returnScreen = Screen::MainMenu; screen = Screen::Gallery; }
                        else if (ui::contains(layout.mainButton(2), mouse)) { returnScreen = Screen::MainMenu; screen = Screen::Settings; }
                        else if (ui::contains(layout.mainButton(3), mouse)) { returnScreen = Screen::MainMenu; screen = Screen::Controls; }
                        else if (ui::contains(layout.mainButton(4), mouse)) window.close();
                    } else if (screen == Screen::Pause) {
                        if (ui::contains(layout.pauseButton(0), mouse)) screen = Screen::Simulation;
                        else if (ui::contains(layout.pauseButton(1), mouse)) { returnScreen = Screen::Pause; screen = Screen::Gallery; }
                        else if (ui::contains(layout.pauseButton(2), mouse)) { returnScreen = Screen::Pause; screen = Screen::Settings; }
                        else if (ui::contains(layout.pauseButton(3), mouse)) { returnScreen = Screen::Pause; screen = Screen::Controls; }
                        else if (ui::contains(layout.pauseButton(4), mouse)) { screen = Screen::MainMenu; cinematic = false; }
                        else if (ui::contains(layout.pauseButton(5), mouse)) window.close();
                    } else if (screen == Screen::Gallery) {
                        bool opened = false;
                        for (std::size_t i = 0; i < 5; ++i) {
                            if (ui::contains(layout.galleryCard(i), mouse)) {
                                settings.flowMode = static_cast<FlowMode>(i);
                                garden.setFlowMode(settings.flowMode);
                                screen = Screen::Simulation;
                                cinematic = false;
                                opened = true;
                                break;
                            }
                        }
                        const sf::FloatRect back{(static_cast<float>(layout.size.x) - 220.0f) * 0.5f,
                                                 static_cast<float>(layout.size.y) - 62.0f, 220.0f, 38.0f};
                        if (!opened && ui::contains(back, mouse)) screen = returnScreen;
                    } else if (screen == Screen::Controls) {
                        const float width = std::min(980.0f, static_cast<float>(layout.size.x) - 40.0f);
                        const float height = std::min(620.0f, static_cast<float>(layout.size.y) - 34.0f);
                        sf::FloatRect panel{(static_cast<float>(layout.size.x) - width) * 0.5f,
                                            (static_cast<float>(layout.size.y) - height) * 0.5f, width, height};
                        if (ui::contains({panel.left + panel.width - 242.0f,
                                          panel.top + panel.height - 62.0f, 190.0f, 38.0f}, mouse))
                            screen = returnScreen;
                    } else if (screen == Screen::Settings) {
                        const sf::FloatRect panel = layout.settingsPanel();
                        const auto rects = settingsSliders(panel);
                        for (std::size_t i = 0; i < rects.size(); ++i) {
                            sf::FloatRect hit{rects[i].left - 8.0f, rects[i].top + 15.0f,
                                              rects[i].width + 16.0f, 30.0f};
                            if (ui::contains(hit, mouse)) {
                                activeSlider = static_cast<int>(i);
                                updateSlider(activeSlider, mouse.x);
                            }
                        }

                        const float columnWidth = (panel.width - 150.0f) * 0.5f;
                        const float left = panel.left + 52.0f;
                        const float right = panel.left + panel.width * 0.5f + 24.0f;
                        const float togglesTop = panel.top + 390.0f;
                        if (ui::contains({left, togglesTop, columnWidth, 30.0f}, mouse)) settings.showHud = !settings.showHud;
                        if (ui::contains({left, togglesTop + 44.0f, columnWidth, 30.0f}, mouse)) settings.dynamicBackground = !settings.dynamicBackground;
                        if (ui::contains({right, togglesTop - 5.0f, columnWidth, 42.0f}, mouse)) {
                            settings.paletteIndex = (settings.paletteIndex + 1) % 6;
                            garden.setPalette(settings.paletteIndex);
                        }
                        if (ui::contains({right, togglesTop + 44.0f, columnWidth, 30.0f}, mouse)) {
                            settings.verticalSync = !settings.verticalSync;
                            window.setVerticalSyncEnabled(settings.verticalSync);
                        }
                        if (ui::contains({panel.left + 52.0f, panel.top + panel.height - 62.0f,
                                          220.0f, 40.0f}, mouse)) {
                            settings.reset();
                            applySettings(settings, garden, camera, window);
                            camera.reset();
                        }
                        if (ui::contains({panel.left + panel.width - 272.0f,
                                          panel.top + panel.height - 62.0f, 220.0f, 40.0f}, mouse)) {
                            settings.save(SettingsFile);
                            screen = returnScreen;
                        }
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    activeSlider = -1;
                    draggedNode = -1;
                }
                if (event.mouseButton.button == sf::Mouse::Right) cameraDragging = false;
            }

            if (event.type == sf::Event::MouseMoved) {
                mouse = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
                if (screen == Screen::Settings && activeSlider >= 0) updateSlider(activeSlider, mouse.x);
                if (screen == Screen::Simulation && !cinematic) {
                    if (cameraDragging) {
                        const sf::Vector2i current{event.mouseMove.x, event.mouseMove.y};
                        camera.orbit(static_cast<float>(current.x - cameraLastMouse.x),
                                     static_cast<float>(current.y - cameraLastMouse.y));
                        cameraLastMouse = current;
                    } else if (draggedNode >= 0) {
                        mouseWorld = camera.screenToPlane(mouse, 0.0f);
                        garden.moveNode(static_cast<std::size_t>(draggedNode), mouseWorld);
                    }
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled && screen == Screen::Simulation && !cinematic) {
                if (controlPressed()) {
                    interactionRadius = std::clamp(interactionRadius + event.mouseWheelScroll.delta * 22.0f,
                                                   85.0f, 390.0f);
                } else {
                    camera.zoom(event.mouseWheelScroll.delta);
                }
            }
        }

        mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (cinematic && screen == Screen::Simulation) camera.updateCinematic(dt, elapsed);
        mouseWorld = camera.screenToPlane(mouse, 0.0f);
        background.update(dt, mouse);

        ToolMode tool = ToolMode::None;
        if (screen == Screen::Simulation && !cinematic && !controlPressed() && !altPressed() &&
            draggedNode < 0 && !cameraDragging) {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                tool = shiftPressed() ? ToolMode::Vortex : ToolMode::Attract;
            if (sf::Mouse::isButtonPressed(sf::Mouse::Middle)) tool = ToolMode::Repel;
        }

        if (screen != Screen::Pause) {
            const float simulationDt = cinematic ? dt * 0.72f : dt;
            garden.update(simulationDt, elapsed, mouseWorld, tool, interactionRadius);
        }

        window.clear();
        background.draw(window, elapsed, garden.palette(), tool,
                        settings.backgroundIntensity, settings.dynamicBackground);
        garden.drawTrails(window, camera);
        garden.drawBloomRings(window, camera);
        garden.drawConnections(window, camera, mouseWorld, tool, interactionRadius);
        garden.drawParticles(window, camera);
        garden.drawNodes(window, camera, elapsed);

        if (tool != ToolMode::None)
            drawInteractionCursor(window, camera, mouseWorld, interactionRadius, garden.palette().colors[0]);

        if (captureRequested && screen == Screen::Simulation) {
            const std::string path = captureArtwork(window);
            toast = path.empty() ? "CAPTURE FAILED" : "ARTWORK SAVED TO CAPTURES";
            toastTimer = 2.7f;
            captureRequested = false;
        }

        const sf::Color accent = garden.palette().colors[0];
        if (screen == Screen::Simulation && settings.showHud && !cinematic) {
            const float toastAlpha = std::clamp(toastTimer / 0.5f, 0.0f, 1.0f);
            const float fps = 1.0f / std::max(smoothedFrameSeconds, 0.000001f);
            drawHud(window, garden, tool, camera, accent, cinematic, toast, toastAlpha,
                    fps, smoothedFrameSeconds * 1000.0f);
        }
        if (screen == Screen::MainMenu) drawMainMenu(window, mouse, accent, layout);
        if (screen == Screen::Pause) drawPauseMenu(window, mouse, accent, layout, cinematic);
        if (screen == Screen::Settings) drawSettings(window, mouse, accent, layout, settings, garden.palette());
        if (screen == Screen::Gallery) drawGallery(window, mouse, accent, layout, settings.flowMode, garden.palette(), elapsed);
        if (screen == Screen::Controls) drawControls(window, mouse, accent, layout);

        window.display();
        toastTimer = std::max(0.0f, toastTimer - dt);
    }

    settings.save(SettingsFile);
    return 0;
}
