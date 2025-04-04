#pragma once
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include <fstream>
#include <memory>
#include <wrl/client.h>

class Audio
{
public:
    // チャンクヘッダ
    struct ChunkHeader
    {
        char id[4];  // チャンク毎のid
        int32_t size;  // チャンクサイズ
    };

    // RIFFヘッダチャンク
    struct RiffHeader
    {
        ChunkHeader chunk; // "RIFF"
        char type[4];  // "WAVE"
    };

    // FMTチャンク
    struct FormatChunk
    {
        ChunkHeader chunk; // "fmt"
        WAVEFORMATEX fmt;  // 波形フォーマット
    };

    // 音声データ
    struct SoundData
    {
        // 波形フォーマット
        WAVEFORMATEX wfex;
        // バッファの先頭アドレス
        BYTE* pBuffer;
        // バッファのサイズ
        unsigned int bufferSize;
    };

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 音声データの読み込み
    /// </summary>
    SoundData SoundLoadWave(const char* fileName);

    /// <summary>
    /// 音声データの解放
    /// </summary>
    void SoundUnload(SoundData* soundData);

    /// <summary>
    /// 音声再生
    /// </summary>
    void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);

    /// <summary>
    /// 音声無限ループ再生
    /// </summary>
    void SoundPlayLoopWave(IXAudio2* xAudio2, const SoundData& soundData);

    /// <summary>
    /// 音声の停止
    /// </summary>
    void SoundStop();

    /// <summary>
    /// xAudio2のゲッター
    /// </summary>
    Microsoft::WRL::ComPtr<IXAudio2> GetXAudio2() const;

private:
    //------オーディオ関係------
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
    IXAudio2MasteringVoice* masterVoice = nullptr;
    IXAudio2SourceVoice* pSourceVoice = nullptr;  // 音声ソースボイスのメンバ変数
    HRESULT result;
};