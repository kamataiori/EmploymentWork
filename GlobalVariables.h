#pragma once
#include <variant>
#include <map>
#include <string>
#include <vector>
#include <json.hpp>

#include "Vector3.h"

class GlobalVariables
{
private:
	static GlobalVariables* instance_;

	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(GlobalVariables&) = delete;
	GlobalVariables& operator=(GlobalVariables&) = delete;

	// 項目
	using Item = std::variant<int32_t, float, Vector3, bool>;
	// グループ
	using Group = std::vector<std::pair<std::string, Item>>;
public:
	// シングルトンインスタンスの取得
	static GlobalVariables* GetInstance();

	// 更新
	void Update();

	// グループの作成
	void CreateGroup(const std::string& groupName);
	// 項目のアクセッサ
	template <typename T>
	void SetValue(const std::string& groupName, const std::string& key, T value);
	template <typename T>
	inline T GetValue(const std::string& groupName, const std::string& key) const;
	// 項目の追加
	template <typename T>
	void AddValue(const std::string& groupName, const std::string& key, T value);
	// グループの保存
	void SaveFile(const std::string& groupName);
	// ディレクトリの全ファイルの読み込み
	void LoadFiles();
	// ファイルから読み込む
	void LoadFile(const std::string& groupName);

private:
	// using宣言
	using json = nlohmann::json;
	// 全データ
	std::vector<std::pair<std::string, Group>> datas_;
	// 保存先ファイルパス
	const std::string kDirectoryPath = "Resources/globalVariables/";
	// ファイル書き出し
	void OutputToFile(const std::string& groupName, json root);
};

template<typename T>
inline void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, T value)
{
	// std::find_ifを使ってグループを検索
	auto groupIt = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
		return pair.first == groupName;
		});

	// グループが見つからない場合エラーをスロー
	if (groupIt == datas_.end()) {
		throw std::runtime_error("Group not found: " + groupName);
	}

	// グループ内のアイテムを検索
	Group& group = groupIt->second;
	auto itemIt = std::find_if(group.begin(), group.end(), [&key](const auto& pair) {
		return pair.first == key;
		});

	if (itemIt == group.end()) {
		// アイテムが見つからない場合、新しいアイテムを追加
		group.emplace_back(key, value);
	}
	else {
		// 既存アイテムの値を更新
		itemIt->second = value;
	}
}

template<typename T>
inline T GlobalVariables::GetValue(const std::string& groupName, const std::string& key) const
{
	// グループの存在確認
	auto groupIt = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
		return pair.first == groupName;
		});
	if (groupIt == datas_.end()) {
		throw std::runtime_error("Group not found: " + groupName);
	}

	// 項目の存在確認
	const Group& group = groupIt->second;
	auto itemIt = std::find_if(group.begin(), group.end(), [&key](const auto& pair) {
		return pair.first == key;
		});
	if (itemIt == group.end()) {
		throw std::runtime_error("Item not found in group '" + groupName + "': " + key);
	}

	// 項目の型を確認して値を取得
	const Item& item = itemIt->second;
	if (std::holds_alternative<T>(item)) {
		return std::get<T>(item);
	}
	else {
		throw std::runtime_error("Type mismatch when getting item '" + key + "' in group '" + groupName + "'");
	}
}

template<typename T>
inline void GlobalVariables::AddValue(const std::string& groupName, const std::string& key, T value)
{
	// グループが存在しない場合は作成
	auto groupIt = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
		return pair.first == groupName;
		});
	if (groupIt == datas_.end()) {
		CreateGroup(groupName);
		groupIt = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
			return pair.first == groupName;
			});
	}

	// グループの参照を取得
	Group& group = groupIt->second;

	// 項目が存在しない場合のみ、SetValueを使用して追加
	auto itemIt = std::find_if(group.begin(), group.end(), [&key](const auto& pair) {
		return pair.first == key;
		});
	if (itemIt == group.end()) {
		SetValue(groupName, key, value);
	}
}