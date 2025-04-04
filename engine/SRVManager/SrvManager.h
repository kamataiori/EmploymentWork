#pragma once
#include "DirectXCommon.h"

class SrvManager
{
public:
    static SrvManager* instance;

    // インスタンスを取得するシングルトンメソッド
    static SrvManager* GetInstance();

    // プライベートコンストラクタ
    SrvManager() = default;

    // コピーコンストラクタおよび代入演算子を削除
    SrvManager(const SrvManager&) = delete;
    SrvManager& operator=(const SrvManager&) = delete;

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

public:

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
   /// SRVを確保する
   /// </summary>
    uint32_t Allocate();

    /// <summary>
    /// SRV生成テクスチャ
    /// </summary>
    void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

    /// <summary>
    /// SRV生成 (Structured Buffer用)
    /// </summary>
    void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

    /// <summary>
    /// 描画前処理
    /// </summary>
    void PreDraw();

    //------Getter------

    /// <summary>
    /// srvDescriptorHeapのゲッター
    /// </summary>
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSrvDescriptorHeap() const { return descriptorHeapSrv; }

    /// <summary>
    /// descriptorSizeSRVのゲッター
    /// </summary>
    uint32_t GetDescriptorSizeSRV() const { return descriptorSizeSRV; }

    /// <summary>
    /// ディスクリプタハンドル計算
    /// </summary>
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

    //------Setter------

    /// <summary>
    /// SRVセットコマンド
    /// </summary>
    void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

    /// <summary>
    /// 最大数チェック
    /// </summary>
    bool IsBelowMaxCount() const;

public:

    // 最大SRV数(最大テクスチャ枚数)
    static const uint32_t kMaxSRVCount;

private:

    // DirectXCommonの初期化
    DirectXCommon* dxCommon_;

    // DescriptorSizeの取得
    uint32_t descriptorSizeSRV = 0;

    // SRV用のヒープディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeapSrv = nullptr;

    // 次に使用するSRVインデックス
    uint32_t useIndex = 0;
};
