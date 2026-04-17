#include "NodeBase.h"
#include "BlackBoard.h"

NodeBase::NodeBase(BlackBoard* black_board)
	: mpBlackBoard(black_board) {
}

NodeBase::~NodeBase() = default;

void NodeBase::execute()
{
	// 前フレームで Success/Fail だった場合は今フレームの冒頭でリセット
	// （こうすることで同フレーム内に get_node_result() で結果を読める）
	if (mNodeResult == NodeResult::Success ||
		mNodeResult == NodeResult::Fail) {
		Reset();
	}

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

	// 終了時（Success/Fail）→ finalize だけ呼ぶ。Reset は次フレームに持ち越す
	if (mNodeResult == NodeResult::Success ||
		mNodeResult == NodeResult::Fail) {
		finalize();
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