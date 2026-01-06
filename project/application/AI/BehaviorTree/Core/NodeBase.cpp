#include "NodeBase.h"
#include "BlackBoard.h"

NodeBase::NodeBase(BlackBoard* black_board)
    : mpBlackBoard(black_board) {
}

NodeBase::~NodeBase() = default;

void NodeBase::execute()
{
    // 初回
    if (mNodeResult == NodeResult::Idle) {
        init();
        if (mNodeResult == NodeResult::Idle) {
            mNodeResult = NodeResult::Running;
        }
    }

    // 実行中
    if (mNodeResult == NodeResult::Running) {
        tick();
    }

    // 終了時
    if (mNodeResult == NodeResult::Success ||
        mNodeResult == NodeResult::Fail) {
        finalize();

        // これが無いと「一回成功したら止まる」現象になる
        mNodeResult = NodeResult::Idle;
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
