#pragma once
#include "Transform.h"
#include <string>

// 前方宣言：実装側で定義されるParticleManagerクラス
class ParticleManager;

/// エミッターが生成するパーティクルの形状タイプ
enum class ShapeType {
    Plane,      // 通常の板状パーティクル
    Primitive,  // ランダムな長さの直線型パーティクル
    Ring,       // リング形状（円形配置）
    Cylinder    // 円柱状パーティクル
};

/// パーティクルエミッターの設定構造体
struct EmitterConfig {
    ShapeType shapeType = ShapeType::Plane;  // 使用する形状タイプ
    uint32_t count = 10;                     // 発生数（Ring, Cylinder は未使用）
    float frequency = 1.0f;                  // 発生間隔（秒）
    bool repeat = false;                     // 繰り返し発生させるか
};

class ParticleEmitter {
public:
    // デフォルトコンストラクタ（必ず Initialize を呼び出すこと）
    ParticleEmitter() = default;

    /// 初期化particleManager 使用するParticleManagerのポインタ
    /// name パーティクルグループ名
    /// transform エミッターの位置・回転・スケール
    /// config パーティクルの設定
    void Initialize(ParticleManager* particleManager, const std::string& name, const Transform& transform, const EmitterConfig& config);

    /// フレーム毎の更新処理（repeat=true時のみEmitを発生）
    void Update();

    /// 外部から明示的にEmitを発生させたい場合に使用
    void Emit();

    // ----- Setter -----

    /// 繰り返しのON/OFF設定
    void SetRepeat(bool repeat);

    /// パーティクルの発生数を設定
    void SetCount(uint32_t count);

    /// パーティクルの発生間隔を設定
    void SetFrequency(float frequency);

    /// エミッターのTransformを設定
    void SetTransform(const Transform& transform);

    /// エミッターの位置を変更
    void SetPosition(const Vector3& position);

    /// エミッターの回転を変更
    void SetRotation(const Vector3& rotation);

    /// エミッターのスケールを変更
    void SetScale(const Vector3& scale);


    // ----- Getter -----

    /// Transform（const参照版）
    const Transform& GetTransform() const;

    /// Transform（書き換え可能版）
    Transform& GetTransform();

private:
    /// 形状タイプに応じて正しいEmitを呼び分ける
    void EmitByShape();

private:
    ParticleManager* particleManager_ = nullptr;  // パーティクルマネージャ
    std::string name_;                            // グループ名（識別子）
    Transform transform_{};                       // エミッターの位置・回転・拡縮
    EmitterConfig config_{};                      // パーティクル設定
    float elapsedTime_ = 0.0f;                    // 発生時間の蓄積
    bool isInitialized_ = false;                  // 初期化済みフラグ
};
