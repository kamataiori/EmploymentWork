#pragma once
#pragma once
#include <variant>
#include <map>
#include <string>
#include <vector>
#include <json.hpp>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include "Vector3.h"

#include <externals/imgui/imgui.h>

struct ObjectData {
    std::string fileName;
    Vector3 translation;
    Vector3 rotation;
    Vector3 scaling;
};

struct LevelData {
    std::vector<ObjectData> objects;
};

class JsonLevelLoader
{
public:
    // JSONファイルからLevelDataを読み込んで返す
    LevelData* LevelDataLoader(const std::string& fileName);

private:
    // 再帰的にオブジェクトツリーを走査する
    void ParseObjectRecursive(const nlohmann::json& object, LevelData* levelData);

private:

    const std::string kDefaultBaseDirectory = "Resources/Json/"; // 適当なディレクトリ
    const std::string kExtension = ".json";
};

