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
       ///* animation =*/ LoadAnimationFile(directorypath, filename);
		// 上のコメントアウトは単一のアニメーションの時に必要 条件で管理する必要あり
		LoadAllAnimations(directorypath, filename);
        skeleton = CreateSkeleton(modelData.rootNode);
    }

    // 各メッシュごとに MeshInstance を作成
    for (const auto& meshData : modelData.meshes) {
        MeshInstance instance;

        CreateVertexData(instance, meshData);
        CreateIndexResource(instance, meshData);
        CreateMaterialData(instance, meshData);

        if (modelData.isAnimation) {
            instance.skinCluster = CreateSkinCluster(
                modelCommon_->GetDxCommon()->GetDevice(),
                skeleton,
                meshData,
                SrvManager::GetInstance()->GetSrvDescriptorHeap(),
                SrvManager::GetInstance()->GetDescriptorSizeSRV());
        }

        meshInstances_.push_back(std::move(instance));
    }

}

void Model::Update()
{
	if (currentAnimation_) {
		animationTime += 1.0f / 60.0f;
		animationTime = std::fmod(animationTime, currentAnimation_->duration);
		AppAnimation(skeleton, *currentAnimation_, animationTime);
		Update(skeleton);
		for (auto& instance : meshInstances_) {
			Update(instance.skinCluster, skeleton);
		}
	}

	//animationTime += 1.0f / 60.0f;  // 時間を進める
	//animationTime = std::fmod(animationTime, animation.duration);  // 繰り返し再生

	//// 骨ごとの状態を決める
	//AppAnimation(skeleton, animation, animationTime);
	//// 現在の骨ごとのLocal情報を基に、SkeltonSpaceの情報を更新する
	//Update(skeleton);
	//// SkeltonSpaceの情報を基に、SkinClusterのMatrixPaletteを更新する
	////Update(skinCluster, skeleton);

	//for (auto& instance : meshInstances_)
	//{
	//	Update(instance.skinCluster, skeleton);
	//	
	//}

	///*NodeAnimation& rootNodeAnimation = animation.NodeAnimations[modelData.rootNode.name];
	//Vector3 translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
	//Quaternion rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
	//Vector3 scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);

	//Matrix4x4 localMatrix = MakeAffineMatrix(scale, rotate, translate);

	//modelData.rootNode.localMatrix = localMatrix;*/
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
	// スキンを持たないメッシュは何もしない（クラッシュ防止）
	if (skinCluster.inverseBindPoseMatrices.empty()) {
		return;
	}

	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
	{
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix = skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix = transpose(Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}

	// モデルがアニメーション対応の場合のみSkinningを実行
	if (modelData.isAnimation) {
		Skinning::GetInstance()->CommonSettingCompute(skinCluster);
	}

}

void Model::Draw()
{
	for (const auto& instance : meshInstances_) {
		D3D12_VERTEX_BUFFER_VIEW vbvs[1];

		// アニメーション対応モデルでスキンがある場合
		if (modelData.isAnimation && !instance.skinCluster.inverseBindPoseMatrices.empty()) {
			// outputVertexResource を使って vbv を構築
			vbvs[0].BufferLocation = instance.skinCluster.outputVertexResource->GetGPUVirtualAddress();
			vbvs[0].SizeInBytes = instance.vertexBufferView.SizeInBytes;
			vbvs[0].StrideInBytes = instance.vertexBufferView.StrideInBytes;

			modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs);
		}
		else {
			// 非アニメーション、またはスキン無し
			vbvs[0] = instance.vertexBufferView;
			modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs);
		}

		// インデックスバッファ設定
		modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&instance.indexBufferView);

		// マテリアル（CBV, テクスチャSRVなど）
		modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, instance.materialResource->GetGPUVirtualAddress());
		SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, instance.textureIndex);

		// パレットSRV（スキンありの場合のみ）
		if (modelData.isAnimation && !instance.skinCluster.inverseBindPoseMatrices.empty()) {
			modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(7, instance.skinCluster.paletteSrvHandle.second);
		}

		// 描画
		uint32_t indexCount = instance.indexBufferView.SizeInBytes / sizeof(uint32_t);
		modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

		// 描画後 → 次のCSのためにoutputVertexResourceをUAVに戻す
		if (modelData.isAnimation && !instance.skinCluster.inverseBindPoseMatrices.empty()) {
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = instance.skinCluster.outputVertexResource.Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			modelCommon_->GetDxCommon()->GetCommandList()->ResourceBarrier(1, &barrier);
		}
	}
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

void Model::CreateVertexData(MeshInstance& instance, const MeshData& data)
{
	assert(!data.vertices.empty()); // 追加：空でないかチェック
	std::cout << "VertexData count: " << data.vertices.size() << std::endl;
	std::cout << "sizeof(VertexData): " << sizeof(VertexData) << std::endl;

	// 頂点リソースを作成
	instance.vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * data.vertices.size());
	instance.vertexBufferView.BufferLocation = instance.vertexResource->GetGPUVirtualAddress();
	instance.vertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * data.vertices.size());
	instance.vertexBufferView.StrideInBytes = sizeof(VertexData);

	//VertexData* mapped = nullptr;
	instance.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&instance.vertexData));
	std::memcpy(instance.vertexData, data.vertices.data(), sizeof(VertexData) * data.vertices.size());
}

void Model::CreateIndexResource(MeshInstance& instance, const MeshData& data)
{
	instance.indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * data.indices.size());
	instance.indexBufferView.BufferLocation = instance.indexResource->GetGPUVirtualAddress();
	instance.indexBufferView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * data.indices.size());
	instance.indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	//uint32_t* mapped = nullptr;
	instance.indexResource->Map(0, nullptr, reinterpret_cast<void**>(&instance.indexData));
	std::memcpy(instance.indexData, data.indices.data(), sizeof(uint32_t) * data.indices.size());
	instance.indexResource->Unmap(0, nullptr);
}

void Model::CreateMaterialData(MeshInstance& instance, const MeshData& data)
{
	instance.materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

	instance.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&instance.materialData));
	//instance.materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// ここでマテリアルの色を設定
	instance.materialData->color = data.material.color;  // ←変更前は固定値
	instance.materialData->enableLighting = true;
	instance.materialData->uvTransform = MakeIdentity4x4();
	instance.materialData->shininess = 50.0f;

	const std::string& texPath = data.material.textureFilePath;

	std::string usedTexturePath;

	if (!texPath.empty()) {
		usedTexturePath = texPath;
	}
	else {
		// テクスチャが無ければ白テクスチャを使う
		usedTexturePath = "Resources/white.png";
	}

	TextureManager::GetInstance()->LoadTexture(usedTexturePath);
	instance.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(usedTexturePath);

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

	// アニメーション有無フラグ
	modelData.isAnimation = (scene->mNumAnimations > 0);

	// Meshごとの処理
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		MeshData meshData;

		// 頂点データを取得
		meshData.vertices.resize(mesh->mNumVertices);
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

			meshData.vertices[vertexIndex].position = { -position.x , position.y , position.z , 1.0f };
			meshData.vertices[vertexIndex].normal = { -normal.x , normal.y , normal.z };
			meshData.vertices[vertexIndex].texcoord = { texcoord.x , texcoord.y };
		}

		// インデックスデータを取得
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形限定
			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				meshData.indices.push_back(face.mIndices[element]);
			}
		}

		// 16の倍数にするために必要な要素数
		size_t padding = 16 - meshData.indices.size() % 16;
		meshData.indices.resize(meshData.indices.size() + padding);

		// スキン情報を取得（Bone）
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = meshData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				{ scale.x, scale.y, scale.z },
				{ rotate.x, -rotate.y, -rotate.z, rotate.w },
				{ -translate.x, translate.y, translate.z });
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId });
			}
		}

		// 対応するマテリアル取得
		if (mesh->mMaterialIndex < scene->mNumMaterials) {
			if (mesh->mMaterialIndex < scene->mNumMaterials) {
				aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

				// テクスチャ読み取り（既存）
				if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
					aiString texturePath;
					if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
						if (strlen(texturePath.C_Str()) > 0) {
							meshData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
						}
					}
				}

				// ベースカラーを読み取り
				aiColor4D diffuse;
				if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
					meshData.material.color = { diffuse.r, diffuse.g, diffuse.b, diffuse.a };
				}
				else {
					meshData.material.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // fallback: 白
				}
			}

		}

		// meshData を modelData に追加
		modelData.meshes.push_back(std::move(meshData));
	}

	// ルートノードを構築
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

void Model::LoadAllAnimations(const std::string& directoryPath, const std::string& fileName)
{
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + fileName;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene && scene->HasAnimations());

	for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
		aiAnimation* animationAssimp = scene->mAnimations[i];
		AnimationData animation;
		animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

		for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
			aiNodeAnim* nodeAnim = animationAssimp->mChannels[channelIndex];
			NodeAnimation& nodeAnimation = animation.NodeAnimations[nodeAnim->mNodeName.C_Str()];
			for (uint32_t k = 0; k < nodeAnim->mNumPositionKeys; ++k) {
				KeyframeVector3 keyframe;
				keyframe.time = float(nodeAnim->mPositionKeys[k].mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { -nodeAnim->mPositionKeys[k].mValue.x, nodeAnim->mPositionKeys[k].mValue.y, nodeAnim->mPositionKeys[k].mValue.z };
				nodeAnimation.translate.keyframes.push_back(keyframe);
			}
			for (uint32_t k = 0; k < nodeAnim->mNumRotationKeys; ++k) {
				KeyframeQuaternion keyframe;
				keyframe.time = float(nodeAnim->mRotationKeys[k].mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { nodeAnim->mRotationKeys[k].mValue.x, -nodeAnim->mRotationKeys[k].mValue.y, -nodeAnim->mRotationKeys[k].mValue.z, nodeAnim->mRotationKeys[k].mValue.w };
				nodeAnimation.rotate.keyframes.push_back(keyframe);
			}
			for (uint32_t k = 0; k < nodeAnim->mNumScalingKeys; ++k) {
				KeyframeVector3 keyframe;
				keyframe.time = float(nodeAnim->mScalingKeys[k].mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { nodeAnim->mScalingKeys[k].mValue.x, nodeAnim->mScalingKeys[k].mValue.y, nodeAnim->mScalingKeys[k].mValue.z };
				nodeAnimation.scale.keyframes.push_back(keyframe);
			}
		}

		std::string animName = animationAssimp->mName.C_Str();
		if (animName.empty()) {
			animName = "Anim_" + std::to_string(i);
		}
		animationMap_[animName] = animation;
	}
}

void Model::SetAnimation(const std::string& name)
{
	if (animationMap_.count(name)) {
		if (currentAnimation_ != &animationMap_[name]) {
			prevAnimation_ = currentAnimation_;  // 前回のアニメ
			currentAnimation_ = &animationMap_[name];
			animationTime = 0.0f;
			blendTime_ = 0.0f; // 補間開始
		}
	}
	else {
		OutputDebugStringA(("Animation not found: " + name + "\n").c_str());
	}
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
	//for (Joint& joint : skeleton.joints)
	//{
	//	// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文。
	//	if (auto it = animation.NodeAnimations.find(joint.name); it != animation.NodeAnimations.end())
	//	{
	//		const NodeAnimation& rootNodeAnimation = (*it).second;
	//		joint.transform.translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
	//		joint.transform.rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
	//		joint.transform.scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
	//	}
	//}
	for (Joint& joint : skeleton.joints) {
		if (auto it = animation.NodeAnimations.find(joint.name); it != animation.NodeAnimations.end()) {
			const NodeAnimation& anim = it->second;

			Vector3 newTranslate = CalculateValue(anim.translate.keyframes, animationTime);
			Quaternion newRotate = CalculateValue(anim.rotate.keyframes, animationTime);
			Vector3 newScale = CalculateValue(anim.scale.keyframes, animationTime);

			if (prevAnimation_ && blendTime_ < blendDuration_) {
				if (auto prevIt = prevAnimation_->NodeAnimations.find(joint.name); prevIt != prevAnimation_->NodeAnimations.end()) {
					const NodeAnimation& prevAnim = prevIt->second;

					Vector3 oldTranslate = CalculateValue(prevAnim.translate.keyframes, animationTime);
					Quaternion oldRotate = CalculateValue(prevAnim.rotate.keyframes, animationTime);
					Vector3 oldScale = CalculateValue(prevAnim.scale.keyframes, animationTime);

					float t = blendTime_ / blendDuration_;
					joint.transform.translate = Lerp(oldTranslate, newTranslate, t);
					joint.transform.rotate = Slerpex(oldRotate, newRotate, t);
					joint.transform.scale = Lerp(oldScale, newScale, t);
					continue;
				}
			}

			joint.transform.translate = newTranslate;
			joint.transform.rotate = newRotate;
			joint.transform.scale = newScale;
		}
	}

	// 補間時間を更新
	if (blendTime_ < blendDuration_) {
		blendTime_ += 1.0f / 60.0f;
	}
	else {
		prevAnimation_ = nullptr;
	}
}

SkinCluster Model::CreateSkinCluster(
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const Skeleton& skeleton,
	const MeshData& meshData,
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize)
{
	SkinCluster skinCluster;

	auto dxCommon = DirectXCommon::GetInstance();

	//========== Palette ==========
	size_t jointCount = skeleton.joints.size();
	skinCluster.inverseBindPoseMatrices.resize(jointCount);

	// 各JointのinverseBindPoseを格納
	for (const auto& [jointName, weightData] : meshData.skinClusterData) {
		if (skeleton.jointMap.contains(jointName)) {
			int jointIndex = skeleton.jointMap.at(jointName);
			skinCluster.inverseBindPoseMatrices[jointIndex] = weightData.inverseBindPoseMatrix;
		}
	}

	skinCluster.paletteResource = dxCommon->CreateBufferResource(sizeof(WellForGPU) * jointCount);
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size()};

	// SRV登録
	uint32_t paletteIndex = SrvManager::GetInstance()->Allocate();
	skinCluster.paletteSrvHandle.first = dxCommon->GetCPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, paletteIndex);
	skinCluster.paletteSrvHandle.second = dxCommon->GetGPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, paletteIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.NumElements = static_cast<UINT>(jointCount);
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	//========== Influence（VertexInfluence） ==========
	std::vector<VertexInfluence> influences(meshData.vertices.size());
	for (const auto& [jointName, weightData] : meshData.skinClusterData) {
		int jointIndex = skeleton.jointMap.at(jointName);
		for (const auto& vw : weightData.vertexWeights) {
			auto& inf = influences[vw.vertexIndex];
			for (int i = 0; i < kNumMaxInfluence; ++i) {
				if (inf.weights[i] == 0.0f) {
					inf.weights[i] = vw.weight;
					inf.jointIndices[i] = jointIndex;
					break;
				}
			}
		}
	}

	skinCluster.influenceResource = dxCommon->CreateBufferResource(sizeof(VertexInfluence) * influences.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	skinCluster.mappedInfluence = { mappedInfluence, influences.size() };
	std::memcpy(mappedInfluence, influences.data(), sizeof(VertexInfluence) * influences.size());


	uint32_t influenceIndex = SrvManager::GetInstance()->Allocate();
	skinCluster.influenceSrvHandle.first = dxCommon->GetCPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, influenceIndex);
	skinCluster.influenceSrvHandle.second = dxCommon->GetGPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, influenceIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC influenceSrvDesc{};
	influenceSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	influenceSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	influenceSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	influenceSrvDesc.Buffer.FirstElement = 0;
	influenceSrvDesc.Buffer.NumElements = static_cast<UINT>(influences.size());
	influenceSrvDesc.Buffer.StructureByteStride = sizeof(VertexInfluence);
	influenceSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(skinCluster.influenceResource.Get(), &influenceSrvDesc, skinCluster.influenceSrvHandle.first);

	//========== InputVertex（構造体 Vertex） ==========
	skinCluster.inputVertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * meshData.vertices.size());
	VertexData* mappedInput = nullptr;
	skinCluster.inputVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInput));
	std::memcpy(mappedInput, meshData.vertices.data(), sizeof(VertexData) * meshData.vertices.size());

	uint32_t inputIndex = SrvManager::GetInstance()->Allocate();
	skinCluster.inputVertexSrvHandle.first = dxCommon->GetCPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, inputIndex);
	skinCluster.inputVertexSrvHandle.second = dxCommon->GetGPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, inputIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC inputSrvDesc{};
	inputSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	inputSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	inputSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	inputSrvDesc.Buffer.FirstElement = 0;
	inputSrvDesc.Buffer.NumElements = static_cast<UINT>(meshData.vertices.size());
	inputSrvDesc.Buffer.StructureByteStride = sizeof(VertexData);
	inputSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(skinCluster.inputVertexResource.Get(), &inputSrvDesc, skinCluster.inputVertexSrvHandle.first);

	//========== OutputVertex（UAV） ==========
	skinCluster.outputVertexResource = dxCommon->CreateBufferResourceUAV(sizeof(VertexData) * meshData.vertices.size());

	uint32_t outputIndex = SrvManager::GetInstance()->Allocate();
	skinCluster.outputVertexUavHandle.first = dxCommon->GetCPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, outputIndex);
	skinCluster.outputVertexUavHandle.second = dxCommon->GetGPUDescriptorHandle(descriptorHeap.Get(), descriptorSize, outputIndex);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = static_cast<UINT>(meshData.vertices.size());
	uavDesc.Buffer.StructureByteStride = sizeof(VertexData);
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(skinCluster.outputVertexResource.Get(), nullptr, &uavDesc, skinCluster.outputVertexUavHandle.first);

	//========== SkinningInformation（CBV） ==========
	skinCluster.skinningInfoBuffer = dxCommon->CreateBufferResource(sizeof(uint32_t));
	uint32_t* mappedCount = nullptr;
	skinCluster.skinningInfoBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedCount));
	*mappedCount = static_cast<uint32_t>(meshData.vertices.size());
	skinCluster.skinningInfoGpuAddress = skinCluster.skinningInfoBuffer->GetGPUVirtualAddress();

	return skinCluster;
}


bool Model::GetEnableLighting() const
{
	assert(material); // materialData が null でないことを確認
	return material->enableLighting != 0; // enableLighting が 0 でなければ true を返す
}

void Model::SetEnableLighting(bool enable)
{
	for (auto& instance : meshInstances_) {
		if (instance.materialData) {
			instance.materialData->enableLighting = enable ? 1 : 0;
		}
	}
}
