#include "ParticleManager.h"
#include <Logger.h>
#include <numbers>

using json = nlohmann::json;

// VertexDataType, BlendMode を int で保存するシンプル版
void to_json(json& j, const ParticleManager::ParticlePreset& p)
{
	j = json{
		{"name", p.name},
		{"vertexType", static_cast<int>(p.vertexType)},
		{"textureFilePath", p.textureFilePath},
		{"blendMode", static_cast<int>(p.blendMode)},
		{"count", p.count},
		{"frequency", p.frequency},
		{"repeat", p.repeat},
		{"useRandomPosition", p.useRandomPosition},
		{"flipY", p.flipY},
		{"lifeTime", p.lifeTime},
		{"initialScale",  {p.initialScale.x,  p.initialScale.y,  p.initialScale.z}},
		{"initialRotate", {p.initialRotate.x, p.initialRotate.y, p.initialRotate.z}},
		{"initialOffset", {p.initialOffset.x, p.initialOffset.y, p.initialOffset.z}},
		{"color",         {p.color.x, p.color.y, p.color.z, p.color.w}},
		{"useBillboard",  p.useBillboard},
		{"velocity",      {p.velocity.x,      p.velocity.y,      p.velocity.z}},
		{"rotationSpeed", {p.rotationSpeed.x, p.rotationSpeed.y, p.rotationSpeed.z}},
		{"scaleSpeed",    {p.scaleSpeed.x,    p.scaleSpeed.y,    p.scaleSpeed.z}},

	};
}



void from_json(const json& j, ParticleManager::ParticlePreset& p)
{
	p.name = j.value("name", "");
	p.vertexType = static_cast<ParticleManager::VertexDataType>(
		j.value("vertexType", 0));
	p.textureFilePath = j.value("textureFilePath", "");
	p.blendMode = static_cast<ParticleManager::BlendMode>(
		j.value("blendMode", (int)ParticleManager::kBlendModeNormal));
	p.count = j.value("count", 10u);
	p.frequency = j.value("frequency", 1.0f);
	p.repeat = j.value("repeat", false);
	p.useRandomPosition = j.value("useRandomPosition", false);
	p.flipY = j.value("flipY", false);
	p.lifeTime = j.value("lifeTime", 1.0f);

	auto readVec3 = [&j](const char* key, Vector3 defaultValue) {
		Vector3 v = defaultValue;
		if (j.contains(key) && j[key].is_array() && j[key].size() == 3) {
			v.x = j[key][0].get<float>();
			v.y = j[key][1].get<float>();
			v.z = j[key][2].get<float>();
		}
		return v;
		};

	auto readVec4 = [&j](const char* key, Vector4 defaultValue) {
		Vector4 v = defaultValue;
		if (j.contains(key) && j[key].is_array() && j[key].size() == 4) {
			v.x = j[key][0].get<float>();
			v.y = j[key][1].get<float>();
			v.z = j[key][2].get<float>();
			v.w = j[key][3].get<float>();
		}
		return v;
		};

	p.initialScale = readVec3("initialScale", { 1.0f, 1.0f, 1.0f });
	p.initialRotate = readVec3("initialRotate", { 0.0f, 0.0f, 0.0f });
	p.initialOffset = readVec3("initialOffset", { 0.0f, 0.0f, 0.0f });

	// 色とビルボード
	p.color = readVec4("color", { 1.0f, 1.0f, 1.0f, 1.0f });
	p.useBillboard = j.value("useBillboard", true);
	// particle発生後のSRT
	p.velocity = readVec3("velocity", { 0.0f, 0.0f, 0.0f });
	p.rotationSpeed = readVec3("rotationSpeed", { 0.0f, 0.0f, 0.0f });
	p.scaleSpeed = readVec3("scaleSpeed", { 0.0f, 0.0f, 0.0f });

}



void ParticleManager::Initialize(VertexDataType type)
{
	dxCommon_ = DirectXCommon::GetInstance();
	srvManager_ = SrvManager::GetInstance();
	// ランダムエンジンの初期化
	randomEngine.seed(seedGenerator());

	// 各ブレンドモードのPSOを事前に生成してキャッシュ
	for (int mode = kBlendModeNone; mode < kCountOfBlendMode; ++mode) {
		GraphicsPipelineState(static_cast<BlendMode>(mode));
	}

	switch (type) {
	case VertexDataType::Plane:
		// 頂点リソースに頂点データを書き込む
		CreateVertexData();
		ringSamplerAdd = false;
		break;
	case VertexDataType::Ring:
		CreateRingVertexData();
		ringSamplerAdd = true;
		break;
	case VertexDataType::Cylinder:
		CreateCylinderVertexData();
		ringSamplerAdd = false;
	}

	// 頂点リソース、バッファービュー
	VertexBufferView();

	//書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//頂点データをリソースにコピー
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

}

void ParticleManager::Update()
{
	if (!camera_) {
		return; // カメラが設定されていない場合は何もしない
	}
	// カメラ情報を取得
	Matrix4x4 cameraMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, camera_->GetRotate(), camera_->GetTranslate());
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kWindowWidth) / float(kWindowHeight), 0.1f, 100.0f);
	Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

	// カメラ目線の設定
	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
	Matrix4x4 billboardMatrix{};
	if (usebillboardMatrix)
	{
		billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);
		billboardMatrix.m[3][0] = 0.0f; // 平行移動成分は無視
		billboardMatrix.m[3][1] = 0.0f;
		billboardMatrix.m[3][2] = 0.0f;
	}
	else
	{
		billboardMatrix = MakeIdentity4x4();
	}
	//if (usebillboardMatrix)
	//{
	//	// カメラの Y 回転だけをビルボードに適用（Z軸回転は除外）
	//	billboardMatrix = MakeRotateYMatrix(camera_->GetRotate().y);
	//	billboardMatrix.m[3][0] = 0.0f; // 平行移動成分は無視
	//	billboardMatrix.m[3][1] = 0.0f;
	//	billboardMatrix.m[3][2] = 0.0f;
	//}
	//else
	//{
	//	billboardMatrix = MakeIdentity4x4();
	//}

	// スケール調整用の倍率を設定
	constexpr float scaleMultiplier = 0.01f; // 必要に応じて調整

	for (auto& group : particleGroups)
	{
		Vector2 textureSize = group.second.textureSize;

		for (auto it = group.second.particleList.begin(); it != group.second.particleList.end();)
		{
			Particle& particle = *it;

			// パーティクルの寿命が尽きた場合は削除
			if (particle.lifeTime <= particle.currentTime)
			{
				it = group.second.particleList.erase(it);
				continue;
			}

			// スケールをテクスチャサイズに基づいて調整
			/*particle.transform.scale.x = textureSize.x * scaleMultiplier;
			particle.transform.scale.y = textureSize.y * scaleMultiplier;*/

			// 位置の更新
			particle.transform.translate = Add(particle.transform.translate, Multiply(kDeltaTime, particle.velocity));

			// 回転の更新（rad/秒）
			particle.transform.rotate.x += particle.rotationSpeed.x * kDeltaTime;
			particle.transform.rotate.y += particle.rotationSpeed.y * kDeltaTime;
			particle.transform.rotate.z += particle.rotationSpeed.z * kDeltaTime;

			// スケールの更新（/秒）
			particle.transform.scale.x += particle.scaleSpeed.x * kDeltaTime;
			particle.transform.scale.y += particle.scaleSpeed.y * kDeltaTime;
			particle.transform.scale.z += particle.scaleSpeed.z * kDeltaTime;

			// 経過時間を更新
			particle.currentTime += kDeltaTime;

			// Scale 行列
			Matrix4x4 scaleMatrix = MakeScaleMatrix(particle.transform.scale);

			// 回転行列
			Matrix4x4 rotateMatrix = MakeIdentity4x4();
			if (usebillboardMatrix)
			{
				// ビルボード時はZ回転だけを反映（画面上でクルクル回るイメージ）
				rotateMatrix = MakeRotateZMatrix(particle.transform.rotate.z);
			}
			else
			{
				// ビルボードを使わない場合はXYZ回転をフルに反映
				Matrix4x4 rotX = MakeRotateXMatrix(particle.transform.rotate.x);
				Matrix4x4 rotY = MakeRotateYMatrix(particle.transform.rotate.y);
				Matrix4x4 rotZ = MakeRotateZMatrix(particle.transform.rotate.z);
				rotateMatrix = Multiply(rotZ, Multiply(rotY, rotX));
			}

			// Translate 行列
			Matrix4x4 translateMatrix = MakeTranslateMatrix(particle.transform.translate);

			// 回転はビルボードに任せるので、particle.transform.rotate は使わない
			// world = S * Billboard * T
			/*Matrix4x4 worldMatrix =
				Multiply(Multiply(scaleMatrix, billboardMatrix), translateMatrix);*/

				// 回転も反映してるorldMatrix
			Matrix4x4 worldMatrix =
				Multiply(Multiply(Multiply(scaleMatrix, billboardMatrix), rotateMatrix), translateMatrix);

			// VP を掛ける
			Matrix4x4 worldviewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);

			// インスタンシングデータの設定
			if (group.second.instanceCount < kNumMaxInstance)
			{
				group.second.instancingDataPtr[group.second.instanceCount].WVP = worldviewProjectionMatrix;
				group.second.instancingDataPtr[group.second.instanceCount].World = worldMatrix;

				// カラーを設定し、アルファ値を減衰
				group.second.instancingDataPtr[group.second.instanceCount].color = particle.color;
				group.second.instancingDataPtr[group.second.instanceCount].color.w = 1.0f - (particle.currentTime / particle.lifeTime);
				group.second.instancingDataPtr[group.second.instanceCount].flipY = group.second.flipY ? 1.0f : 0.0f;
				if (group.second.instancingDataPtr[group.second.instanceCount].color.w < 0.0f)
				{
					group.second.instancingDataPtr[group.second.instanceCount].color.w = 0.0f;
				}

				++group.second.instanceCount;
			}

			++it;
		}
	}
}


void ParticleManager::Draw()
{
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList().Get();

	// ルートシグネチャを設定
	commandList->SetGraphicsRootSignature(rootSignature_.Get());

	// キャッシュ内に指定されたブレンドモードのPSOが存在するか確認
	auto it = pipelineStateCache_.find(blendMode_);
	if (it == pipelineStateCache_.end() || !it->second) {
		Logger::Log("PSO for blend mode not found, defaulting to normal blend mode.");
		it = pipelineStateCache_.find(kBlendModeNormal);
		if (it == pipelineStateCache_.end() || !it->second) {
			Logger::Log("Default PSO not found. Aborting draw call.");
			return;
		}
	}



	commandList->SetPipelineState(it->second.Get());

	// プリミティブトポロジ（描画形状）を設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// VBV (Vertex Buffer View)を設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// SrvManagerのインスタンスを取得
	SrvManager* srvManager = SrvManager::GetInstance();

	// 全てのパーティクルグループについて処理を行う
	for (auto& group : particleGroups) {
		if (group.second.instanceCount == 0) continue; // インスタンスが無い場合はスキップ

		Vector2 textureLeftTop = group.second.textureLeftTop;
		Vector2 textureSize = group.second.textureSize;

		for (auto& particle : group.second.particleList)
		{
			// UV座標の計算
			float uStart = textureLeftTop.x / textureSize.x;
			float uEnd = (textureLeftTop.x + textureSize.x) / textureSize.x;
			float vStart = textureLeftTop.y / textureSize.y;
			float vEnd = (textureLeftTop.y + textureSize.y) / textureSize.y;

			// 必要であればUV座標を設定する処理を追加
		}

		//マテリアルCBufferの場所を設定
		commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

		// テクスチャのSRVのDescriptorTableを設定
		commandList->SetGraphicsRootDescriptorTable(2, srvManager->GetGPUDescriptorHandle(group.second.srvIndex));

		// インスタンシングデータのSRVのDescriptorTableを設定
		commandList->SetGraphicsRootDescriptorTable(1, srvManager->GetGPUDescriptorHandle(group.second.instancingSrvIndex));

		// Draw Call (インスタンシング描画)
		commandList->DrawInstanced(group.second.vertexCount, group.second.instanceCount, 0, 0);

		// インスタンスカウントをリセット
		group.second.instanceCount = 0;
	}
}

void ParticleManager::DrawImGuiParticlePresetEditor()
{
#ifdef USE_IMGUI

	using namespace ImGui;
	namespace fs = std::filesystem;

	static ParticlePreset preset;          // 現在編集中のプリセット
	static bool initialized = false;

	// テクスチャ候補（Resources 以下のみ）
	static std::vector<std::string> textureList;
	static int currentTextureIndex = -1;

	if (!initialized) {
		// 初期値
		preset.name = "NewParticle";
		preset.vertexType = VertexDataType::Plane;
		preset.textureFilePath = "";
		preset.blendMode = kBlendModeNormal;
		preset.count = 10;
		preset.frequency = 1.0f;
		preset.repeat = false;
		preset.useRandomPosition = true;
		preset.flipY = false;
		preset.lifeTime = 1.0f;
		preset.initialScale = { 1.0f, 1.0f, 1.0f };
		preset.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		preset.useBillboard = true;

		// Resources 以下からテクスチャ候補を列挙
		textureList.clear();
		const std::string textureDir = "Resources/ParticleTexture";

		if (fs::exists(textureDir)) {
			for (auto& entry : fs::directory_iterator(textureDir)) {
				if (!entry.is_regular_file()) { continue; }
				auto ext = entry.path().extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
				if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds") {
					// ファイル名だけを保持する
					textureList.push_back(entry.path().filename().string());
				}
			}
		}
		if (!textureList.empty()) {
			currentTextureIndex = 0;
			// JSON に保存するパスは必ず Resources/ParticleTexture/ から始める
			preset.textureFilePath = "Resources/ParticleTexture/" + textureList[0];
		}


		initialized = true;
	}

	// ウィンドウタイトル
	if (!Begin("パーティクルプリセットエディタ")) {
		End();
		return;
	}

	// --- プリセット名 ---
	{
		char buf[64];
		// 安全な strncpy_s を使用
		strncpy_s(buf, sizeof(buf), preset.name.c_str(), _TRUNCATE);
		if (InputText("プリセット名", buf, sizeof(buf))) {
			preset.name = buf;
		}
	}

	// --- 形状（メッシュ）選択 ---
	{
		const char* items[] = { "板(Plane)", "リング(Ring)", "円柱(Cylinder)" };
		int current = static_cast<int>(preset.vertexType);
		if (Combo("形状タイプ", &current, items, IM_ARRAYSIZE(items))) {
			preset.vertexType = static_cast<VertexDataType>(current);
		}
	}

	// --- ブレンドモード選択 ---
	{
		const char* blendItems[] = {
			"なし(None)",
			"通常(Normal)",
			"加算(Add)",
			"減算(Subtract)",
			"乗算(Multiply)",
			"スクリーン(Screen)"
		};
		int current = static_cast<int>(preset.blendMode);
		if (Combo("ブレンドモード", &current, blendItems, IM_ARRAYSIZE(blendItems))) {
			preset.blendMode = static_cast<BlendMode>(current);
		}
	}

	// --- テクスチャ選択（Resources 以下のファイルのみ） ---
	if (!textureList.empty()) {
		if (currentTextureIndex < 0 || currentTextureIndex >= (int)textureList.size()) {
			currentTextureIndex = 0;
		}

		auto getter = [](void* data, int idx, const char** out_text) -> bool {
			auto* vec = reinterpret_cast<std::vector<std::string>*>(data);
			if (idx < 0 || idx >= (int)vec->size()) return false;
			*out_text = (*vec)[idx].c_str();
			return true;
			};

		if (!textureList.empty()) {
			if (currentTextureIndex < 0 || currentTextureIndex >= (int)textureList.size()) {
				currentTextureIndex = 0;
			}

			auto getter = [](void* data, int idx, const char** out_text) -> bool {
				auto* vec = reinterpret_cast<std::vector<std::string>*>(data);
				if (idx < 0 || idx >= (int)vec->size()) return false;
				*out_text = (*vec)[idx].c_str();  // filename だけ
				return true;
				};

			// Combo を呼ぶ
			if (ImGui::Combo("テクスチャファイル", &currentTextureIndex, getter,
				(void*)&textureList, (int)textureList.size())) {

				const std::string fileName = textureList[currentTextureIndex];

				// JSON 用のパスは必ずここにそろえる
				preset.textureFilePath = "Resources/ParticleTexture/" + fileName;
			}
		}
		else {
			ImGui::TextColored(ImVec4(1, 0, 0, 1),
				"Resources/ParticleTexture 以下にテクスチャが見つかりません");
		}


	}
	else {
		TextColored(ImVec4(1, 0, 0, 1), "Resources 以下にテクスチャが見つかりません");
	}

	// --- エミッター設定 ---
	Separator();
	Text("エミッタ設定");
	DragInt("発生数", (int*)&preset.count, 1, 1, 1000);
	DragFloat("発生間隔(秒)", &preset.frequency, 0.01f, 0.0f, 60.0f);
	Checkbox("繰り返し再生", &preset.repeat);
	Checkbox("ランダム位置を使用", &preset.useRandomPosition);

	// --- パーティクル共通 ---
	Separator();
	Text("パーティクル共通設定");
	DragFloat3("初期スケール(scale)", &preset.initialScale.x, 0.01f, 0.0f, 100.0f);
	DragFloat3("初期回転(rotate)", &preset.initialRotate.x, 0.01f, -6.28f, 6.28f);
	DragFloat3("初期場所(translate)", &preset.initialOffset.x, 0.01f, -1000.0f, 1000.0f);
	DragFloat3("初期速度 (Velocity)", &preset.velocity.x, 0.01f, -1000.0f, 1000.0f);
	DragFloat3("回転速度(rad/秒)", &preset.rotationSpeed.x, 0.01f, -10.0f, 10.0f);
	DragFloat3("スケール速度(/秒)", &preset.scaleSpeed.x, 0.01f, -10.0f, 10.0f);

	// 色(RGBA)
	ColorEdit4("カラー (RGBA)", &preset.color.x);
	// ビルボードON/OFF
	Checkbox("常にカメラ目線(ビルボード)", &preset.useBillboard);
	Checkbox("上下反転(Flip Y)", &preset.flipY);
	DragFloat("寿命(秒)", &preset.lifeTime, 0.01f, 0.0f, 100.0f);


	// --- 保存ボタン ---
	Separator();
	if (Button("JSON を保存")) {
		if (SavePresetToJson(preset)) {
			TextColored(ImVec4(0, 1, 0, 1), "保存しました");
		}
		else {
			TextColored(ImVec4(1, 0, 0, 1), "保存に失敗しました");
		}
	}

	End();

#endif // USE_IMGUI
}


bool ParticleManager::SavePresetToJson(const ParticlePreset& preset,
	const std::string& directory)
{
	namespace fs = std::filesystem;

	try {
		// ディレクトリが無ければ作る
		if (!fs::exists(directory)) {
			fs::create_directories(directory);
		}

		std::string fileName = preset.name;
		if (fileName.empty()) {
			fileName = "UnnamedParticle";
		}

		fs::path path = fs::path(directory) / (fileName + ".json");

		// JSON にシリアライズ（to_json が呼ばれる）
		json j = preset;

		std::ofstream ofs(path);
		if (!ofs) {
			Logger::Log("Failed to open particle preset json: " + path.string());
			return false;
		}
		ofs << j.dump(4); // インデント付きで保存
		ofs.close();

		// メモリ側のプリセットも更新
		presets_[preset.name] = preset;

		// すでに同名のパーティクルグループが存在していれば、テクスチャ等を作り直す
		auto itGroup = particleGroups.find(preset.name);
		if (itGroup != particleGroups.end()) {
			// いったん削除
			DeleteParticleGroup(preset.name);

			// 新しいテクスチャ&ブレンドモードで作成し直し
			CreateParticleGroup(preset.name, preset.textureFilePath, preset.blendMode);

			// プリセットの共通設定も反映
			SetFlipYToGroup(preset.name, preset.flipY);
			SetLifeTimeToGroup(preset.name, preset.lifeTime);
			SetColorToGroup(preset.name, preset.color);

			// ビルボードは現在マネージャ全体設定
			SetUseBillboard(preset.useBillboard);

			SetVelocityToGroup(preset.name, preset.velocity);
			SetRotationSpeedToGroup(preset.name, preset.rotationSpeed);
			SetScaleSpeedToGroup(preset.name, preset.scaleSpeed);
		}

		return true;
	}
	catch (const std::exception& e) {
		Logger::Log(std::string("Exception in SavePresetToJson: ") + e.what());
		return false;
	}
}


bool ParticleManager::LoadPresetFromJson(const std::string& presetName, ParticlePreset& outPreset, const std::string& directory)
{
	namespace fs = std::filesystem;

	fs::path path = fs::path(directory) / (presetName + ".json");
	if (!fs::exists(path)) {
		return false;
	}

	try {
		std::ifstream ifs(path);
		if (!ifs) {
			return false;
		}
		json j;
		ifs >> j;
		outPreset = j.get<ParticlePreset>();

		presets_[outPreset.name] = outPreset;
		return true;
	}
	catch (const std::exception& e) {
		Logger::Log(std::string("Exception in LoadPresetFromJson: ") + e.what());
		return false;
	}
}

void ParticleManager::LoadAllPresets(const std::string& directory)
{
	namespace fs = std::filesystem;

	presets_.clear();

	if (!fs::exists(directory)) {
		return;
	}

	for (auto& entry : fs::directory_iterator(directory)) {
		if (!entry.is_regular_file()) { continue; }
		if (entry.path().extension() != ".json") { continue; }

		try {
			std::ifstream ifs(entry.path());
			if (!ifs) { continue; }

			json j;
			ifs >> j;

			ParticlePreset preset = j.get<ParticlePreset>();
			if (!preset.name.empty()) {
				presets_[preset.name] = preset;
			}
		}
		catch (...) {
			// 壊れているファイルは無視
		}
	}
}

void ParticleManager::EmitByPresetName(const std::string& presetName,
	const Transform& emitterTransform)
{
	// ---- プリセット検索 or 読み込み ----
	ParticlePreset preset;
	{
		auto it = presets_.find(presetName);
		if (it != presets_.end()) {
			preset = it->second;
		}
		else {
			// まだメモリに無ければ JSON から読み込みを試す
			if (!LoadPresetFromJson(presetName, preset)) {
				Logger::Log("ParticleManager::EmitByPresetName : preset not found -> " + presetName);
				return;
			}
		}
	}

	// JSON 側の name が空なら、presetName を使う
	if (preset.name.empty()) {
		preset.name = presetName;
	}

	const std::string groupName = preset.name;

	// ---- パーティクルグループが無ければ作成 ----
	auto itGroup = particleGroups.find(groupName);
	if (itGroup == particleGroups.end()) {
		// テクスチャとブレンドモードはプリセットから
		CreateParticleGroup(groupName, preset.textureFilePath, preset.blendMode);

		// 新規作成したグループに対して プリセットの設定を反映
		SetFlipYToGroup(groupName, preset.flipY);
		SetLifeTimeToGroup(groupName, preset.lifeTime);
		SetColorToGroup(groupName, preset.color);

		// ビルボードは現在マネージャ全体設定
		SetUseBillboard(preset.useBillboard);

		SetVelocityToGroup(groupName, preset.velocity);
		SetRotationSpeedToGroup(groupName, preset.rotationSpeed);
		SetScaleSpeedToGroup(groupName, preset.scaleSpeed);
	}
	else {
		// 既にあるグループにも、最新プリセットの値を反映しておく
		SetFlipYToGroup(groupName, preset.flipY);
		SetLifeTimeToGroup(groupName, preset.lifeTime);
		SetColorToGroup(groupName, preset.color);
		SetUseBillboard(preset.useBillboard);
		SetVelocityToGroup(groupName, preset.velocity);
		SetRotationSpeedToGroup(groupName, preset.rotationSpeed);
		SetScaleSpeedToGroup(groupName, preset.scaleSpeed);
	}

	// ---- 呼び出し元の Transform とプリセット値を合成 ----
	Transform t = emitterTransform;

	// スケールは乗算（呼び出し元の大きさに対して相対的に）
	t.scale.x *= preset.initialScale.x;
	t.scale.y *= preset.initialScale.y;
	t.scale.z *= preset.initialScale.z;

	// 回転は加算
	t.rotate.x += preset.initialRotate.x;
	t.rotate.y += preset.initialRotate.y;
	t.rotate.z += preset.initialRotate.z;

	// 位置はオフセット分だけ足す
	t.translate.x += preset.initialOffset.x;
	t.translate.y += preset.initialOffset.y;
	t.translate.z += preset.initialOffset.z;

	// ---- 形状に応じて既存の Emit 系を呼ぶ ----
	switch (preset.vertexType) {
	case VertexDataType::Plane:
		// Plane は通常の Emit。count と useRandomPosition をプリセットから。
		Emit(groupName, t, preset.count, preset.useRandomPosition);
		break;

	case VertexDataType::Ring:
		// Ring は 1 個だけの扱いなので count は無視
		RingEmit(groupName, t);
		break;

	case VertexDataType::Cylinder:
		// Cylinder も 1 個だけ
		CylinderEmit(groupName, t);
		break;

	default:
		Logger::Log("ParticleManager::EmitByPresetName : unsupported vertexType in preset -> " + presetName);
		break;
	}

	// ここが重要：Emit した「あと」で、プリセット値をグループに反映する
	SetFlipYToGroup(groupName, preset.flipY);
	SetLifeTimeToGroup(groupName, preset.lifeTime);
	SetColorToGroup(groupName, preset.color);
	SetVelocityToGroup(groupName, preset.velocity);
	SetRotationSpeedToGroup(groupName, preset.rotationSpeed);
	SetScaleSpeedToGroup(groupName, preset.scaleSpeed);
}


void ParticleManager::VertexBufferView()
{
	//==========頂点リソース生成==========//
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

	////=========VertexBufferViewを作成する=========////

	//リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点のサイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);
}

void ParticleManager::CreateVertexData()
{
	modelData.vertices.push_back({ .position = {1.0f, 1.0f, 0.0f, 1.0f}, .texcoord = {0.0f, 0.0f}, .normal = {0.0f, 0.0f, 1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f, 1.0f, 0.0f, 1.0f}, .texcoord = {1.0f, 0.0f}, .normal = {0.0f, 0.0f, 1.0f} });
	modelData.vertices.push_back({ .position = {1.0f, -1.0f, 0.0f, 1.0f}, .texcoord = {0.0f, 1.0f}, .normal = {0.0f, 0.0f, 1.0f} });
	modelData.vertices.push_back({ .position = {1.0f, -1.0f, 0.0f, 1.0f}, .texcoord = {0.0f, 1.0f}, .normal = {0.0f, 0.0f, 1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f, 1.0f, 0.0f, 1.0f}, .texcoord = {1.0f, 0.0f}, .normal = {0.0f, 0.0f, 1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f, -1.0f, 0.0f, 1.0f}, .texcoord = {1.0f, 1.0f}, .normal = {0.0f, 0.0f, 1.0f} });
}

void ParticleManager::CreateRingVertexData() {
	const uint32_t kRingDivide = 32;
	const float kOuterRadius = 1.0f;
	const float kInnerRadius = 0.2f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

	modelData.vertices.clear();

	for (uint32_t index = 0; index < kRingDivide; ++index) {
		float sin = std::sin(index * radianPerDivide);
		float cos = std::cos(index * radianPerDivide);
		float sinNext = std::sin((index + 1) * radianPerDivide);
		float cosNext = std::cos((index + 1) * radianPerDivide);

		float u = float(index) / float(kRingDivide);
		float uNext = float(index + 1) / float(kRingDivide);

		// 外→内の三角形1
		modelData.vertices.push_back({ { -sin * kOuterRadius, cos * kOuterRadius, 0.0f, 1.0f }, { u, 0.0f }, { 0.0f, 0.0f, 1.0f } });
		modelData.vertices.push_back({ { -sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f }, { uNext, 0.0f }, { 0.0f, 0.0f, 1.0f } });
		modelData.vertices.push_back({ { -sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f }, { u, 1.0f }, { 0.0f, 0.0f, 1.0f } });

		// 三角形2（内→外）
		modelData.vertices.push_back({ { -sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f }, { u, 1.0f }, { 0.0f, 0.0f, 1.0f } });
		modelData.vertices.push_back({ { -sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f }, { uNext, 0.0f }, { 0.0f, 0.0f, 1.0f } });
		modelData.vertices.push_back({ { -sinNext * kInnerRadius, cosNext * kInnerRadius, 0.0f, 1.0f }, { uNext, 1.0f }, { 0.0f, 0.0f, 1.0f } });
	}
}

void ParticleManager::CreateCylinderVertexData() {
	const uint32_t kCylinderDivide = 32;
	const float kHeight = 1.0f;
	const float kRadius = 0.5f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

	modelData.vertices.clear();

	for (uint32_t i = 0; i < kCylinderDivide; ++i) {
		float theta = i * radianPerDivide;
		float nextTheta = (i + 1) * radianPerDivide;

		float sinTheta = std::sin(theta), cosTheta = std::cos(theta);
		float sinNext = std::sin(nextTheta), cosNext = std::cos(nextTheta);

		Vector3 bottom1 = { kRadius * cosTheta, -kHeight / 2, kRadius * sinTheta };
		Vector3 bottom2 = { kRadius * cosNext, -kHeight / 2, kRadius * sinNext };
		Vector3 top1 = { kRadius * cosTheta, kHeight / 2, kRadius * sinTheta };
		Vector3 top2 = { kRadius * cosNext, kHeight / 2, kRadius * sinNext };

		// 側面（2三角形）
		modelData.vertices.push_back({ { bottom1.x, bottom1.y, bottom1.z, 1.0f }, { 0.0f, 1.0f }, { cosTheta, 0.0f, sinTheta } });
		modelData.vertices.push_back({ { bottom2.x, bottom2.y, bottom2.z, 1.0f }, { 1.0f, 1.0f }, { cosNext, 0.0f, sinNext } });
		modelData.vertices.push_back({ { top1.x, top1.y, top1.z, 1.0f }, { 0.0f, 0.0f }, { cosTheta, 0.0f, sinTheta } });

		modelData.vertices.push_back({ { top1.x, top1.y, top1.z, 1.0f }, { 0.0f, 0.0f }, { cosTheta, 0.0f, sinTheta } });
		modelData.vertices.push_back({ { bottom2.x, bottom2.y, bottom2.z, 1.0f }, { 1.0f, 1.0f }, { cosNext, 0.0f, sinNext } });
		modelData.vertices.push_back({ { top2.x, top2.y, top2.z, 1.0f }, { 1.0f, 0.0f }, { cosNext, 0.0f, sinNext } });

		// 上面
		modelData.vertices.push_back({ { 0.0f, kHeight / 2, 0.0f, 1.0f }, { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } });
		modelData.vertices.push_back({ { top1.x, top1.y, top1.z, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } });
		modelData.vertices.push_back({ { top2.x, top2.y, top2.z, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } });

		// 下面
		modelData.vertices.push_back({ { 0.0f, -kHeight / 2, 0.0f, 1.0f }, { 0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } });
		modelData.vertices.push_back({ { bottom2.x, bottom2.y, bottom2.z, 1.0f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } });
		modelData.vertices.push_back({ { bottom1.x, bottom1.y, bottom1.z, 1.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } });
	}
}


void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath, BlendMode blendMode)
{
	// 登録済みの名前かチェックして assert
	bool nameExists = false;
	for (auto it = particleGroups.begin(); it != particleGroups.end(); ++it) {
		if (it->second.materialFilePath == name) {
			nameExists = true;
			break;
		}
	}
	if (nameExists) {
		assert(false && "Particle group with this name already exists!");
	}

	// 新たなパーティクルグループを作成
	ParticleGroup newGroup;
	newGroup.materialFilePath = textureFilePath;

	// テクスチャのSRVインデックスを取得して設定
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	newGroup.srvIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

	// テクスチャサイズを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath);
	Vector2 textureSize = { static_cast<float>(metadata.width), static_cast<float>(metadata.height) };

	//// サイズを設定（指定があればそれを使用、なければテクスチャサイズを使用）
	//if (customSize.x > 0.0f && customSize.y > 0.0f) {
	//	newGroup.textureSize = customSize;
	//}
	//else {
	//	newGroup.textureSize = textureSize;
	//}

	//// テクスチャサイズを設定
	//AdjustTextureSize(newGroup, textureFilePath);

	// 頂点数を取得
	newGroup.vertexCount = static_cast<UINT>(modelData.vertices.size());

	// インスタンシング用リソースの生成
	//InstancingResource();
	newGroup.instancingResource =
		dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
	newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingDataPtr));
	for (uint32_t index = 0; index < kNumMaxInstance; ++index)
	{
		newGroup.instancingDataPtr[index].WVP = MakeIdentity4x4();
		newGroup.instancingDataPtr[index].World = MakeIdentity4x4();
		//newGroup.instancingDataPtr[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// 最大インスタンシング用リソースの生成
	//InstancingMaxResource();

	// インスタンシング用SRVを確保してSRVインデックスを記録
	newGroup.instancingSrvIndex = srvManager_->Allocate();
	srvManager_->CreateSRVforStructuredBuffer(newGroup.instancingSrvIndex, newGroup.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

	// パーティクルグループをリストに追加
	particleGroups.emplace(name, newGroup);

	// マテリアルデータの初期化
	CreateMaterialData();

	// 新しいブレンドモードを設定
	blendMode_ = blendMode;
	//GraphicsPipelineState(blendMode);  // 再生成

}

void ParticleManager::SetFlipYToGroup(const std::string& groupName, bool flip)
{
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;
	it->second.flipY = flip;
}

void ParticleManager::CreateMaterialData()
{
	// マテリアル用のリソースを作成
	materialResource = dxCommon_->CreateBufferResource(sizeof(Material));

	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//SpriteはLightingしないのfalseを設定する
	materialData->enableLighting = false;
	materialData->uvTransform = MakeIdentity4x4();
}

void ParticleManager::InstancingMaxResource()
{
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource2 = dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
	ParticleForGPU* instancingData2 = nullptr;
	instancingResource2->Map(0, nullptr, reinterpret_cast<void**>(&instancingData2));
	for (uint32_t index = 0; index < kNumMaxInstance; ++index)
	{
		instancingData2[index].WVP = MakeIdentity4x4();
		instancingData2[index].World = MakeIdentity4x4();
		//instancingData2[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate)
{
	// 位置だけランダムにばら撒く
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

	Particle particle{}; // ゼロ初期化

	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };

	Vector3 randomTranslate{
		distribution(randomEngine),
		distribution(randomEngine),
		distribution(randomEngine)
	};
	particle.transform.translate = Add(translate, randomTranslate);

	// 速度・色・寿命はプリセット側(Set系)で一括設定するので、
	// ここではデフォルト値だけ入れておく
	particle.velocity = { 0.0f, 0.0f, 0.0f };
	particle.rotationSpeed = { 0.0f, 0.0f, 0.0f };
	particle.scaleSpeed = { 0.0f, 0.0f, 0.0f };
	particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	particle.lifeTime = 1.0f;
	particle.currentTime = 0.0f;

	return particle;
}

ParticleManager::Particle ParticleManager::PrimitiveMakeNewParticle(std::mt19937& randomEngine, const Vector3& translate)
{
	// ランダム生成器
	std::uniform_real_distribution<float> distRotate(0.0f, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distScale(0.4f, 1.5f);

	Particle particle;

	// 横は固定、縦をランダム
	particle.transform.scale = { 0.05f, distScale(randomEngine), 1.0f };

	// Z軸方向にランダムに回転させる
	particle.transform.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };

	// 位置や速度は固定（必要に応じて変更可）
	particle.transform.translate = translate;
	particle.velocity = { 0.0f, 0.0f, 0.0f };

	// 色や寿命も固定
	particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	particle.lifeTime = 1.0f;
	particle.currentTime = 0;

	return particle;
}

ParticleManager::Particle ParticleManager::RingMakeNewParticle(const Vector3& translate)
{
	Particle particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = translate;
	particle.velocity = { 0.0f, 0.0f, 0.0f };
	particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	particle.lifeTime = 999999.0f; // 永続表示
	particle.currentTime = 0.0f;
	return particle;
}

ParticleManager::Particle ParticleManager::CylinderMakeNewParticle(const Vector3& translate)
{
	Particle particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = translate;
	particle.velocity = { 0.0f, 0.0f, 0.0f };
	particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	particle.lifeTime = 999999.0f;
	particle.currentTime = 0.0f;
	return particle;
}

void ParticleManager::Emit(const std::string& name, const Transform& transform, uint32_t count, bool useRandomPosition) {
	if (particleGroups.find(name) == particleGroups.end()) {
		assert("Specified particle group does not exist!");
		return;
	}

	ParticleGroup& group = particleGroups[name];

	if (group.particleList.size() >= count) {
		return;
	}

	for (uint32_t i = 0; i < count; ++i) {
		Particle particle{};

		if (useRandomPosition) {
			// 位置だけランダム
			particle = MakeNewParticle(randomEngine, transform.translate);
		}
		else {
			// 完全に固定位置で出す
			particle.transform.translate = transform.translate;
			particle.transform.scale = { 1.0f, 1.0f, 1.0f };
			particle.transform.rotate = { 0.0f, 0.0f, 0.0f };

			particle.velocity = { 0.0f, 0.0f, 0.0f };
			particle.rotationSpeed = { 0.0f, 0.0f, 0.0f };
			particle.scaleSpeed = { 0.0f, 0.0f, 0.0f };
			particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };
			particle.lifeTime = 1.0f;
			particle.currentTime = 0.0f;
		}

		// Emit 引数側のスケール／回転を上書き
		particle.transform.scale = transform.scale;
		particle.transform.rotate = transform.rotate;

		group.particleList.push_back(particle);
	}
}

void ParticleManager::PrimitiveEmit(const std::string name, const Transform& transform, uint32_t count)
{
	if (particleGroups.find(name) == particleGroups.end()) {
		// パーティクルグループが存在しない場合はエラーを出力して終了
		assert("Specified particle group does not exist!");

	}

	// 指定されたパーティクルグループが存在する場合、そのグループにパーティクルを追加
	ParticleGroup& group = particleGroups[name];

	// すでにkNumMaxInstanceに達している場合、新しいパーティクルの追加をスキップする
	if (group.particleList.size() >= count) {
		return;
	}

	// 指定された数のパーティクルを生成して追加
	for (uint32_t i = 0; i < count; ++i) {
		Particle particle = PrimitiveMakeNewParticle(randomEngine, transform.translate);
		particle.transform.rotate = transform.rotate;
		particle.transform.scale = transform.scale;
		group.particleList.push_back(particle);
	}
}

void ParticleManager::RingEmit(const std::string name, const Transform& transform)
{
	if (particleGroups.find(name) == particleGroups.end()) {
		assert("Specified particle group does not exist!");
	}

	ParticleGroup& group = particleGroups[name];

	// すでにリングが存在するなら追加しない（常に1つだけとする）
	if (!group.particleList.empty()) {
		return;
	}

	Particle particle = RingMakeNewParticle(transform.translate);
	particle.transform.rotate = transform.rotate;
	particle.transform.scale = transform.scale;
	group.particleList.push_back(particle);
}

void ParticleManager::CylinderEmit(const std::string& name, const Transform& transform)
{
	if (particleGroups.find(name) == particleGroups.end()) {
		assert("Specified particle group does not exist!");
	}

	ParticleGroup& group = particleGroups[name];

	if (!group.particleList.empty()) {
		return;
	}

	Particle particle = CylinderMakeNewParticle(transform.translate);
	particle.transform.rotate = transform.rotate;
	particle.transform.scale = transform.scale;
	group.particleList.push_back(particle);
}

void ParticleManager::SetScaleToGroup(const std::string& groupName, const Vector3& scale) {
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.transform.scale = scale;
	}
}

void ParticleManager::SetRotationToGroup(const std::string& groupName, const Vector3& rotation) {
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.transform.rotate = rotation;
	}
}

void ParticleManager::SetTranslationToGroup(const std::string& groupName, const Vector3& translation) {
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.transform.translate = translation;
	}
}

void ParticleManager::SetVelocityToGroup(const std::string& groupName, const Vector3& velocity) {
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.velocity = velocity;
	}
}

void ParticleManager::SetRotationSpeedToGroup(const std::string& groupName, const Vector3& rotationSpeed)
{
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.rotationSpeed = rotationSpeed;
	}
}

void ParticleManager::SetScaleSpeedToGroup(const std::string& groupName, const Vector3& scaleSpeed)
{
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.scaleSpeed = scaleSpeed;
	}
}

void ParticleManager::SetColorToGroup(const std::string& groupName, const Vector4& color) {
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.color = color;
	}
}

void ParticleManager::SetLifeTimeToGroup(const std::string& groupName, float lifeTime)
{
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.lifeTime = lifeTime;
	}
}

void ParticleManager::SetCurrentTimeToGroup(const std::string& groupName, float currentTime)
{
	auto it = particleGroups.find(groupName);
	if (it == particleGroups.end()) return;

	for (auto& particle : it->second.particleList) {
		particle.currentTime = currentTime;
	}
}

void ParticleManager::DeleteParticleGroup(const std::string& groupName) {
	particleGroups.erase(groupName);
}


void ParticleManager::AdjustTextureSize(ParticleGroup& group, const std::string& textureFilePath)
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureFilePath);

	// テクスチャサイズを設定
	group.textureSize.x = static_cast<float>(metadata.width);
	group.textureSize.y = static_cast<float>(metadata.height);
}


void ParticleManager::RootSignature()
{
	////=========DescriptorRange=========////

	//D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRangeForInstancing_[0].BaseShaderRegister = 0;  //0から始まる
	descriptorRangeForInstancing_[0].NumDescriptors = 1;  //数は1
	descriptorRangeForInstancing_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  //SRVを使う
	descriptorRangeForInstancing_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;  //Offsetを自動計算


	////=========RootSignatureを生成する=========////

	//RootSignature作成
	/*D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};*/
	descriptionRootSignature_.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	//RootSignature作成。複数設定できるので配列。今回は結果1つだけなので長さ1の配列
	/*D3D12_ROOT_PARAMETER rootParameters[4] = {};*/
	rootParameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    //CBVを使う
	rootParameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;    //PixelShaderで使う
	rootParameters_[0].Descriptor.ShaderRegister = 0;    //レジスタ番号0とバインド
	rootParameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters_[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing_;
	rootParameters_[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing_);
	rootParameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  //DescriptorTableを使う
	rootParameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters_[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing_;
	rootParameters_[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing_);
	////=========平行光源をShaderで使う=========////
	rootParameters_[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[3].Descriptor.ShaderRegister = 1;  //レジスタ番号1を使う
	////=========光源のカメラの位置をShaderで使う=========////
	rootParameters_[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[4].Descriptor.ShaderRegister = 2;  //レジスタ番号1を使う
	////========ポイントライトをShaderで使う========////
	rootParameters_[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[5].Descriptor.ShaderRegister = 3;  //レジスタ番号3を使う

	////========スポットライトをShaderで使う========////
	rootParameters_[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  //CBVを使う
	rootParameters_[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	rootParameters_[6].Descriptor.ShaderRegister = 4;  //レジスタ番号4を使う

	descriptionRootSignature_.pParameters = rootParameters_;    //ルートパラメータ配列へのポインタ
	descriptionRootSignature_.NumParameters = _countof(rootParameters_);    //配列の長さ


	////=========Samplerの設定=========////

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  //バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  //0～1の範囲外をリピート
	if (ringSamplerAdd = true) {
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
	else {

		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;  //比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;  //ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;  //レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  //PixelShaderで使う
	descriptionRootSignature_.pStaticSamplers = staticSamplers;
	descriptionRootSignature_.NumStaticSamplers = _countof(staticSamplers);



	////シリアライズしてバイナリにする
	//Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	//Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成
	/*Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;*/
	hr = dxCommon_->GetDevice().Get()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));

	if (FAILED(hr)) {
		Logger::Log("rootsig\n");
		exit(1);
	}

	assert(SUCCEEDED(hr));
}

void ParticleManager::GraphicsPipelineState(BlendMode blendMode)
{
	//ルートシグネチャの生成
	RootSignature();

	////=========InputLayoutの設定を行う=========////

	InputLayout();


	////=========BlendStateの設定を行う=========////

	BlendState(blendMode);

	////=========RasterizerStateの設定を行う=========////

	RasterizerState();


	////=========ShaderをCompileする=========////

	vertexShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Particle.VS.hlsl",
		L"vs_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
	if (vertexShaderBlob_ == nullptr) {
		Logger::Log("vertexShaderBlob_\n");
		exit(1);
	}
	assert(vertexShaderBlob_ != nullptr);
	pixelShaderBlob_ = dxCommon_->CompileShader(L"Resources/shaders/Particle.PS.hlsl",
		L"ps_6_0"/*, dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler()*/);
	if (pixelShaderBlob_ == nullptr) {
		Logger::Log("pixelShaderBlob_\n");
		exit(1);
	}
	assert(pixelShaderBlob_ != nullptr);


	////=========DepthStencilStateの設定を行う=========////

	DepthStencilState();


	////=========PSOを生成する=========////
	  // PSOを生成しキャッシュに保存
	auto pipelineState = PSO();
	if (pipelineState) {
		pipelineStateCache_[blendMode] = pipelineState;
	}
	else {
		Logger::Log("Failed to create PSO for blend mode: " + std::to_string(blendMode));
		assert(false && "PSO creation failed!");
	}
}

void ParticleManager::InputLayout()
{
	////=========InputLayoutの設定を行う=========////

	/*D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};*/
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc_.pInputElementDescs = inputElementDescs;
	inputLayoutDesc_.NumElements = _countof(inputElementDescs);
}

void ParticleManager::BlendState(BlendMode blendMode)
{
	////=========BlendStateの設定を行う=========////

	 // すべての色要素を書き込む
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;

	switch (blendMode) {
	case kBlendModeNormal:
		blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	case kBlendModeAdd:
		blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	case kBlendModeSubtract:
		blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		break;
	case kBlendModeMultiply:
		blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
		blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
		blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	case kBlendModeScreen:
		blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		break;
	default:
		blendDesc_.RenderTarget[0].BlendEnable = FALSE;
		break;
	}

	// α値のブレンド設定
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
}

void ParticleManager::RasterizerState()
{
	////=========RasterizerStateの設定を行う=========////

	//RasterizerStateの設定
	/*D3D12_RASTERIZER_DESC rasterizerDesc{};*/
	//裏面(時計回り)を表示しない
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_BACK;
	//三角形の中を塗りつぶす
	rasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;
	//カリングしない(裏面も表示させる)
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_NONE;
}

void ParticleManager::DepthStencilState()
{
	////=========DepthStencilStateの設定を行う=========////

	////DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効化する
	depthStencilDesc_.DepthEnable = true;
	//書き込みします
	depthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> ParticleManager::PSO()
{
	// グラフィックスパイプラインのルートシグネチャを設定
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();

	// 入力レイアウトを設定
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc_;

	// 頂点シェーダーとピクセルシェーダーを設定
	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(), pixelShaderBlob_->GetBufferSize() };

	// ブレンドステートを設定
	graphicsPipelineStateDesc.BlendState = blendDesc_;

	// ラスタライザーステートを設定
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc_;

	// レンダーターゲットの数を設定し、フォーマットを指定
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// プリミティブトポロジの種類を設定します（ここでは三角形）
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// サンプルの設定（標準の1xサンプリング）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 深度ステンシルステートの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc_;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// グラフィックスパイプラインステートオブジェクト (PSO) を生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	HRESULT hr = dxCommon_->GetDevice().Get()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineState));

	// PSOの生成に失敗した場合、ログを出力し、nullポインタを返す
	if (FAILED(hr)) {
		Logger::Log("PSO creation failed");
		return nullptr;
	}

	return pipelineState;
}

//void ParticleManager::PSO()
//{
//	////=========PSOを生成する=========////
//
//	/*D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};*/
//	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();    //RootSignature
//	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc_;    //InputLayout
//	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(),
//	vertexShaderBlob_->GetBufferSize() };    //VertexShader
//	graphicsPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(),
//	pixelShaderBlob_->GetBufferSize() };    //PixelShader
//	graphicsPipelineStateDesc.BlendState = blendDesc_;    //BlendState
//	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc_;    //RasterizerState
//	//書き込むRTVの情報
//	graphicsPipelineStateDesc.NumRenderTargets = 1;
//	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//	//利用するトポロジ(形状)のタイプ。三角形
//	graphicsPipelineStateDesc.PrimitiveTopologyType =
//		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	//どのように画面に色を打ち込むかの設定(気にしなくて良い)
//	graphicsPipelineStateDesc.SampleDesc.Count = 1;
//	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
//	//DepthStencilの設定
//	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc_;
//	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	//実際に生成
//	/*Microsoft::WRL::ComPtr<ID3D12PipelineState>graphicsPipelineState = nullptr;*/
//	HRESULT hr = dxCommon_->GetDevice().Get()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
//		IID_PPV_ARGS(&graphicsPipelineState));
//
//	if (FAILED(hr)) {
//		Logger::Log("PSO\n");
//		exit(1);
//	}
//
//	assert(SUCCEEDED(hr));
//}
