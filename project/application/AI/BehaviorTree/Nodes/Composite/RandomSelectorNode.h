#pragma once
#include "application/AI/BehaviorTree/Nodes/Composite/SelectorNode.h"
#include <algorithm>
#include <random>
#include <vector>

//======================================================
// RandomSelectorNode
//------------------------------------------------------
// SelectorNode を継承し、実行開始時に子ノードの
// 評価順をランダムにシャッフルする
//
// ・init() でシャッフル済みインデックスリストを生成
// ・get_next_index() でシャッフル順に次の子を返す
// ・毎回 init() が呼ばれるたびに順番が変わる
//   → 弾→召喚 or 召喚→弾 がランダムに決まる
//======================================================
class RandomSelectorNode : public SelectorNode
{
public:
	explicit RandomSelectorNode(BlackBoard* bb)
		: SelectorNode(bb)
		, rng_(std::random_device{}())
	{
	}

protected:
	// 実行開始時にシャッフル
	void init() override
	{
		SelectorNode::init(); // mRunningNodeIndex = 0 などの初期化

		// 子ノードのインデックスをシャッフル
		shuffledIndices_.resize(mChildNodes.size());
		for (int i = 0; i < (int)mChildNodes.size(); ++i) {
			shuffledIndices_[i] = i;
		}
		std::shuffle(shuffledIndices_.begin(), shuffledIndices_.end(), rng_);

		// 最初の子をシャッフル順の先頭に差し替え
		// mRunningNodeIndex はシャッフル配列の何番目か を表す
		shufflePos_ = 0;
		mRunningNodeIndex = shuffledIndices_[shufflePos_];
	}

	// 次に評価する子のインデックスをシャッフル順で返す
	int get_next_index() const override
	{
		int nextPos = shufflePos_ + 1;
		if (nextPos >= (int)shuffledIndices_.size()) {
			// 全部試した → Fail を出すために範囲外を返す
			return (int)mChildNodes.size();
		}
		// tick() 内で mRunningNodeIndex に代入されるため
		// const_cast で shufflePos_ を進める
		const_cast<RandomSelectorNode*>(this)->shufflePos_ = nextPos;
		return shuffledIndices_[nextPos];
	}

private:
	std::mt19937     rng_;
	std::vector<int> shuffledIndices_;
	int              shufflePos_ = 0;
};