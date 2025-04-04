#pragma once
#include <map>
#include <string>
#include <memory>
#include "Model.h"
#include "TextureManager.h" // テクスチャ管理

class ModelManager
{
public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon* dxCommon);

    /// <summary>
    /// モデルファイルの読み込み
    /// </summary>
    /// <param name="filepath">モデルファイルのパス</param>
    /// <param name="textureFile">テクスチャファイルのパス</param>
    void LoadModel(const std::string& filepath);

    /// <summary>
    /// モデルの検索
    /// </summary>
    /// <param name="filePath"></param>
    /// <returns></returns>
    Model* FindModel(const std::string& filePath);

private:
    static ModelManager* instance;

    // コンストラクタ
    ModelManager() = default;
    // デストラクタ
    ~ModelManager() = default;
    // コピーコンストラクタの削除
    ModelManager(const ModelManager&) = delete;

    // コピー代入演算子の削除
    ModelManager& operator=(const ModelManager&) = delete;

public:
    // シングルトンインスタンスの取得
    static ModelManager* GetInstance();
    // 終了
    void Finalize();

    //-------モデルデータコンテナ------//
    std::map<std::string, std::unique_ptr<Model>> models;

private:
    ModelCommon* modelCommon = nullptr;
};
