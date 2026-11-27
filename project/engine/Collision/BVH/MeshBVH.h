#pragma once
#include "Struct.h"
#include <vector>
#include <functional>

//======================================================
// MeshBVH
//------------------------------------------------------
// 三角形メッシュのAABB木（Bounding Volume Hierarchy）。
// 静的ジオメトリ（ステージ）向けに、ワールド空間へ焼き込んだ
// 三角形群から一度だけ構築して使い回す。
//
// ・Build   : ワールド空間の三角形からAABB木を構築（軸最長のmedian split）
// ・QueryOverlap : 問い合わせAABBに重なり得る三角形だけをcbに渡す
//   （総当たり O(N) を平均 O(log N) に落とすのが目的）
//======================================================
class MeshBVH {
public:
    // ワールド空間の三角形群からBVHを構築する（既存データは破棄）
    void Build(std::vector<Triangle> triangles);

    // box(ワールドAABB)に重なり得る三角形をcbへ渡す。
    // cbがtrueを返したら走査を打ち切る（「1個当たれば十分」な検出用途向け）。
    // 戻り値：cbが一度でもtrueを返したか。
    bool QueryOverlap(const AABB& box, const std::function<bool(const Triangle&)>& cb) const;

    bool Empty() const { return triangles_.empty(); }
    size_t TriangleCount() const { return triangles_.size(); }
    const AABB& RootBounds() const { return rootBounds_; }

private:
    struct Node {
        AABB bounds{};   // このノードが包むAABB
        // 内部ノード：子のindex。ノードはDFS順に確保されるので left の直後が right とは限らない
        //（左部分木を全部確保した後ろに right が来る）。必ず両方を保持すること。
        int  left = -1;
        int  right = -1;
        int  start = 0;  // 葉：triangles_ 内の開始index
        int  count = 0;  // 葉：三角形数（count>0 が葉の印）
    };

    // [start, start+count) の三角形からノードを構築し、そのindexを返す
    int BuildRecursive(int start, int count);

    static AABB TriangleBounds(const Triangle& t);
    static AABB Merge(const AABB& a, const AABB& b);
    static bool Overlap(const AABB& a, const AABB& b);
    static Vector3 Centroid(const Triangle& t);

    std::vector<Triangle> triangles_; // 葉の順に並べ替え済み
    std::vector<Node>     nodes_;
    AABB rootBounds_{};

    // 1葉に収める最大三角形数
    static constexpr int kLeafSize = 4;
};
