#include "Model.h"
#include "MathFunctions.h"
#include "TextureManager.h"
#include <Object3d.h>
#include <iostream>

void Model::Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename)
{
	//引数で受け取ってメンバ変数に記録する
	this->modelCommon_ = modelCommon;

	//モデル読み込み
	modelData = LoadModelFile(directorypath, filename);
	// アニメーション読み込み
	if (modelData.isAnimation) {
		animation = LoadAnimationFile(directorypath, filename);
		skeleton = CreateSkeleton(modelData.rootNode);
		skinCluster = CreateSkinCluster(modelCommon_->GetDxCommon()->GetDevice(), skeleton, modelData, SrvManager::GetInstance()->GetSrvDescriptorHeap(), SrvManager::GetInstance()->GetDescriptorSizeSRV());
	}

	// 頂点データを作成
	CreateVertexData();

	// マテリアルデータの初期化
	CreateMaterialData();

	CreateIndexResource();

	//.objの参照しているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャの番号を取得
	modelData.material.textureIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Update()
{
	animationTime += 1.0f / 60.0f;  // 時間を進める
	animationTime = std::fmod(animationTime, animation.duration);  // 繰り返し再生

	// 骨ごとの状態を決める
	AppAnimation(skeleton, animation, animationTime);
	// 現在の骨ごとのLocal情報を基に、SkeltonSpaceの情報を更新する
	Update(skeleton);
	// SkeltonSpaceの情報を基に、SkinClusterのMatrixPaletteを更新する
	Update(skinCluster, skeleton);

	/*NodeAnimation& rootNodeAnimation = animation.NodeAnimations[modelData.rootNode.name];
	Vector3 translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
	Quaternion rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
	Vector3 scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);

	Matrix4x4 localMatrix = MakeAffineMatrix(scale, rotate, translate);

	modelData.rootNode.localMatrix = localMatrix;*/
}

void Model::Update(Skeleton& skeleton)
{
	// すべてのJointを更新。親が若いので通常ループで処理可能になっている
	for (Joint& joint : skeleton.joints)
	{
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		// 親がいれば親の行列をかける
		if (joint.parent)
		{
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		}
		else
		{
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

void Model::Update(SkinCluster& skinCluster, const Skeleton& skeleton)
{
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
	{
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = transpose(Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

void Model::Draw()
{
	//VertexBufferViewを設定
	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
				vertexBufferView,
				skinCluster.influenceBufferView
	};
	modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 2, vbvs);
	//modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	// IndexBufferを設定
	modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);

	//マテリアルCBufferの場所を設定
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	//SRVのDescriptorTableの先頭を設定
	//modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(2, modelCommon_->GetDxCommon()->GetGPUDescriptorHandle(SrvManager::GetInstance()->GetSrvDescriptorHeap().Get(), SrvManager::GetInstance()->GetDescriptorSizeSRV(), 1));

	// SRVのDescriptorTableを設定,テクスチャを指定
	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, modelData.material.textureIndex);

	if (modelData.isAnimation) {

		modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(7, skinCluster.paletteSrvHandle.second);
	}

	//描画!（DrawCall/ドローコール）
	//modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(modelData.vertices.size()), 1, 0, 0, 0);
	modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
}

void Model::DrawSkeleton(const Matrix4x4& worldMatrix)
{
	if (!modelData.isAnimation) return;

	std::vector<Vector3> jointPositions;
	std::vector<int> parentIndices;

	for (const Joint& joint : skeleton.joints) {
		// ジョイントの空間座標をワールド行列で変換
		Vector3 worldPosition = TransformCoord(joint.skeletonSpaceMatrix.Translation(), worldMatrix);
		jointPositions.push_back(worldPosition);
		parentIndices.push_back(joint.parent.value_or(-1));
	}

	DrawLine::GetInstance()->DrawBone(jointPositions, parentIndices);
}

void Model::CreateVertexData()
{
	// 頂点リソースを作成
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

	//リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * modelData.vertices.size());
	//頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	//書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//頂点データをリソースにコピー
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::CreateIndexResource()
{
	indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());

	//リソースの先頭アドレスから使う
	indexBufferViewSprite.BufferLocation = indexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * modelData.indices.size();
	//インデックスはuint32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndex));
	std::memcpy(mappedIndex, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
	indexResource->Unmap(0, nullptr);
}

void Model::CreateMaterialData()
{
	// マテリアル用のリソースを作成
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//SpriteはLightingしないfalseを設定する
	materialData->enableLighting = true;
	materialData->uvTransform = MakeIdentity4x4();
	materialData->shininess = 50.0f;
}

//.mtlファイル読み取り
Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	//1,中で必要となる変数の宣言
	MaterialData materialData;  //構築するMaterialData
	std::string line;  //ファイルから読んだ1行を格納するもの

	//2,ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);  //ファイルを開く
	assert(file.is_open());  //とりあえず開けなかったら止める

	//3,実際にファイルを読み、ModelDataを構築していく
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;  //先頭の識別子を読む

		//identifireに応じた処理
		if (identifier == "map_Kd")
		{
			std::string textureFilename;
			s >> textureFilename;
			//連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	//4,MaterialDataを返す
	return materialData;
}

Model::Node Model::ReadNode(aiNode* node)
{
	Node result;

	// `aiMatrix4x4`からスケール、回転、平行移動成分を抽出
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);  // Assimpの関数で分解

	// スケールの設定（符号反転なし）
	result.transform.scale = { scale.x, scale.y, scale.z };

	// 回転の設定（YとZの符号を反転して右手系→左手系に変更）
	result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };

	// 平行移動の設定（X軸反転で右手系→左手系に変更）
	result.transform.translate = { -translate.x, translate.y, translate.z };

	// `MakeAffineMatrix`を使用して`localMatrix`を構築
	result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	// ノード名と子ノードの設定
	result.name = node->mName.C_Str();
	result.children.resize(node->mNumChildren);
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
		// 再帰的に読み込んで階層構造を作成
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

//.objファイル読み取り
Model::ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename)
{
	// Assimpのインポーターを作成
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	// シーンを読み込む
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_Triangulate);
	assert(scene && scene->HasMeshes()); // シーンが有効でメッシュがあるか確認

	ModelData modelData;

	if (scene->mNumAnimations) {
		modelData.isAnimation = true;
	}
	else {
		modelData.isAnimation = false;
	}


	// Meshを解析する
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals()); // 法線がないMeshは非対応
		assert(mesh->HasTextureCoords(0)); // テクスチャ座標がないMeshは非対応
		modelData.vertices.resize(mesh->mNumVertices);  // 最初に頂点数分のメモリを確保しておく

		// 頂点データを解析する
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

			modelData.vertices[vertexIndex].position = { -position.x , position.y , position.z , 1.0f };
			modelData.vertices[vertexIndex].normal = { -normal.x , normal.y , normal.z };
			modelData.vertices[vertexIndex].texcoord = { texcoord.x , texcoord.y };
		}

		// Faceを解析する
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形のみサポート

			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];
				modelData.indices.push_back(vertexIndex);
			}
		}

		// SkinCluster構築用のデータ取得を追加
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			// Jointごとの格納領域を作る
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			// InverseBindPoseMatrixの抽出
			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse(); // BindPoseMatrixに戻す
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate); // 成分を抽出
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix({ scale.x,scale.y,scale.z }, { rotate.x,-rotate.y,-rotate.z,rotate.w }, { -translate.x,translate.y,translate.z });
			// InverseBindMatrixにする
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

			// Weight情報を取り出す
			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
			{
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId });
			}
		}
	}

	// Materialを解析する
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}
	}

	// ルートノードを解析してモデルデータに設定
	modelData.rootNode = ReadNode(scene->mRootNode);

	return modelData;
}

AnimationData Model::LoadAnimationFile(const std::string& directoryPath, const std::string& fileName)
{
	AnimationData animation; // 今回作るアニメーション

	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + fileName;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);

	// ファイルの読み込みが成功したか確認
	if (!scene) {
		std::cerr << "Error loading animation file: " << importer.GetErrorString() << std::endl;
		assert(scene && "Failed to load animation file.");
		return animation; // もしくは適切なエラー処理を行う
	}

	// アニメーションが含まれているか確認
	assert(scene->mNumAnimations != 0 && "No animations found in the file.");

	aiAnimation* animationAssimp = scene->mAnimations[0]; // 最初のアニメーションだけ採用。もちろん複数対応するに越したことはない

	// 時間の単位を秒に変換
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

	// assimpでは個々のNodeのAnimationをchannelと呼んでいるのでchannelを回してNodeAnimationの情報をとってくる
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.NodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// 各PositionKeysをKeyframeVector3としてNodeAnimationに追加
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // ここでも秒に変換
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // 右手→左手
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		// 各RotationKeysをKeyframeQuaternionとしてNodeAnimationに追加
		if (nodeAnimationAssimp->mNumRotationKeys > 0)
		{
			// RotationKeysが存在するかチェック
			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
				aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
				KeyframeQuaternion keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w }; // 右手→左手
				nodeAnimation.rotate.keyframes.push_back(keyframe);
			}
		}

		// 各ScalingKeysをKeyframeVector3としてNodeAnimationに追加
		if (nodeAnimationAssimp->mNumScalingKeys > 0)
		{
			// ScalingKeysが存在するかチェック
			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
				nodeAnimation.scale.keyframes.push_back(keyframe);
			}
		}

	}

	// 解析完了
	return animation;

}

int32_t Model::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size());  // 現在登録されている数をIndexに
	joint.parent = parent;
	joints.push_back(joint);  // SkeletonのJoint列に追加
	for (const Node& child : node.children)
	{
		// 子Jointを作成し、そのIndexを登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	// デバッグログ
	OutputDebugStringA(("Joint created: " + joint.name + "\n").c_str());

	// 自身のIndexを返す
	return joint.index;
}

Skeleton Model::CreateSkeleton(const Node& rootNode)
{
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	// 名前とindexのマッピングを行いアクセスしやすくする
	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	// デバッグログ
	if (skeleton.joints.empty()) {
		OutputDebugStringA("ERROR: Skeleton creation failed! No joints found.\n");
	}
	else {
		OutputDebugStringA(("Skeleton created successfully! Joint count: " + std::to_string(skeleton.joints.size()) + "\n").c_str());
	}

	return skeleton;
}

void Model::AppAnimation(Skeleton& skeleton, const AnimationData& animation, float animationTime)
{
	for (Joint& joint : skeleton.joints)
	{
		// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文。
		if (auto it = animation.NodeAnimations.find(joint.name); it != animation.NodeAnimations.end())
		{
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
			joint.transform.rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
			joint.transform.scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
		}
	}
}

SkinCluster Model::CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize)
{
	SkinCluster skinCluster_;

	// palette用のResourceを確保
	skinCluster_.paletteResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster_.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster_.mappedPalette = { mappedPalette,skeleton.joints.size() }; // spanを使ってアクセスするようにする
	paletteIndex = SrvManager::GetInstance()->Allocate();
	skinCluster_.paletteSrvHandle.first = modelCommon_->GetDxCommon()->GetCPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, paletteIndex);
	skinCluster_.paletteSrvHandle.second = modelCommon_->GetDxCommon()->GetGPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, paletteIndex);

	// palette用のSRVを作成。StructuredBufferでアクセスできるようにする
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	device->CreateShaderResourceView(skinCluster_.paletteResource.Get(), &paletteSrvDesc, skinCluster_.paletteSrvHandle.first);

	// influence用のResourceを確保。頂点ごとにinfluence情報を追加できるようにする
	skinCluster_.influenceResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster_.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());  // θ埋め。weightを0にしておく
	skinCluster_.mappedInfluence = { mappedInfluence,modelData.vertices.size() };

	// influence用のVBVを作成
	skinCluster_.influenceBufferView.BufferLocation = skinCluster_.influenceResource->GetGPUVirtualAddress();
	skinCluster_.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster_.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixの保存領域を作成して、単位行列で埋める
	skinCluster_.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster_.inverseBindPoseMatrices.begin(), skinCluster_.inverseBindPoseMatrices.end(), MakeIdentity4x4);

	// ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める
	for (const auto& jointWeight : modelData.skinClusterData)
	{
		// ModelのSkinClusterの情報を解析
		auto it = skeleton.jointMap.find(jointWeight.first);  // JointWeight.firstはjoint名なので、skeltonに対象となるjointが含まれているか判断
		if (it == skeleton.jointMap.end())
		{
			// そんな名前のjointは存在しないため、次に回す
			continue;
		}
		// (*it).secondにJointのIndexが入っているので、該当IndexのInverseBindPosseMatrixを代入
		skinCluster_.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& VertexWeightData : jointWeight.second.vertexWeights)
		{
			auto& currentInfluence = skinCluster_.mappedInfluence[VertexWeightData.vertexIndex];  // 該当のVertexIndexのInfluence情報を参照しておく
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index)
			{
				if (currentInfluence.weights[index] == 0.0f)
				{
					// Weight==0が空いている状態なので、その場所にWeightとJointのIndexを代入
					currentInfluence.weights[index] = VertexWeightData.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	return skinCluster_;
}

bool Model::GetEnableLighting() const
{
	assert(materialData); // materialData が null でないことを確認
	return materialData->enableLighting != 0; // enableLighting が 0 でなければ true を返す
}
