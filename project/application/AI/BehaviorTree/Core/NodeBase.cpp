#include "NodeBase.h"
#include "BlackBoard.h"

NodeBase::NodeBase(BlackBoard* black_board)
    : mpBlackBoard(black_board) {
}

NodeBase::~NodeBase() = default;

void NodeBase::execute()
{
    // 初回（未実行なら init）
    if (mNodeResult == NodeResult::Idle) {
        init();

        // init側で結果を決めなかった場合、最低でもRunningにする
        if (mNodeResult == NodeResult::Idle) {
            mNodeResult = NodeResult::Running;
        }
    }

    // 実行中
    if (mNodeResult == NodeResult::Running) {
        tick();
    }

    // 終了時（Success/Fail）
    if (mNodeResult == NodeResult::Success ||
        mNodeResult == NodeResult::Fail) {
        finalize();

        // 直書きせず Reset 経由で戻す（将来拡張に強い）
        Reset();
    }
}

NodeResult NodeBase::get_node_result() const
{
    return mNodeResult;
}

void NodeBase::Reset()
{
    mNodeResult = NodeResult::Idle;
}

void NodeBase::init()
{
    mNodeResult = NodeResult::Running;
}

void NodeBase::tick() {}
void NodeBase::finalize() {}
