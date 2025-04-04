#pragma once
#include "SpriteCommon.h"
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"

class SpriteCommon;

class Sprite
{
public:

	///===========================
	// メンバ関数
	///===========================

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(std::string textureFilePath);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 頂点データを作成
	/// </summary>
	void CreateVertexData();

	/// <summary>
	/// インデックスデータを作成
	/// </summary>
	void CreateIndexData();

	/// <summary>
	/// マテリアルデータの初期化
	/// </summary>
	void  CreateMaterialData();

	/// <summary>
	/// 座標変換行列データの初期化
	/// </summary>
	void CreateTransformationMatrixData();


	////////=========DescriptorHandleの取得の関数化=========////

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSize * index);
		return handleCPU;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += (descriptorSize * index);
		return handleGPU;
	}


	///===========================
	// 構造体
	///===========================

	//頂点データの拡張
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	//マテリアルを拡張する
	struct Material {
		Vector4 color;
	};

	//TransformationMatrixを拡張する
	struct TransformationMatrix {
		Matrix4x4 WVP;
	};


	////======getter関数=======////

	D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() 
	{
		return vertexBufferViewSprite;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> GetMaterialResource() 
	{
		return materialResourceSprite;
	}

	//アンカーポイント
	const Vector2& GetAnchorPoint() const { return anchorPoint; }
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }
	//左右フリップ
	void SetFlipX(bool flipX) {isFlipX_ = flipX;}
	bool GetFlipX() const {return isFlipX_;}
	//上下フリップ
	void SetFlipY(bool flipY) {isFlipY_ = flipY;}
	bool GetFlipY() const {return isFlipY_;}


	//座標
	const Vector2& GetPosition() const { return position; }
	void SetPosition(const Vector2& position) { this->position = position; }
	//---テクスチャ範囲指定---//
	//textureLeftTop
	void SetTextureLeftTop(const Vector2& leftTop) {textureLeftTop = leftTop;}
	Vector2 GetTextureLeftTop() const {return textureLeftTop;}
	//textureSize
	void SetTextureSize(const Vector2& size) {textureSize = size;}
	Vector2 GetTextureSize() const {return textureSize;}

	//回転
	float GetRotation() const { return rotation; }
	void SetRotation(float rotation) { this->rotation = rotation; }
	//色
	const Vector4& GetColor() const { return materialDataSprite->color; }
	void SetColor(const Vector4& color) { materialDataSprite->color = color; }
	//サイズ
	const Vector2& GetSize() const { return size; }
	void SetSize(const Vector2& size) { this->size = size; }


private:

	//SpriteCommon
	SpriteCommon* spriteCommon = nullptr;

	//頂点データ
	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite;  // 頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite;   // インデックスバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite;  //// マテリアル用の定数バッファ
	//...Mapしてデータを書き込む。色は白を設定しておくとよい...
	Material* materialDataSprite = nullptr;  // バッファリソース内のデータを指すポインタ
	//Sprite用のTransformMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite;
	//データを書き込む
	TransformationMatrix* transformationMatrixDataSprite = nullptr;

	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	// バッファリソースの使い道を補完するビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite;
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite;

	//テクスチャ番号
	uint32_t textureIndex = 0;



	//////=========Transformを使ってCBufferを更新する=========////

	//CPUで動かす用のTransformを作る
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	//UVTransform用の変数を用意
	Transform uvTransform{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};


	////-------座標-------////
	
	//座標
	Vector2 position = { 0.0f,0.0f };
	//テクスチャ左上座標
	Vector2 textureLeftTop = { 0.0f,0.0f };
	//テクスチャ切り出しサイズ
	Vector2 textureSize = { 0.0f,0.0f };
	//回転
	float rotation = 0.0f;
	//サイズ
	Vector2 size = { 640.0f,360.0f };
	//アンカーポイント
	Vector2 anchorPoint = { 0.0f,0.0f };

	//左右フリップ
	bool isFlipX_ = false;
	//上下フリップ
	bool isFlipY_ = false;


	//テクスチャサイズイメージ合わせ
	void AdjustTextureSize();

	std::string FilePath = {};
};

