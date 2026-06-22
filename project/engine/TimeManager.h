#pragma once

//======================================================================
// ヒットストップ（被弾の瞬間に時間を一瞬だけ止める/遅くする「手応え」演出）
//----------------------------------------------------------------------
// アクションゲームの打撃感は、当たった瞬間に時間を僅かに止めることで生まれる。
// ここではその1回分を、実際のアクションゲームで使われる代表的な手法が
// すべて表現できるパラメータとして定義する。
//======================================================================

/// <summary>
/// ヒットストップ1回分のパラメータ。
/// - minScale       : 「止め」中の時間倍率。0.0 = 完全停止(フリーズ)、0.2 = スロー演出
/// - freezeSeconds  : minScale を維持する時間（＝“止め”そのものの長さ）
/// - recoverSeconds : 通常速度へ戻すまでの復帰時間（カクッと戻さず滑らかに加速）
/// </summary>
struct HitStopRequest
{
    float minScale = 0.0f;       // 停止中の時間倍率（0=完全フリーズ）
    float freezeSeconds = 0.05f; // 止めを維持する秒数
    float recoverSeconds = 0.04f;// 通常速度へ戻す復帰秒数
};

/// <summary>
/// よく使うヒットストップの「プリセット」。
/// 実際のアクションゲームで使われる代表的なパターンを名前付きで提供する。
/// 呼び出し側は意味のある名前を渡すだけでよく、数値の調整はここに集約される。
/// </summary>
namespace HitStopPreset
{
    //--- 通常ヒット（Light）：軽い完全フリーズ。全ての攻撃ヒットの基本の手応え ---
    inline constexpr float kLightMinScale = 0.0f;        // 完全停止
    inline constexpr float kLightFreezeSeconds = 0.05f;  // 約3フレーム止める
    inline constexpr float kLightRecoverSeconds = 0.04f; // 滑らかに復帰

    //--- 撃破・トドメ（Heavy）：長めの完全フリーズ＋強い余韻 ---
    inline constexpr float kHeavyMinScale = 0.0f;        // 完全停止
    inline constexpr float kHeavyFreezeSeconds = 0.12f;  // しっかり止めて存在感を出す
    inline constexpr float kHeavyRecoverSeconds = 0.10f; // 余韻を残してゆっくり復帰

    //--- 大ダメージ（SlowMotion）：完全停止せず低速で“ねっとり”見せる ---
    inline constexpr float kSlowMinScale = 0.15f;        // 完全には止めない
    inline constexpr float kSlowFreezeSeconds = 0.10f;   // 低速を維持する時間
    inline constexpr float kSlowRecoverSeconds = 0.18f;  // 長めにかけて通常へ戻す

    /// <summary>通常ヒット用：軽い完全フリーズ</summary>
    inline HitStopRequest Light()
    {
        return HitStopRequest{ kLightMinScale, kLightFreezeSeconds, kLightRecoverSeconds };
    }

    /// <summary>撃破・トドメ用：長めの完全フリーズ＋強い余韻</summary>
    inline HitStopRequest Heavy()
    {
        return HitStopRequest{ kHeavyMinScale, kHeavyFreezeSeconds, kHeavyRecoverSeconds };
    }

    /// <summary>大ダメージ用：完全停止しないスローモーション式</summary>
    inline HitStopRequest SlowMotion()
    {
        return HitStopRequest{ kSlowMinScale, kSlowFreezeSeconds, kSlowRecoverSeconds };
    }
}

/// <summary>
/// ゲーム全体の時間を管理するシングルトン
/// - baseTimeScale : スロー/早送り/ポーズ用の基準倍率 (1.0 = 通常, 0.0 = 停止)
/// - hitStop       : 上記の基準倍率に“乗算で重ねる”一瞬の止め演出（ヒットストップ）
/// - deltaTime     : 上記すべてを適用した後の Δt （ゲームロジック用）
/// - unscaledDeltaTime : 生の Δt （UIやポストエフェクト等、スロー無視したい用）
///
/// ヒットストップを基準倍率とは別レイヤーにすることで、
/// ポーズ(0.0)やボス撃破スロー(0.1)など別用途で設定された baseTimeScale を壊さず、
/// その上から打撃の瞬間だけ“止め”を重ねられる。
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

    /// <summary>基準となる timeScale を設定 (0 以上。0 にすると完全停止)</summary>
    void SetTimeScale(float scale);

    /// <summary>現在の基準 timeScale を取得（ヒットストップは含まない）</summary>
    float GetTimeScale() const { return baseTimeScale_; }

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

    //=== ヒットストップ ===

    /// <summary>
    /// ヒットストップを要求する（攻撃が当たった瞬間に呼ぶ）。
    /// 連続ヒット時は「より長く止まる」要求のときだけ採用し、
    /// 弱い要求で進行中の演出を途中で打ち切らないようにする。
    /// </summary>
    /// <param name="request">止めの強さ・長さ（HitStopPreset を使うと簡単）</param>
    void RequestHitStop(const HitStopRequest& request);

    /// <summary>現在ヒットストップ中か</summary>
    bool IsHitStopActive() const { return hitStopState_ != HitStopState::Inactive; }

private:
    TimeManager() = default;
    ~TimeManager() = default;

    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    /// <summary>
    /// ヒットストップの内部状態を1フレーム分進め、基準倍率に掛ける係数(0..1)を返す。
    /// 実時間(unscaled)で進めるので、止めている最中でも必ず時間切れになる。
    /// </summary>
    float UpdateHitStop(float unscaledDeltaSeconds);

    /// <summary>現在のヒットストップ演出の残り秒数（優先度判定に使う）</summary>
    float HitStopRemainingSeconds() const;

private:
    float baseTimeScale_ = 1.0f;      // 1.0 = 通常（ポーズ/スロー等の基準）
    float deltaTime_ = 0.0f;          // 全適用後 Δt
    float unscaledDeltaTime_ = 0.0f;  // スケール前 Δt
    float time_ = 0.0f;               // 全適用後 累積時間
    float unscaledTime_ = 0.0f;       // スケール前 累積時間

    // --- ヒットストップ状態機械 ---
    enum class HitStopState
    {
        Inactive,   // 何もしていない
        Freezing,   // minScale を維持して「止めて」いる
        Recovering, // minScale → 1.0 へ滑らかに復帰中
    };

    HitStopState hitStopState_ = HitStopState::Inactive;
    HitStopRequest hitStopRequest_{}; // 現在進行中の要求
    float hitStopElapsed_ = 0.0f;     // 現フェーズの経過秒
};
