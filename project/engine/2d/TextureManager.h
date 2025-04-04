#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include <string>
#include <unordered_map>

class TextureManager
{
private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath"></param>
	void LoadTexture(const std::string& filePath);

	/// <summary>
	/// SRVインデックスの開始番号
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	/// <summary>
	/// テクスチャ番号からGPUハンドルを取得
	/// </summary>
	/// <param name="textureIndex"></param>
	/// <returns></returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	/// <summary>
	/// メタデータを取得
	/// </summary>
	/// <param name="textureIndex"></param>
	/// <returns></returns>
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

	uint32_t GetSrvIndex(const std::string& filePath);

private:
	//テクスチャ1枚分のデータ
	struct TextureData {
		std::string filePath;  //画像のファイルパス
		DirectX::TexMetadata metadata;  //画像の幅、高さなどの情報
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;  //テクスチャリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;  // 中間リソース
		uint32_t srvIndex;  //SRVインデックス
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;  //SRV作成時に必要なCPUハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;  //描画コマンドに必要なGPUハンドル
	};

	//テクスチャデータ
	std::unordered_map<std::string, TextureData> textureDatas;

	//SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

	DirectXCommon* dxCommon_ = nullptr;

	SrvManager* srvManager_ = nullptr;

};

