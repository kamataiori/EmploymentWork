#include "TextureManager.h"
#include <filesystem>

using namespace StringUtility;

TextureManager* TextureManager::instance = nullptr;
//ImGuiで0番目を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager* TextureManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new TextureManager;
	}
	return instance;
}

void TextureManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void TextureManager::Initialize()
{
	dxCommon_ = DirectXCommon::GetInstance();
	srvManager_ = SrvManager::GetInstance();
	//SRVの数と同数
	textureDatas.reserve(SrvManager::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath)
{
    std::string resolvedPath = ResolveTexturePath(filePath);

    // 読み込み済みチェックも resolvedPath で統一
    if (textureDatas.contains(resolvedPath)) {
        return;
    }

    assert(SrvManager::GetInstance()->IsBelowMaxCount());

    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtility::ConvertString(resolvedPath);
    HRESULT hr = S_FALSE;

    // 拡張子が.ddsならDDSとして読み込み
    if (resolvedPath.ends_with(".dds")) {

        // DDSロードしたことを Output に出す（VSの出力ウィンドウ）
        std::string msg = "[TextureManager] Load DDS: " + resolvedPath + "\n";
        OutputDebugStringA(msg.c_str());

        hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
    }
    else {
        hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    }

    auto HrToHex = [](HRESULT hr) {
        char buf[64];
        sprintf_s(buf, "0x%08X", (unsigned)hr);
        return std::string(buf);
        };

    if (FAILED(hr)) {
        std::string msg =
            "[TextureManager] FAILED: " + resolvedPath +
            " hr=" + HrToHex(hr) + "\n";
        OutputDebugStringA(msg.c_str());
        assert(false && "Texture load failed. See Output window for path/hr.");
    }


    assert(SUCCEEDED(hr));

    DirectX::ScratchImage mipImages{};
    if (DirectX::IsCompressed(image.GetMetadata().format)) {
        // 圧縮フォーマットならそのまま使う
        mipImages = std::move(image);
    }
    else {
        hr = DirectX::GenerateMipMaps(
            image.GetImages(), image.GetImageCount(), image.GetMetadata(),
            DirectX::TEX_FILTER_SRGB, 0, mipImages
        );
        assert(SUCCEEDED(hr));
    }

    // ★ここも resolvedPath で統一（キーがブレない）
    TextureData& textureData = textureDatas[resolvedPath];
    textureData.metadata = mipImages.GetMetadata();
    textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
    textureData.resource->SetName(filePathW.c_str());

    textureData.intermediateResource = dxCommon_->UploadTextureData(
        textureData.resource.Get(), mipImages, dxCommon_->GetDevice().Get(), dxCommon_->GetCommandList().Get()
    );

    textureData.srvIndex = srvManager_->Allocate();

    textureData.srvHandleCPU = dxCommon_->GetCPUDescriptorHandle(
        srvManager_->GetSrvDescriptorHeap().Get(), srvManager_->GetDescriptorSizeSRV(), textureData.srvIndex
    );
    textureData.srvHandleGPU = dxCommon_->GetGPUDescriptorHandle(
        srvManager_->GetSrvDescriptorHeap().Get(), srvManager_->GetDescriptorSizeSRV(), textureData.srvIndex
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureData.metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (textureData.metadata.IsCubemap()) {
        // CubemapとしてのSRV設定
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = UINT(textureData.metadata.mipLevels);
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else {
        // 通常の2Dテクスチャ設定
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);
    }

    dxCommon_->GetDevice()->CreateShaderResourceView(
        textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU
    );
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
    std::string resolvedPath = ResolveTexturePath(filePath);

    // テクスチャが存在するかチェック
    auto it = textureDatas.find(resolvedPath);
    assert(it != textureDatas.end() && "テクスチャが存在しません");

    // 読み込み済みならSRVインデックスを返す
    return it->second.srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
    std::string resolvedPath = ResolveTexturePath(filePath);

    // テクスチャが存在するかチェック
    assert(textureDatas.contains(resolvedPath) && "テクスチャが存在しません");

    // テクスチャデータを取得
    TextureData& textureData = textureDatas[resolvedPath];
    return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath)
{
    std::string resolvedPath = ResolveTexturePath(filePath);

    // テクスチャが存在するかチェック
    assert(textureDatas.contains(resolvedPath) && "テクスチャが存在しません");

    // テクスチャデータを取得
    TextureData& textureData = textureDatas[resolvedPath];
    return textureData.metadata;
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath)
{
    std::string resolvedPath = ResolveTexturePath(filePath);

    // テクスチャが存在するかチェック
    assert(textureDatas.contains(resolvedPath) && "テクスチャが存在しません");

    // テクスチャデータを取得
    //TextureData& textureData = textureDatas[resolvedPath];
    TextureData& textureData = textureDatas.at(resolvedPath);
    return textureData.srvIndex;
}

std::string TextureManager::ResolveTexturePath(const std::string& filePath)
{
	namespace fs = std::filesystem;

	// すでにdds指定ならそのまま
	if (filePath.ends_with(".dds")) {
		return filePath;
	}

	// xxx.png / xxx.jpg → xxx.dds が存在するならそっちを使う
	fs::path p(filePath);
	fs::path ddsPath = p;
	ddsPath.replace_extension(".dds");

	if (fs::exists(ddsPath)) {
		return ddsPath.string();
	}

	return filePath;
}

