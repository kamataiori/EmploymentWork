#include "Enemy.h"
#include <CollisionTypeIdDef.h>
#include <cmath>

static inline float LenXZ(const Vector3& d) { return std::sqrt(d.x * d.x + d.z * d.z); }
//
//Enemy::Enemy(BaseScene* scene)
//{
//}

void Enemy::Initialize()
{
	object3d_->Initialize();

	// モデル読み込み
	/*ModelManager::GetInstance()->LoadModel("uvChecker.gltf");
	ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");*/
	ModelManager::GetInstance()->LoadModel("matest.obj");
	ModelManager::GetInstance()->LoadModel("Skeleton.gltf");
	ModelManager::GetInstance()->LoadModel("Sam.gltf");
	ModelManager::GetInstance()->LoadModel("uvChecker.gltf");

	object3d_->SetModel("uvChecker.gltf");

	// 初期Transform設定
	transform.translate = { 0.0f, 0.0f, 0.0f };
	transform.rotate = { 0.0f, 0.0f, 0.0f };
	transform.scale = { 1.0f, 1.0f, 1.0f };

	// object3dにtransformを反映
	object3d_->SetTranslate(transform.translate);
	object3d_->SetRotate(transform.rotate);
	object3d_->SetScale(transform.scale);


	// コライダーの初期化
	SetCollider(this);
	SetPosition(object3d_->GetTranslate());  // 3Dモデルの位置にコライダーをセット
	//SetRotation(object3d_->GetRotate());
	//SetScale(object3d_->GetScale());
	sphere.radius = 1.5f;

	SphereCollider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));

	BuildBehaviorTree();
	BuildBTView();
}

void Enemy::Update()
{
	const float dt = GetDeltaTime();

	// ターゲット距離（XZ）
	if (bb_.target) {
		Vector3 d = bb_.target->GetTransform().translate - transform.translate;
		d.y = 0.0f;
		bb_.distToTarget = LenXZ(d);
	}
	else {
		bb_.distToTarget = 9999.0f;
	}

	// BT実行
	tree_.Tick(bb_, dt);

	// 可視化
	DrawBTView();

	//ImGui::Begin("IMGUI SMOKE");
	//ImGui::Text("frame=%d", ImGui::GetFrameCount());
	//ImGui::End();

	//// ★テスト：ctx を作って直に 1 ノードだけ出す
	//static ax::NodeEditor::EditorContext* testCtx = nullptr;
	//if (!testCtx) {
	//	ax::NodeEditor::Config cfg; cfg.SettingsFile = nullptr;
	//	testCtx = ax::NodeEditor::CreateEditor(&cfg);
	//}

	//ax::NodeEditor::SetCurrentEditor(testCtx);
	//ImGui::Begin("NE SMOKE");
	//ax::NodeEditor::Begin("NE");
	//ax::NodeEditor::BeginNode(ax::NodeEditor::NodeId(1));
	//ImGui::Text("hello node-editor");
	//ax::NodeEditor::EndNode();
	//ax::NodeEditor::End();
	//ImGui::End();
	//ax::NodeEditor::SetCurrentEditor(nullptr);



	object3d_->SetTranslate(transform.translate);
	object3d_->SetScale(transform.scale);
	object3d_->SetRotate(transform.rotate);
	object3d_->Update();

	SetPosition(object3d_->GetTranslate());
	sphere.color = static_cast<int>(Color::WHITE);
}

void Enemy::Draw()
{
	object3d_->Draw();

	// SphereCollider の描画
	//SphereCollider::Draw();
}

void Enemy::SkinningDraw()
{
}

void Enemy::ParticleDraw()
{
}

void Enemy::OnCollision()
{
	sphere.color = static_cast<int>(Color::RED);
}

void Enemy::BuildBehaviorTree() {
	// 追尾アクション（これだけ）
	auto actChase = std::make_unique<BTAction>("ChaseTarget",
		[this](BTBlackboard& bb, float dt)->BTStatus {
			if (!bb.target) return BTStatus::Failure;
			Vector3 to = bb.target->GetTransform().translate - transform.translate;
			to.y = 0.0f;
			float dist = LenXZ(to);
			if (dist <= stopDistance_) return BTStatus::Success;

			if (dist > 1e-6f) {
				to.x /= dist; to.z /= dist;
				transform.translate.x += to.x * bb.moveSpeed * dt;
				transform.translate.z += to.z * bb.moveSpeed * dt;
			}
			return BTStatus::Running;
		});
	BTNode* rawChase = actChase.get();

	// Root = Selector( Chase )
	auto root = std::make_unique<BTSelector>("Root");
	BTNode* rawRoot = root.get();
	root->Add(std::move(actChase));
	tree_.SetRoot(std::move(root));

	// 可視化用保持
	nodeRoot_ = rawRoot;
	nodeChase_ = rawChase;
}

float Enemy::GetDeltaTime() const {
	// あなたのエンジンのΔt関数に差し替え
	return 1.0f / 60.0f;
}

void Enemy::BuildBTView() {
	if (!visGraph_) visGraph_ = std::make_unique<btvis::GraphView>(&visIds_);

	auto* vRoot = visGraph_->AddNode<btvis::CompositeNodeView>("Root");
	auto* vChs = visGraph_->AddNode<btvis::ActionNodeView>("ChaseTarget");

	// 一度だけで十分（描画フレーム内で遅延実行されます）
	visGraph_->AutoLayoutGrid();

	visBind_.clear();
	visBind_[nodeRoot_] = vRoot;
	visBind_[nodeChase_] = vChs;

	tree_.SetVisual([this](const BTNode* n, BTStatus s) {
		auto it = visBind_.find(n);
		if (it == visBind_.end()) return;
		using ES = btvis::ExecState;
		ES st = (s == BTStatus::Running) ? ES::Running
			: (s == BTStatus::Success) ? ES::Succeeded
			: ES::Failed;
		it->second->SetExecState(st);
		});
}

void Enemy::DrawBTView() {

	visGraph_->Draw("Enemy BT");

}