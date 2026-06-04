#pragma once
#include <cmath>
#include "TitleScene.h"

/// <summary>
/// 汎用 Tween / Easing 用のユーティリティ
/// ここにイージング関数をまとめておき、どのクラスからでも再利用できるようにする
/// </summary>
namespace Tween
{
    // イージング関数の型
    using EasingFunc = float(*)(float);

    namespace Easing
    {
        /// --------------------------------------
        /// 基本イージング
        /// --------------------------------------

        /// <summary>線形補間（そのまま t）</summary>
        inline float Linear(float t)
        {
            return t;
        }

        /// <summary>Ease-In Quad（ゆっくり始まってだんだん速く）</summary>
        inline float EaseInQuad(float t)
        {
            return t * t;
        }

        /// <summary>Ease-Out Quad（速く始まってだんだんゆっくり）</summary>
        inline float EaseOutQuad(float t)
        {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }

        /// <summary>Ease-In-Out Quad（最初と最後がゆっくり）</summary>
        inline float EaseInOutQuad(float t)
        {
            if (t < 0.5f) {
                return 2.0f * t * t;
            }
            else {
                float u = 1.0f - t;
                return 1.0f - 2.0f * u * u;
            }
        }

        /// <summary>Ease-In-Out Cubic（メリハリがある）</summary>
        inline float EaseInOutCubic(float t)
        {
            if (t < 0.5f) {
                return 4.0f * t * t * t;
            }
            else {
                float u = (2.0f * t) - 2.0f;
                return 0.5f * u * u * u + 1.0f;
            }
        }

        /// --------------------------------------
        /// 追加：もっと急な加速系
        /// --------------------------------------

        /// <summary>
        /// Ease-In Cubic（かなり急に加速）
        /// 最初はほぼ動かず、中盤から一気に回り込ませたいときに最適
        /// </summary>
        inline float EaseInCubic(float t)
        {
            return t * t * t;
        }

        /// <summary>
        /// Ease-In Quart（Cubic よりもさらに急激に加速）
        /// 回り込みを「最初ゆっくり → 中盤～後半で一気に加速」させたい場合
        /// </summary>
        inline float EaseInQuart(float t)
        {
            float u = t;
            return u * u * u * u;
        }

        /// <summary>
        /// Ease-In Quint（Quart よりさらに強い加速）
        /// </summary>
        inline float EaseInQuint(float t)
        {
            float u = t;
            return u * u * u * u * u;
        }

        /// <summary>
        /// Ease-In Expo（終盤で一気に動き出す超極端な加速）
        /// 「最後に一気に走り抜ける」ような演出に最適
        /// </summary>
        inline float EaseInExpo(float t)
        {
            if (t <= 0.0f) return 0.0f;
            if (t >= 1.0f) return 1.0f;
            return std::pow(2.0f, 10.0f * (t - 1.0f));
        }

        /// --------------------------------------
        /// 追加：急激に減速系 / 反対方向
        /// --------------------------------------

        /// <summary>Ease-Out Cubic（最初に加速 → 最後ゆっくり）</summary>
        inline float EaseOutCubic(float t)
        {
            float u = t - 1.0f;
            return u * u * u + 1.0f;
        }

        /// <summary>Ease-Out Quart（より急な減速）</summary>
        inline float EaseOutQuart(float t)
        {
            float u = t - 1.0f;
            return -(u * u * u * u) + 1.0f;
        }

        /// <summary>Ease-Out Expo（ほぼ最初に全部動いて、最後ゆっくり）</summary>
        inline float EaseOutExpo(float t)
        {
            if (t >= 1.0f) return 1.0f;
            return 1.0f - std::pow(2.0f, -10.0f * t);
        }

        /// --------------------------------------
        /// さらに複雑：インアウトの Expo
        /// --------------------------------------

        /// <summary>Ease-In-Out Expo（最初と最後が超ゆっくり、中盤で爆速）</summary>
        inline float EaseInOutExpo(float t)
        {
            if (t <= 0.0f) return 0.0f;
            if (t >= 1.0f) return 1.0f;

            if (t < 0.5f) {
                return std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f;
            }
            else {
                return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
            }
        }
    }

    /// --------------------------------------
    /// 補間関数（線形）
    /// --------------------------------------
    template<typename T>
    inline T Lerp(const T& a, const T& b, float t)
    {
        return a + (b - a) * t;
    }

    /// --------------------------------------
    /// イージング補間版 Evaluate
    /// --------------------------------------
    template<typename T>
    inline T Evaluate(const T& from, const T& to, float t, EasingFunc easing)
    {
        float e = easing ? easing(t) : t;
        return Lerp(from, to, e);
    }
}
