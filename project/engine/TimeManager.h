#pragma once

/// <summary>
/// ゲーム全体の時間を管理するシングルトン
/// - timeScale : スロー/早送り用の倍率 (1.0 = 通常, 0.2 = 1/5倍速)
/// - deltaTime : timeScale 適用後の Δt （ゲームロジック用）
/// - unscaledDeltaTime : 生の Δt （UIやポストエフェクト等、スロー無視したい用）
/// </summary>
class TimeManager
{
public:
    /// <summary>シングルトンインスタンス取得</summary>
    static TimeManager* GetInstance();

    /// <summary>
    /// 毎フレームの生の経過秒数を渡して更新
    /// - MyGame::Update から 1 フレームにつき 1 回だけ呼ぶ想定
    /// </summary>
    /// <param name="rawDeltaSeconds">前フレームからの経過秒数</param>
    void Update(float rawDeltaSeconds);

    /// <summary>timeScale を設定 (0 以上。0 にすると完全停止)</summary>
    void SetTimeScale(float scale);

    /// <summary>現在の timeScale を取得</summary>
    float GetTimeScale() const { return timeScale_; }

    /// <summary>
    /// timeScale 適用後の Δt を取得
    /// - ゲームロジック / カメラ演出 / 物理更新などはこちらを使う
    /// </summary>
    float GetDeltaTime() const { return deltaTime_; }

    /// <summary>
    /// timeScale 無しの生の Δt を取得
    /// - FPS表示、PostEffect、UIアニメ等に使うとよい
    /// </summary>
    float GetUnscaledDeltaTime() const { return unscaledDeltaTime_; }

    /// <summary>timeScale 適用後の累積時間（ゲーム時間）</summary>
    float GetTime() const { return time_; }

    /// <summary>生の累積時間（実時間）</summary>
    float GetUnscaledTime() const { return unscaledTime_; }

private:
    TimeManager() = default;
    ~TimeManager() = default;

    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

private:
    float timeScale_ = 1.0f;  // 1.0 = 通常
    float deltaTime_ = 0.0f;  // スケール後 Δt
    float unscaledDeltaTime_ = 0.0f;  // スケール前 Δt
    float time_ = 0.0f;  // スケール後 累積時間
    float unscaledTime_ = 0.0f;  // スケール前 累積時間
};
