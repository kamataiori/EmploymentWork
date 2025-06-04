#include "JsonLevelLoader.h"

LevelData* JsonLevelLoader::LevelDataLoader(const std::string& fileName)
{
    const std::string fullpath = kDefaultBaseDirectory + fileName + kExtension;

    std::ifstream file(fullpath);
    if (file.fail()) {
        assert(0 && "JSONファイルを開けませんでした");
    }

    nlohmann::json deserialized;
    file >> deserialized;

    // レベル構造確認
    assert(deserialized.is_object());
    assert(deserialized.contains("name"));
    assert(deserialized["name"].is_string());
    std::string name = deserialized["name"].get<std::string>();
    assert(name.compare("scene") == 0);

    // データ読み込み開始
    LevelData* levelData = new LevelData();

    for (const nlohmann::json& object : deserialized["objects"]) {
        ParseObjectRecursive(object, levelData);
    }

    return levelData;
}

void JsonLevelLoader::ParseObjectRecursive(const nlohmann::json& object, LevelData* levelData)
{
    assert(object.contains("type"));
    std::string type = object["type"].get<std::string>();

    if (type == "MESH") {
        levelData->objects.emplace_back(ObjectData{});
        ObjectData& objectData = levelData->objects.back();

        if (object.contains("file_name")) {
            objectData.fileName = object["file_name"];
        }

        if (object.contains("transform")) {
            nlohmann::json transform = object["transform"];

            if (transform.contains("translation") && transform["translation"].is_array()) {
                objectData.translation.x = (float)transform["translation"][0];
                objectData.translation.y = (float)transform["translation"][2];
                objectData.translation.z = (float)transform["translation"][1];
            }

            if (transform.contains("rotation") && transform["rotation"].is_array()) {
                objectData.rotation.x = -(float)transform["rotation"][0];
                objectData.rotation.y = -(float)transform["rotation"][2];
                objectData.rotation.z = -(float)transform["rotation"][1];
            }

            if (transform.contains("scale") && transform["scale"].is_array()) {
                objectData.scaling.x = (float)transform["scale"][0];
                objectData.scaling.y = (float)transform["scale"][2];
                objectData.scaling.z = (float)transform["scale"][1];
            }
        }

    }

    // 子要素の再帰処理
    if (object.contains("children")) {
        for (const auto& child : object["children"]) {
            ParseObjectRecursive(child, levelData);
        }
    }
}
