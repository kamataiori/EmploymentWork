// DamagePopupManager.h
#pragma once
#include "IDamagePopupSink.h"
#include "Sprite.h"
#include "Camera/Camera.h"
#include "WinApp.h"
#include "MathFunctions.h"
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

//======================================================
// DamagePopupManager（ダメージ数値のフロート表示）
//------------------------------------------------------
// ・敵が被ダメージしたとき、敵のワールド座標を画面に投影し、
//   その「右上」に与ダメージ量を数字PNG（Resources/number/0X.png）で表示する。
// ・色は赤（テクスチャは白なので乗算で赤く染める）。
// ・少し上へ浮かびながらフェードして消える。
// ・IDamagePopupSink 実装。Scene が所有し、Enemy/MinionEnemy へ注入する。
// ・ヘッダオンリー：新規 .cpp を増やさない。
//======================================================
class DamagePopupManager : public IDamagePopupSink {
public:
    void SetCamera(Camera* camera) { camera_ = camera; }

    // ---- IDamagePopupSink ----
    void SpawnDamage(const Vector3& worldPos, int amount) override {
        if (amount <= 0) return;

        Popup p;
        p.worldPos = worldPos;
        p.timer = 0.0f;

        const std::string str = std::to_string(amount);
        p.digits.reserve(str.size());
        for (char c : str) {
            auto sp = std::make_unique<Sprite>();
            sp->Initialize(dir_ + "0" + std::string(1, c) + ".png"); // 0→00.png …
            sp->SetSize(digitSize_);
            sp->SetAnchorPoint({ 0.0f, 0.5f });
            sp->SetColor(color_); // 赤に染める
            p.digits.push_back(std::move(sp));
        }
        popups_.push_back(std::move(p));
    }

    void Update(float dt) {
        for (auto& p : popups_) {
            p.timer += dt;
        }
        popups_.erase(
            std::remove_if(popups_.begin(), popups_.end(),
                [this](const Popup& p) { return p.timer >= life_; }),
            popups_.end());
    }

    void Draw() {
        if (!camera_) return;

        const Matrix4x4& vp = camera_->GetViewProjectionMatrix();
        const float screenW = static_cast<float>(WinApp::kClientWidth);
        const float screenH = static_cast<float>(WinApp::kClientHeight);

        for (auto& p : popups_) {
            // ワールド座標 → クリップ → NDC → スクリーン
            const Vector3& w = p.worldPos;
            const float cx = w.x * vp.m[0][0] + w.y * vp.m[1][0] + w.z * vp.m[2][0] + vp.m[3][0];
            const float cy = w.x * vp.m[0][1] + w.y * vp.m[1][1] + w.z * vp.m[2][1] + vp.m[3][1];
            const float cw = w.x * vp.m[0][3] + w.y * vp.m[1][3] + w.z * vp.m[2][3] + vp.m[3][3];
            if (cw <= 1e-5f) continue; // カメラ背後は描画しない

            const float ndcX = cx / cw;
            const float ndcY = cy / cw;
            const float sx = (ndcX + 1.0f) * 0.5f * screenW;
            const float sy = (1.0f - ndcY) * 0.5f * screenH;

            const float t = p.timer / life_; // 0〜1

            // 右上オフセット ＋ 時間で少し上へ浮かせる
            const float baseX = sx + offsetX_;
            const float baseY = sy + offsetY_ - floatUp_ * t;

            // 終盤でフェードアウト
            float alpha = 1.0f;
            if (t > fadeStart_) {
                alpha = 1.0f - (t - fadeStart_) / (1.0f - fadeStart_);
            }

            // 桁を左から並べて描画
            for (size_t i = 0; i < p.digits.size(); ++i) {
                const float px = baseX + static_cast<float>(i) * (digitSize_.x + spacing_);
                p.digits[i]->SetPosition({ px, baseY });
                Vector4 c = color_;
                c.w = alpha;
                p.digits[i]->SetColor(c);
                p.digits[i]->Update();
                p.digits[i]->Draw();
            }
        }
    }

private:
    struct Popup {
        Vector3 worldPos{};
        float   timer = 0.0f;
        std::vector<std::unique_ptr<Sprite>> digits;
    };

    Camera* camera_ = nullptr;
    std::vector<Popup> popups_;

    // ---- 調整パラメータ ----
    std::string dir_ = "Resources/number/";       // 数字PNGのフォルダ
    Vector2 digitSize_{ 20.0f, 26.0f };            // 1桁のサイズ
    float   spacing_ = 2.0f;                       // 桁間のすき間
    Vector4 color_{ 1.0f, 0.0f, 0.0f, 1.0f };      // 赤（白テクスチャに乗算）
    float   life_ = 0.8f;                          // 表示時間[秒]
    float   floatUp_ = 50.0f;                      // 寿命の間に上へ浮く量[px]
    float   offsetX_ = 30.0f;                      // 敵中心からの右オフセット[px]
    float   offsetY_ = -50.0f;                     // 敵中心からの上オフセット[px]（負＝上）
    float   fadeStart_ = 0.6f;                     // この進行度を超えたらフェード開始
};
