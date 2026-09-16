#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

struct Projection3D {
    sf::Vector2f screen{};
    float depth = 0.0f;
    float scale = 0.0f;
    bool visible = false;
};

class Camera3D {
public:
    explicit Camera3D(sf::Vector2u viewport = {1280u, 720u}) : viewport_(viewport) {}

    void resize(sf::Vector2u viewport) {
        if (viewport_.x == viewport.x && viewport_.y == viewport.y) return;
        viewport_ = viewport;
        markDirty();
    }

    void setFov(float degrees) {
        const float value = std::clamp(degrees, 42.0f, 78.0f);
        if (std::abs(value - fovDegrees_) < 0.0001f) return;
        fovDegrees_ = value;
        markDirty();
    }
    [[nodiscard]] float fov() const { return fovDegrees_; }

    void setSensitivity(float value) { sensitivity_ = std::clamp(value, 0.35f, 2.2f); }
    [[nodiscard]] float sensitivity() const { return sensitivity_; }

    void setDistance(float value) {
        const float clamped = std::clamp(value, 430.0f, 1850.0f);
        if (std::abs(clamped - distance_) < 0.0001f) return;
        distance_ = clamped;
        markDirty();
    }
    [[nodiscard]] float distance() const { return distance_; }

    void orbit(float deltaX, float deltaY) {
        if (std::abs(deltaX) < 0.0001f && std::abs(deltaY) < 0.0001f) return;
        yaw_ += deltaX * 0.0052f * sensitivity_;
        pitch_ = std::clamp(pitch_ + deltaY * 0.0046f * sensitivity_, -1.15f, 1.15f);
        markDirty();
    }

    void zoom(float wheelDelta) {
        if (std::abs(wheelDelta) < 0.0001f) return;
        distance_ *= std::exp(-wheelDelta * 0.105f);
        distance_ = std::clamp(distance_, 430.0f, 1850.0f);
        markDirty();
    }

    void reset() {
        yaw_ = 0.55f;
        pitch_ = 0.16f;
        distance_ = 1080.0f;
        target_ = {0.0f, 0.0f, 0.0f};
        markDirty();
    }

    void updateCinematic(float dt, float elapsed) {
        yaw_ += dt * 0.105f;
        const float desiredPitch = 0.13f + std::sin(elapsed * 0.19f) * 0.24f;
        pitch_ += (desiredPitch - pitch_) * std::min(dt * 1.5f, 1.0f);
        const float desiredDistance = 1030.0f + std::sin(elapsed * 0.13f) * 145.0f;
        distance_ += (desiredDistance - distance_) * std::min(dt * 0.7f, 1.0f);
        markDirty();
    }

    [[nodiscard]] Projection3D project(const sf::Vector3f& point) const {
        ensureCache();
        const sf::Vector3f rel = subtract(point, cachedBasis_.position);
        const float cameraX = dot(rel, cachedBasis_.right);
        const float cameraY = dot(rel, cachedBasis_.up);
        const float cameraZ = dot(rel, cachedBasis_.forward);

        Projection3D result;
        result.depth = cameraZ;
        if (cameraZ <= 8.0f || viewport_.x == 0u || viewport_.y == 0u) return result;

        result.scale = cachedFocal_ / cameraZ;
        result.screen = {
            cachedHalfWidth_ + cameraX * result.scale,
            cachedHalfHeight_ - cameraY * result.scale
        };
        constexpr float margin = 160.0f;
        result.visible = result.screen.x >= -margin && result.screen.x <= static_cast<float>(viewport_.x) + margin &&
                         result.screen.y >= -margin && result.screen.y <= static_cast<float>(viewport_.y) + margin;
        return result;
    }

    [[nodiscard]] sf::Vector3f screenToPlane(sf::Vector2f screen, float planeZ = 0.0f) const {
        ensureCache();
        if (cachedFocal_ <= 0.001f) return target_;

        const float cameraX = (screen.x - cachedHalfWidth_) / cachedFocal_;
        const float cameraY = -(screen.y - cachedHalfHeight_) / cachedFocal_;
        sf::Vector3f direction = add(cachedBasis_.forward,
                                     add(scale(cachedBasis_.right, cameraX),
                                         scale(cachedBasis_.up, cameraY)));
        direction = normalized(direction);
        if (std::abs(direction.z) < 0.0001f) return target_;
        const float t = (planeZ - cachedBasis_.position.z) / direction.z;
        if (t <= 0.0f) return target_;
        return add(cachedBasis_.position, scale(direction, t));
    }

    [[nodiscard]] sf::Vector3f position() const {
        ensureCache();
        return cachedBasis_.position;
    }
    [[nodiscard]] sf::Vector3f target() const { return target_; }
    [[nodiscard]] std::uint64_t revision() const { return revision_; }

private:
    struct Basis {
        sf::Vector3f position{};
        sf::Vector3f forward{};
        sf::Vector3f right{};
        sf::Vector3f up{};
    };

    sf::Vector2u viewport_{};
    sf::Vector3f target_{0.0f, 0.0f, 0.0f};
    float yaw_ = 0.55f;
    float pitch_ = 0.16f;
    float distance_ = 1080.0f;
    float fovDegrees_ = 58.0f;
    float sensitivity_ = 1.0f;

    mutable Basis cachedBasis_{};
    mutable float cachedFocal_ = 1.0f;
    mutable float cachedHalfWidth_ = 640.0f;
    mutable float cachedHalfHeight_ = 360.0f;
    mutable bool cacheDirty_ = true;
    std::uint64_t revision_ = 1;

    void markDirty() {
        cacheDirty_ = true;
        ++revision_;
    }

    void ensureCache() const {
        if (!cacheDirty_) return;
        cachedBasis_ = makeBasisUncached();
        cachedFocal_ = focalLengthUncached();
        cachedHalfWidth_ = static_cast<float>(viewport_.x) * 0.5f;
        cachedHalfHeight_ = static_cast<float>(viewport_.y) * 0.5f;
        cacheDirty_ = false;
    }

    [[nodiscard]] float focalLengthUncached() const {
        constexpr float Pi = 3.14159265358979323846f;
        const float radians = fovDegrees_ * Pi / 180.0f;
        return (static_cast<float>(viewport_.y) * 0.5f) / std::tan(radians * 0.5f);
    }

    [[nodiscard]] Basis makeBasisUncached() const {
        const float cp = std::cos(pitch_);
        const sf::Vector3f orbitDirection{
            cp * std::sin(yaw_),
            std::sin(pitch_),
            cp * std::cos(yaw_)
        };
        Basis basis;
        basis.position = add(target_, scale(orbitDirection, distance_));
        basis.forward = normalized(subtract(target_, basis.position));

        const sf::Vector3f worldUp{0.0f, 1.0f, 0.0f};
        basis.right = normalized(cross(basis.forward, worldUp));
        if (dot(basis.right, basis.right) < 0.000001f) basis.right = {1.0f, 0.0f, 0.0f};
        basis.up = normalized(cross(basis.right, basis.forward));
        return basis;
    }

    static sf::Vector3f add(const sf::Vector3f& a, const sf::Vector3f& b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }
    static sf::Vector3f subtract(const sf::Vector3f& a, const sf::Vector3f& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }
    static sf::Vector3f scale(const sf::Vector3f& v, float s) {
        return {v.x * s, v.y * s, v.z * s};
    }
    static float dot(const sf::Vector3f& a, const sf::Vector3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    static sf::Vector3f cross(const sf::Vector3f& a, const sf::Vector3f& b) {
        return {a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
    }
    static float lengthSquared(const sf::Vector3f& v) {
        return dot(v, v);
    }
    static sf::Vector3f normalized(const sf::Vector3f& v) {
        const float magnitudeSquared = lengthSquared(v);
        if (magnitudeSquared <= 0.00000001f) return {};
        return scale(v, 1.0f / std::sqrt(magnitudeSquared));
    }
};
