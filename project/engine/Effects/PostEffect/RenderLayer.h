#pragma once
#include <cstddef>

/// <summary>
/// 合成レイヤーの識別子。
/// 各レイヤーは専用のCanvas（描画先＋ポストエフェクトスタック）を持ち、
/// PostEffectManager 経由でレイヤーごとに別々のエフェクトを掛けられる。
/// レイヤーを増やすときは Count の前に足す。
/// </summary>
enum class RenderLayerId {
	World,     // 背景＋3Dオブジェクト＋前景スプライト
	Particle,  // パーティクル専用
	Count
};

constexpr size_t kRenderLayerCount = static_cast<size_t>(RenderLayerId::Count);
