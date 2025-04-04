#include "Sprite.h"
#include "MathFunctions.h"
#include "TextureManager.h"

void Sprite::Initialize( std::string textureFilePath)
{
	//引数で受け取ってメンバ変数に記録する
	this->spriteCommon = SpriteCommon::GetInstance();

	// 頂点データを作成
	CreateVertexData();

	// インデックスデータを作成
	CreateIndexData();

	// マテリアルデータの初期化
	CreateMaterialData();

	// 座標変換行列データの初期化
	CreateTransformationMatrixData();

	//単位行列を書き込んでおく
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	FilePath = textureFilePath;
	//画像サイズをテクスチャサイズに合わせる
	AdjustTextureSize();

}

void Sprite::Update()
{
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	//アンカーポイント
	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	//左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	//上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(FilePath);
	float tex_left = textureLeftTop.x / metadata.width;
	float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
	float tex_top = textureLeftTop.y / metadata.height;
	float text_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;


	//左下
	vertexData[0].position = { left,bottom,0.0f,1.0f };
	vertexData[0].texcoord = { tex_left,text_bottom };
	//左上
	vertexData[1].position = { left,top,0.0f,1.0f };
	vertexData[1].texcoord = { tex_left,tex_top };
	//右下
	vertexData[2].position = { right,bottom,0.0f,1.0f };
	vertexData[2].texcoord = { tex_right,text_bottom };

	//２枚目の三角形
	//右上
	vertexData[3].position = { right,top,0.0f,1.0f };
	vertexData[3].texcoord = { tex_right,tex_top };


	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;

	transform.translate = { position.x,position.y,0.0f };
	transform.rotate = { 0.0f,0.0f,rotation };
	transform.scale = { size.x,size.y,1.0f };


	Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform.scale);
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransform.rotate.z));
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransform.translate));

	//Sprite用のWorldViewProjectionMatrixを作る
	Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
	Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));

	transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;

}

void Sprite::Draw()
{
	//Spriteの描画。変更が必要なものだけ変更する
	spriteCommon->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
	spriteCommon->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);//IBVを設定

	//マテリアルCBufferの場所を設定
	spriteCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
	//TransformationMatrixCBufferの場所を設定
	spriteCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
	//SRVのDescriptorTableの先頭
	spriteCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(FilePath));
	//描画！（DrawCall/ドローコール）
	////dxCommon->GetCommandList()->DrawInstanced(6, 1, 0, 0);
	spriteCommon->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::CreateVertexData()
{
	// 頂点リソースを作成
	vertexResourceSprite = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * 6);

	//リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//////=========ResourceSpriteにデータを書き込む=========////

	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//////１枚目の三角形
	////左下
	//vertexData[0].position = { 0.0f,360.0f,0.0f,1.0f };
	//vertexData[0].texcoord = { 0.0f,1.0f };
	////左上
	//vertexData[1].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexData[1].texcoord = { 0.0f,0.0f };
	////右下
	//vertexData[2].position = { 640.0f,360.0f,0.0f,1.0f };
	//vertexData[2].texcoord = { 1.0f,1.0f };

	////２枚目の三角形
	////左上
	//vertexData[3].position = { 640.0f,0.0f,0.0f,1.0f };
	//vertexData[3].texcoord = { 1.0f,0.0f };

	//vertexData[0].normal = { 0.0f,0.0f,-1.0f };
}

void Sprite::CreateIndexData()
{
	//Resourceを作成
	indexResourceSprite = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);

	//リソースの先頭アドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	//使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	//////=========IndexResourceにデータを書き込む=========////

	//インデックスリソースにデータを書き込む
	//uint32_t* indexDataSprite = nullptr;
	/*indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;*/
}

void Sprite::CreateMaterialData()
{
	// マテリアル用のリソースを作成
	materialResourceSprite = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));

	//書き込むためのアドレスを取得
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}

void Sprite::CreateTransformationMatrixData()
{
	// 座標変換行列用のリソースを作成
	transformationMatrixResourceSprite = spriteCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));

	//書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	//単位行列を書き込んでおく
	transformationMatrixDataSprite->WVP = MakeIdentity4x4();
}

void Sprite::AdjustTextureSize()
{
	//テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(FilePath);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	//画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}
