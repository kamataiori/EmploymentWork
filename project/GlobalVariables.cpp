#include "GlobalVariables.h"

#include <windows.h>
#include <filesystem>
#include <fstream>

#include <externals/imgui/imgui.h>

GlobalVariables* GlobalVariables::instance_ = nullptr;

void GlobalVariables::SaveFile(const std::string& groupName)
{
    // グループを検索
    auto itGroup = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
        return pair.first == groupName;
        });

    // グループが見つからない場合エラーをスロー
    if (itGroup == datas_.end()) {
        throw std::runtime_error("Group not found: " + groupName);
    }

    // JSONオブジェクトを作成
    nlohmann::json root = nlohmann::json::object();
    nlohmann::json groupJson = nlohmann::json::array(); // 配列で順序を明示

    // グループ内のアイテムを順序通りに追加
    for (const auto& itItem : itGroup->second) {
        nlohmann::json itemJson = nlohmann::json::object();
        const std::string& itemName = itItem.first;
        const Item& item = itItem.second;

        if (std::holds_alternative<int32_t>(item)) {
            itemJson[itemName] = std::get<int32_t>(item);
        }
        else if (std::holds_alternative<float>(item)) {
            itemJson[itemName] = std::get<float>(item);
        }
        else if (std::holds_alternative<Vector3>(item)) {
            Vector3 value = std::get<Vector3>(item);
            itemJson[itemName] = { value.x, value.y, value.z };
        }
        else if (std::holds_alternative<bool>(item)) {
            itemJson[itemName] = std::get<bool>(item);
        }

        // 順序を守るために配列に追加
        groupJson.push_back(itemJson);
    }

    // グループデータをルートに追加
    root[groupName] = groupJson;

    // ファイル出力処理
    OutputToFile(groupName, root);
}

void GlobalVariables::OutputToFile(const std::string& groupName, json root)
{
    // ディレクトリが無ければ作成
    std::filesystem::path dir(kDirectoryPath);
    if (!std::filesystem::exists(kDirectoryPath)) {
        std::filesystem::create_directory(kDirectoryPath);
    }
    // 書き込むjsonファイルのフルパスを作成
    std::string filePath = kDirectoryPath + groupName + ".json";
    // 書き込み用ファイルストリーム
    std::ofstream ofs;
    // ファイルを書き込み用に開く
    ofs.open(filePath);

    // ファイルオープンに失敗
    if (ofs.fail()) {
        std::string message = "Failed open data fail for write.";
        MessageBoxA(nullptr, message.c_str(), "GrobalVariables", 0);
        assert(0);
        return;
    }

    // ファイルにjson文字列を書き込む
    ofs << std::setw(4) << root << std::endl;
    // ファイルを閉じる
    ofs.close();
}

void GlobalVariables::LoadFiles()
{
    // ディレクトリを確認
    std::filesystem::path dir(kDirectoryPath);
    if (!std::filesystem::exists(kDirectoryPath)) {
        return;
    }
    std::filesystem::directory_iterator dir_it(kDirectoryPath);
    for (const std::filesystem::directory_entry& entry : dir_it) {
        // ファイルパスを取得
        const std::filesystem::path& filePath = entry.path();
        // ファイル拡張子を取得
        std::string extension = filePath.extension().string();
        // .jsonファイル以外はスキップ
        if (extension.compare(".json") != 0) {
            continue;
        }
        // ファイル読み込み
        LoadFile(filePath.stem().string());
    }

}

void GlobalVariables::LoadFile(const std::string& groupName)
{
    // 読み込むJSONファイルのパスを作成
    std::string filePath = kDirectoryPath + groupName + ".json";
    std::ifstream ifs(filePath);

    if (ifs.fail()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    nlohmann::json root;
    //nlohmann::ordered_json root;
    ifs >> root;
    ifs.close();

    // グループの存在確認
    if (root.find(groupName) == root.end()) {
        throw std::runtime_error("Group not found in JSON file: " + groupName);
    }

    // JSONデータを取得
    const auto& groupData = root[groupName];

    // グループを作成
    CreateGroup(groupName);
    auto groupIt = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
        return pair.first == groupName;
        });

    if (groupIt == datas_.end()) {
        throw std::runtime_error("Failed to create group: " + groupName);
    }

    Group& group = groupIt->second;

    // JSON配列を順序通りに解析
    for (const auto& item : groupData) {
        if (!item.is_object()) {
            throw std::runtime_error("Invalid item format in JSON file: " + groupName);
        }

        // 各オブジェクトのキーと値を取得
        for (auto it = item.begin(); it != item.end(); ++it) {
            const std::string& key = it.key();

            if (it.value().is_number_integer()) {
                AddValue(groupName, key, it.value().get<int32_t>());
            }
            else if (it.value().is_number_float()) {
                AddValue(groupName, key, static_cast<float>(it.value().get<double>()));
            }
            else if (it.value().is_array() && it.value().size() == 3) {
                Vector3 value{ it.value()[0], it.value()[1], it.value()[2] };
                AddValue(groupName, key, value);
            }
            else if (it.value().is_boolean()) {
                AddValue(groupName, key, it.value().get<bool>());
            }
            else {
                throw std::runtime_error("Unsupported value type for key: " + key);
            }
        }
    }
}

GlobalVariables* GlobalVariables::GetInstance()
{
    if (instance_ == nullptr) {
        instance_ = new GlobalVariables;
    }
    return instance_;
}

void GlobalVariables::Update()
{
#ifdef _DEBUG

    static char searchBuffer[128] = ""; // 検索文字列用バッファ

    if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    // 検索バーを追加
    ImGui::InputText("Search Groups", searchBuffer, sizeof(searchBuffer));

    // 各グループ
    for (auto& groupPair : datas_) {
        const std::string& groupName = groupPair.first;
        Group& group = groupPair.second;

        // TreeNode（グループ名）だけを検索
        if (strlen(searchBuffer) > 0 && groupName.find(searchBuffer) == std::string::npos) {
            continue;
        }

        if (ImGui::TreeNode(groupName.c_str())) {
            for (auto& itemPair : group) {
                const std::string& itemName = itemPair.first;
                Item& item = itemPair.second;

                // int32_t
                if (std::holds_alternative<int32_t>(item)) {
                    int32_t* ptr = std::get_if<int32_t>(&item);
                    ImGui::SliderInt(itemName.c_str(), ptr, 0, 100);
                }
                // float
                if (std::holds_alternative<float>(item)) {
                    float* ptr = std::get_if<float>(&item);
                    ImGui::DragFloat(itemName.c_str(), ptr, 0.01f);
                }
                // Vector3
                if (std::holds_alternative<Vector3>(item) && itemName != "color") {
                    Vector3* ptr = std::get_if<Vector3>(&item);
                    ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), 0.01f);
                }
                if (std::holds_alternative<Vector3>(item) && itemName == "color") {
                    Vector3* ptr = std::get_if<Vector3>(&item);
                    ImGui::ColorEdit3(itemName.c_str(), reinterpret_cast<float*>(ptr));
                }
                // bool 
                if (std::holds_alternative<bool>(item)) {
                    bool* ptr = std::get_if<bool>(&item);
                    ImGui::Checkbox(itemName.c_str(), ptr);
                }
            }
            if (ImGui::Button("Export")) {
                SaveFile(groupName);
                std::string message = std::format("{}.json saved.", groupName);
                MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();

#endif // _DEBUG
}

void GlobalVariables::CreateGroup(const std::string& groupName)
{
    // std::find_ifを使ってグループを検索
    auto it = std::find_if(datas_.begin(), datas_.end(), [&groupName](const auto& pair) {
        return pair.first == groupName;
        });

    // 存在しない場合、新しいグループを追加
    if (it == datas_.end()) {
        datas_.emplace_back(groupName, Group{});
    }
}