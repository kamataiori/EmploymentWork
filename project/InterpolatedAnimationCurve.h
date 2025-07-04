#pragma once
#include <vector>
#include <cassert>
#include <cmath>
#include "Vector3.h"
#include "Quaternion.h"

// Interpolation template declaration and specializations

template <typename T>
T Interpolate(const T& a, const T& b, float t);

template <>
inline Vector3 Interpolate<Vector3>(const Vector3& a, const Vector3& b, float t) {
    return a * (1.0f - t) + b * t;
}

template <>
inline Quaternion Interpolate<Quaternion>(const Quaternion& a, const Quaternion& b, float t) {
    return Quaternion::Slerp(a, b, t);
}

template <typename T>
struct Keyframe {
    float time;
    T value;
};

template <typename T>
class AnimationCurve {
public:
    void AddKeyframe(float time, const T& value) {
        keyframes_.push_back({ time, value });
    }

    T Evaluate(float time) const {
        if (keyframes_.empty()) return T{};

        if (time <= keyframes_.front().time) {
            return keyframes_.front().value;
        }
        if (time >= keyframes_.back().time) {
            return keyframes_.back().value;
        }

        for (size_t i = 0; i < keyframes_.size() - 1; ++i) {
            const auto& kf0 = keyframes_[i];
            const auto& kf1 = keyframes_[i + 1];
            if (time >= kf0.time && time <= kf1.time) {
                float t = (time - kf0.time) / (kf1.time - kf0.time);
                return Interpolate(kf0.value, kf1.value, t);
            }
        }
        return keyframes_.back().value;
    }

    const std::vector<Keyframe<T>>& GetKeyframes() const {
        return keyframes_;
    }

private:
    std::vector<Keyframe<T>> keyframes_;
};