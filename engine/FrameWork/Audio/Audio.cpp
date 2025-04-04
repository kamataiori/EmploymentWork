#include "Audio.h"
#include <cassert>

void Audio::Initialize()
{
	// Audioの初期化
	// XAudioエンジンのインスタンスを作成
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// マスターボイスを生成
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));
}

void Audio::Finalize()
{
	// 音声リソースの解放
	if (pSourceVoice) {
		pSourceVoice->DestroyVoice();
		pSourceVoice = nullptr;
	}
}

Audio::SoundData Audio::SoundLoadWave(const char* fileName)
{
	HRESULT result;

	//------①ファイルオープン------

	// ファイル入力ストリームのインスタンス
	std::ifstream file;

	// .wavファイルをバイナリモードで開く
	file.open(fileName, std::ios_base::binary);

	// ファイルオープン失敗を検出する
	assert(file.is_open());

	//------②.wavデータ読み込み------

	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));

	// ファイルがRIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	// タイプがWAVEかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// Formatチャンクの読み込み
	FormatChunk format = {};
	file.read((char*)&format, sizeof(ChunkHeader));

	// チャンクヘッダーの確認
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	// Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	// JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {
		// 読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		// 再読み込み
		file.read((char*)&data, sizeof(data));
	}

	// チャンクIDが "data" であることを確認
	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// Dataチャンクのデータ部（波形データ）の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);


	//------③ファイルクローズ------

	// Waveファイルを閉じる
	file.close();

	//------④読み込んだ音声データをreturn------

	// returnするための音声データ
	SoundData soundData{};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

void Audio::SoundUnload(SoundData* soundData)
{
	// バッファのメモリを解放
	delete[] soundData->pBuffer;
	soundData->pBuffer = nullptr;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

void Audio::SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT result;

	// 波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;      // バッファの先頭アドレス
	buf.AudioBytes = soundData.bufferSize;   // バッファのサイズ
	buf.Flags = XAUDIO2_END_OF_STREAM;       // 再生終了のフラグ

	// 波形データの設定をSourceVoiceに反映
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	// 再生開始
	result = pSourceVoice->Start(0);
	assert(SUCCEEDED(result));
}

void Audio::SoundPlayLoopWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT result;

	// 波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf = { 0 };
	buf.AudioBytes = soundData.bufferSize;
	buf.pAudioData = soundData.pBuffer;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.LoopCount = XAUDIO2_LOOP_INFINITE; // 無限ループ設定

	// 波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = pSourceVoice->Start(0);
	assert(SUCCEEDED(result));
}

void Audio::SoundStop()
{
	if (pSourceVoice != nullptr)
	{
		// 再生停止
		pSourceVoice->Stop(0);

		// バッファをリセットして再生位置をリセット
		pSourceVoice->FlushSourceBuffers();
	}
}

Microsoft::WRL::ComPtr<IXAudio2> Audio::GetXAudio2() const
{
	return xAudio2;
}
