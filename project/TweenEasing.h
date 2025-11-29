#pragma once
#include <cmath>

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
        /// <summary>
        /// 線形補間（そのまま t）
        /// </summary>
        inline float Linear(float t)
        {
            return t;
        }

        /// <summary>
        /// イージング：Ease-In Quad（ゆっくり始まってだんだん速く）
        /// </summary>
        inline float EaseInQuad(float t)
        {
            return t * t;
        }

        /// <summary>
        /// イージング：Ease-Out Quad（速く始まってだんだんゆっくり）
        /// </summary>
        inline float EaseOutQuad(float t)
        {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }

        /// <summary>
        /// イージング：Ease-In-Out Quad（最初と最後がゆっくり）
        /// </summary>
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

        /// <summary>
        /// イージング：Ease-In-Out Cubic（よりメリハリのあるカーブ）
        /// </summary>
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
    }

    /// <summary>
    /// 汎用 Lerp（線形補間） T が +,-,*(float) をサポートしていれば何でも使える
    /// </summary>
    template<typename T>
    inline T Lerp(const T& a, const T& b, float t)
    {
        return a + (b - a) * t;
    }

    /// <summary>
    /// from → to を t（0～1）の範囲で、指定イージングで補間した値を返す
    /// EasingFunc には上の Easing::XXX を渡す。
    /// </summary>
    template<typename T>
    inline T Evaluate(const T& from, const T& to, float t, EasingFunc easing)
    {
        if (!easing) {
            // イージング指定が無ければ線形扱い
            return Lerp(from, to, t);
        }
        float e = easing(t);
        return Lerp(from, to, e);
    }

}
