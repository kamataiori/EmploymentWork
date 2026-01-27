#pragma once
#include <functional>
#include <string>
#include "SceneTransitionTypes.h"
#include "SceneManager.h"

class UIManager;

// 遷移演出の生成と実行を担当するクラス
// SceneManagerはここに「要求」を投げるだけにする
class SceneTransitionService {
public:
    SceneTransitionService() = default;

    // UIManagerを注入（アプリ起動時に1回でOK）
    void SetUIManager(UIManager* uiManager) { uiManager_ = uiManager; }

    // 遷移中かどうか（入力ロックなどに使える）
    bool IsTransitioning() const { return isTransitioning_; }

    // 遷移開始
    // nextSceneName : 次シーン名
    // req           : 演出パラメータ
    // onSwitch      : 暗転完了で呼ぶ（通常SceneManager::ChangeScene）
    // onFinish      : 明転完了で呼ぶ（通常フラグ解除）
    void StartTransition(
        const std::string& nextSceneName,
        const TransitionRequest& req,
        std::function<void(const std::string&)> onSwitch,
        std::function<void()> onFinish
    );

private:
    // 遷移UIを生成してUIManagerにAddする（種類ごとにここを増やす）
    void CreateAndEnqueueFade(
        const TransitionRequest& req,
        std::function<void()> onSwitchOnce,
        std::function<void()> onFinishOnce
    );

    // 上下シャッター
    void CreateAndEnqueueShutter(
        const TransitionRequest& req,
        std::function<void()> onSwitchOnce,
        std::function<void()> onFinishOnce
    );

private:
    UIManager* uiManager_ = nullptr;
    bool isTransitioning_ = false;
};
