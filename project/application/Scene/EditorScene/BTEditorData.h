#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <json.hpp>

//======================================================
// BTEditorData.h
//
// BT エディターが扱うノード・リンクのデータ構造。
// ゲームランタイムの INode 階層とは独立したエディター専用表現。
// JSON (nlohmann/json) でシリアライズ / デシリアライズする。
//======================================================

namespace BTEditor {

	//------------------------------------------------------
	// ノード種別
	//------------------------------------------------------
	enum class NodeKind : int
	{
		Root = 0,  // ルート（常に1個）
		Sequence = 1,
		Selector = 2,
		Decorator = 3,  // Inverter など
		Leaf = 4,  // 汎用 Leaf / ExecuteStateLeaf
	};

	inline const char* NodeKindName(NodeKind k)
	{
		switch (k) {
		case NodeKind::Root:      return "Root";
		case NodeKind::Sequence:  return "Sequence";
		case NodeKind::Selector:  return "Selector";
		case NodeKind::Decorator: return "Decorator";
		case NodeKind::Leaf:      return "Leaf";
		default:                  return "Unknown";
		}
	}

	//------------------------------------------------------
	// Leaf が実行するステート種別
	//------------------------------------------------------
	enum class LeafStateType : int
	{
		None = 0,
		ChargeDash = 1,
		SummonMinion = 2,
		FindTarget = 3,
		ChaseTarget = 4,
		NearIdle = 5,
		StayHome = 6,
		IsTargetFar = 7,  // Condition (bool 判定 Leaf)
		ShootSplitBullet = 8, // 分裂弾発射
		ThrowBigBullet = 9, // 大弾投げ
		IsPhase2 = 10, // HP50%以下か判定
		IsPhase3 = 11, // HP25%以下か判定
		NotLastAction = 12, // 前回と同じ攻撃を禁止
		IsAngry = 13, // 怒り状態（HP50%以下）か判定
		ShootSplitBulletAngry = 14, // 怒り時の分裂弾（8発）
		Custom = 99, // 将来拡張用
	};

	inline const char* LeafStateTypeName(LeafStateType t)
	{
		switch (t) {
		case LeafStateType::None:          return "None";
		case LeafStateType::ChargeDash:    return "ChargeDash";
		case LeafStateType::SummonMinion:  return "SummonMinion";
		case LeafStateType::FindTarget:    return "FindTarget";
		case LeafStateType::ChaseTarget:   return "ChaseTarget";
		case LeafStateType::NearIdle:      return "NearIdle";
		case LeafStateType::StayHome:      return "StayHome";
		case LeafStateType::IsTargetFar:   return "IsTargetFar";
		case LeafStateType::ShootSplitBullet: return "ShootSplitBullet";
		case LeafStateType::ThrowBigBullet:   return "ThrowBigBullet";
		case LeafStateType::IsPhase2:         return "IsPhase2";
		case LeafStateType::IsPhase3:         return "IsPhase3";
		case LeafStateType::NotLastAction:         return "NotLastAction";
		case LeafStateType::IsAngry:               return "IsAngry";
		case LeafStateType::ShootSplitBulletAngry: return "ShootSplitBulletAngry";
		case LeafStateType::Custom:        return "Custom";
		default:                           return "Unknown";
		}
	}

	//------------------------------------------------------
	// ChargeDashParam（JSON↔ゲームランタイムで共用）
	//------------------------------------------------------
	struct ChargeDashParamData
	{
		float chargeTime = 0.8f;
		float dashDistance = 20.0f;
		float dashSpeed = 25.0f;
		float recoverTime = 0.4f;
		float cooldownTime = 1.2f;
		float turnLerp = 0.25f;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
		ChargeDashParamData,
		chargeTime, dashDistance, dashSpeed,
		recoverTime, cooldownTime, turnLerp
	)

		//------------------------------------------------------
		// ノードパラメータ（種別ごとに使うフィールドが異なる）
		//------------------------------------------------------
		struct NodeParam
	{
		// --- Leaf 共通 ---
		LeafStateType stateType = LeafStateType::None;

		// --- ChargeDash 専用 ---
		ChargeDashParamData chargeDash{};

		// --- IsTargetFar / ChaseTarget 専用 ---
		float thresholdDistance = 10.0f;

		// --- Decorator 専用 ---
		std::string decoratorType = "Inverter"; // 今後拡張

		// --- NotLastAction 用：禁止する攻撃名 ---
		std::string lastActionName; // "ChargeDash" など

		// --- カスタム Leaf 名（Custom 用） ---
		std::string customName;
	};

	inline void to_json(nlohmann::json& j, const NodeParam& p)
	{
		j["stateType"] = static_cast<int>(p.stateType);
		j["chargeDash"] = p.chargeDash;
		j["thresholdDistance"] = p.thresholdDistance;
		j["decoratorType"] = p.decoratorType;
		j["lastActionName"] = p.lastActionName;
		j["customName"] = p.customName;
	}

	inline void from_json(const nlohmann::json& j, NodeParam& p)
	{
		p.stateType = static_cast<LeafStateType>(j.value("stateType", 0));
		p.chargeDash = j.value("chargeDash", ChargeDashParamData{});
		p.thresholdDistance = j.value("thresholdDistance", 10.0f);
		p.decoratorType = j.value("decoratorType", std::string("Inverter"));
		p.lastActionName = j.value("lastActionName", std::string(""));
		p.customName = j.value("customName", std::string(""));
	}

	//------------------------------------------------------
	// エディター上の1ノード
	//------------------------------------------------------
	struct EditorNode
	{
		int       id = -1;
		NodeKind  kind = NodeKind::Leaf;
		std::string label;          // ユーザー向け表示名
		float posX = 0.0f;
		float posY = 0.0f;
		NodeParam param;

		// imnodes のピン ID
		// 入力ピン (親接続用)  : id * 3 + 0
		// 出力ピン (子接続用)  : id * 3 + 1
		// imnodes nodeId       : id * 3 + 2
		int InputPinId()  const { return id * 3 + 0; }
		int OutputPinId() const { return id * 3 + 1; }
		int ImNodeId()    const { return id * 3 + 2; }
	};

	inline void to_json(nlohmann::json& j, const EditorNode& n)
	{
		j["id"] = n.id;
		j["kind"] = static_cast<int>(n.kind);
		j["label"] = n.label;
		j["posX"] = n.posX;
		j["posY"] = n.posY;
		j["param"] = n.param;
	}

	inline void from_json(const nlohmann::json& j, EditorNode& n)
	{
		n.id = j.value("id", -1);
		n.kind = static_cast<NodeKind>(j.value("kind", 0));
		n.label = j.value("label", std::string(""));
		n.posX = j.value("posX", 0.0f);
		n.posY = j.value("posY", 0.0f);
		n.param = j.value("param", NodeParam{});
	}

	//------------------------------------------------------
	// 2ノード間のリンク（親→子 の有向辺）
	//------------------------------------------------------
	struct EditorLink
	{
		int id = -1;
		int fromNodeId = -1;  // 親ノードID
		int toNodeId = -1;  // 子ノードID
	};

	inline void to_json(nlohmann::json& j, const EditorLink& l)
	{
		j["id"] = l.id;
		j["fromNodeId"] = l.fromNodeId;
		j["toNodeId"] = l.toNodeId;
	}

	inline void from_json(const nlohmann::json& j, EditorLink& l)
	{
		l.id = j.value("id", -1);
		l.fromNodeId = j.value("fromNodeId", -1);
		l.toNodeId = j.value("toNodeId", -1);
	}

	//------------------------------------------------------
	// グラフ全体
	//------------------------------------------------------
	struct BTGraph
	{
		std::vector<EditorNode> nodes;
		std::vector<EditorLink> links;
		int nextId = 1;   // 発行済み最大ID+1

		void Clear()
		{
			nodes.clear();
			links.clear();
			nextId = 1;
		}

		int NewId() { return nextId++; }

		EditorNode* FindNode(int id)
		{
			for (auto& n : nodes) if (n.id == id) return &n;
			return nullptr;
		}

		const EditorNode* FindNode(int id) const
		{
			for (auto& n : nodes) if (n.id == id) return &n;
			return nullptr;
		}

		// 親ノードIDを返す（リンクの fromNodeId）
		// 存在しなければ -1
		int ParentOf(int nodeId) const
		{
			for (auto& l : links) {
				if (l.toNodeId == nodeId) return l.fromNodeId;
			}
			return -1;
		}

		// 子ノードIDを昇順で返す
		std::vector<int> ChildrenOf(int nodeId) const
		{
			std::vector<int> result;
			for (auto& l : links) {
				if (l.fromNodeId == nodeId) result.push_back(l.toNodeId);
			}
			return result;
		}
	};

	inline void to_json(nlohmann::json& j, const BTGraph& g)
	{
		j["nodes"] = g.nodes;
		j["links"] = g.links;
		j["nextId"] = g.nextId;
	}

	inline void from_json(const nlohmann::json& j, BTGraph& g)
	{
		g.nodes = j.value("nodes", std::vector<EditorNode>{});
		g.links = j.value("links", std::vector<EditorLink>{});
		g.nextId = j.value("nextId", 1);
	}

} // namespace BTEditor