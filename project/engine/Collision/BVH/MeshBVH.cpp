#include "MeshBVH.h"
#include <algorithm>
#include <limits>

namespace {
    constexpr float kInf = std::numeric_limits<float>::max();
}

void MeshBVH::Build(std::vector<Triangle> triangles)
{
    triangles_ = std::move(triangles);
    nodes_.clear();
    if (triangles_.empty()) {
        rootBounds_ = AABB{ {0,0,0}, {0,0,0}, 0 };
        return;
    }
    // ノードは最大で 2*葉数 程度。事前確保して再確保を避ける
    nodes_.reserve(triangles_.size() * 2);
    BuildRecursive(0, static_cast<int>(triangles_.size()));
    rootBounds_ = nodes_.empty() ? AABB{} : nodes_[0].bounds;
}

int MeshBVH::BuildRecursive(int start, int count)
{
    // このノードを先に確保（indexを固定するため）
    const int nodeIndex = static_cast<int>(nodes_.size());
    nodes_.push_back(Node{});

    // 範囲全体のAABB
    AABB bounds = TriangleBounds(triangles_[start]);
    for (int i = start + 1; i < start + count; ++i) {
        bounds = Merge(bounds, TriangleBounds(triangles_[i]));
    }

    // 葉にできるなら葉にする
    if (count <= kLeafSize) {
        Node leaf{};
        leaf.bounds = bounds;
        leaf.left = -1;
        leaf.start = start;
        leaf.count = count;
        nodes_[nodeIndex] = leaf;
        return nodeIndex;
    }

    // 最長軸を分割軸に選ぶ
    const Vector3 extent = bounds.max - bounds.min;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > ((axis == 0) ? extent.x : extent.y)) axis = 2;

    // 分割軸の重心でmedian split（範囲の中央で並べ替え）
    Triangle* first = triangles_.data() + start;
    Triangle* mid = first + count / 2;
    Triangle* last = triangles_.data() + start + count;
    std::nth_element(first, mid, last,
        [axis](const Triangle& a, const Triangle& b) {
            const Vector3 ca = Centroid(a);
            const Vector3 cb = Centroid(b);
            const float va = (axis == 0) ? ca.x : (axis == 1) ? ca.y : ca.z;
            const float vb = (axis == 0) ? cb.x : (axis == 1) ? cb.y : cb.z;
            return va < vb;
        });

    const int leftCount = count / 2;

    // 子を構築。左部分木を全て確保した後ろに右が来るので、戻り値を両方保持する
    const int leftChild = BuildRecursive(start, leftCount);
    const int rightChild = BuildRecursive(start + leftCount, count - leftCount);

    Node inner{};
    inner.bounds = bounds;
    inner.left = leftChild;
    inner.right = rightChild;
    inner.count = 0; // 内部ノード
    nodes_[nodeIndex] = inner;
    return nodeIndex;
}

bool MeshBVH::QueryOverlap(const AABB& box, const std::function<bool(const Triangle&)>& cb) const
{
    if (nodes_.empty()) return false;

    bool anyHit = false;
    // 明示スタックで木を降りる（再帰を避ける）
    constexpr int kMaxStack = 64;
    int stack[kMaxStack];
    int sp = 0;
    stack[sp++] = 0;

    while (sp > 0) {
        const Node& node = nodes_[stack[--sp]];
        if (!Overlap(node.bounds, box)) {
            continue;
        }
        if (node.count > 0) {
            // 葉：候補三角形を精密判定へ
            for (int i = node.start; i < node.start + node.count; ++i) {
                const Triangle& tri = triangles_[i];
                if (!Overlap(TriangleBounds(tri), box)) continue;
                if (cb(tri)) {
                    anyHit = true;
                    return anyHit; // 早期終了（1個当たれば十分）
                }
            }
        }
        else {
            // 内部：子を積む（スタック溢れ防止のガード付き）
            if (node.left >= 0 && sp < kMaxStack)  stack[sp++] = node.left;
            if (node.right >= 0 && sp < kMaxStack) stack[sp++] = node.right;
        }
    }
    return anyHit;
}

AABB MeshBVH::TriangleBounds(const Triangle& t)
{
    AABB b{};
    b.min = t.vertices[0];
    b.max = t.vertices[0];
    for (int i = 1; i < 3; ++i) {
        b.min.x = std::min(b.min.x, t.vertices[i].x);
        b.min.y = std::min(b.min.y, t.vertices[i].y);
        b.min.z = std::min(b.min.z, t.vertices[i].z);
        b.max.x = std::max(b.max.x, t.vertices[i].x);
        b.max.y = std::max(b.max.y, t.vertices[i].y);
        b.max.z = std::max(b.max.z, t.vertices[i].z);
    }
    b.color = 0;
    return b;
}

AABB MeshBVH::Merge(const AABB& a, const AABB& b)
{
    AABB r{};
    r.min.x = std::min(a.min.x, b.min.x);
    r.min.y = std::min(a.min.y, b.min.y);
    r.min.z = std::min(a.min.z, b.min.z);
    r.max.x = std::max(a.max.x, b.max.x);
    r.max.y = std::max(a.max.y, b.max.y);
    r.max.z = std::max(a.max.z, b.max.z);
    r.color = 0;
    return r;
}

bool MeshBVH::Overlap(const AABB& a, const AABB& b)
{
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}

Vector3 MeshBVH::Centroid(const Triangle& t)
{
    return (t.vertices[0] + t.vertices[1] + t.vertices[2]) * (1.0f / 3.0f);
}
