#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "SceneTransitionTypes.h"
#include "SceneManager.h"

class UIManager;
class ISceneTransitionState;

// 遷移演出の生成と実行を担当するクラス
// SceneManagerはここに「要求」を投げるだけにする
class SceneTransitionService {
public:
    SceneTransitionService(); // レジストリをここで構築
    ~SceneTransitionService();

    // UIManagerを注入（アプリ起動時に1回でOK）
    void SetUIManager(UIManager* uiManager) { uiManager_ = uiManager; }

    // 遷移中かどうか（入力ロックなどに使える）
    bool IsTransitioning() const { return isTransitioning_; }

    // 遷移開始
    void StartTransition(
        const std::string& nextSceneName,
        const TransitionRequest& req,
        std::function<void(const std::string&)> onSwitch,
        std::function<void()> onFinish
    );

private:
    // TransitionType をハッシュに使うためのハッシュ
    struct EnumHash {
        template <class T>
        std::size_t operator()(T v) const noexcept {
            return static_cast<std::size_t>(v);
        }
    };

    // type -> state factory
    using StateFactory = std::function<std::unique_ptr<ISceneTransitionState>()>;
    std::unordered_map<TransitionType, StateFactory, EnumHash> factory_;

    // 現在のState
    std::unique_ptr<ISceneTransitionState> state_;

private:
    UIManager* uiManager_ = nullptr;
    bool isTransitioning_ = false;
};
