#include "Sword.h"
#include <CollisionTypeIdDef.h>

void Sword::Initialize()
{
    object3d_->Initialize();

    ModelManager::GetInstance()->LoadModel("sword.obj");
    object3d_->SetModel("sword.obj");

    // ローカルオフセット初期値（あとで調整）
    transform.translate = { 0.0f, 0.0f, 0.0f };
    transform.rotate = { 0.0f, 0.0f, 0.0f };
    transform.scale = { 1.0f, 1.0f, 1.0f };

    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->SetScale(transform.scale);

    // ---- コライダー初期化（球1個）----
    // ※ Shape / ShapeKind / Sphere は Player.cpp と同じ型が使える前提
    Sphere sp{};
    sp.center = { 0,0,0 };
    sp.radius = 0.0f; // 最初は無効

    Shape shape{};
    shape.kind = ShapeKind::Sphere;
    shape.sphere = sp;

    *multiCollider_ = MultiCollider(shape);

    // 種別はプロジェクトに合わせて変えてOK
    // 例：kPlayerWeapon が無ければ一旦 kPlayer にしてもいい（判定フィルタ側で調整）
    multiCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayerWeapon));
    multiCollider_->SetHitCallback([this]() { this->OnCollision(); });
}

void Sword::AttachTo(Object3d* ownerObj, const std::string& jointName)
{
    ownerObj_ = ownerObj;
    ownerJoint_ = jointName;

    // 親の指定ボーンへ
    object3d_->SetParentJoint(ownerObj_, ownerJoint_);

    // ここで Sword 側のデフォルト握りオフセットを適用（1回だけ）
    transform.translate = defaultOffsetT_;
    transform.rotate = defaultOffsetR_;
    transform.scale = defaultOffsetS_;
}

void Sword::SetLocalOffset(const Vector3& t, const Vector3& r, const Vector3& s)
{
    transform.translate = t;
    transform.rotate = r;
    transform.scale = s;
}

void Sword::Update()
{
#ifdef USE_IMGUI
	/*ImGui::PushID(this);
	char title[64];
	sprintf_s(title, "Sword##%p", this);

	if (ImGui::Begin(title)) {
		ImGui::Text("Default Grip Offset (Bone Local)");

		ImGui::DragFloat3("Def Translate", &defaultOffsetT_.x, 0.01f);
		ImGui::DragFloat3("Def Rotate", &defaultOffsetR_.x, 0.01f);
		ImGui::DragFloat3("Def Scale", &defaultOffsetS_.x, 0.01f);

		if (ImGui::Button("Apply To Current")) {
			transform.translate = defaultOffsetT_;
			transform.rotate = defaultOffsetR_;
			transform.scale = defaultOffsetS_;
		}

		ImGui::Separator();
		ImGui::Text("Current Transform");
		ImGui::DragFloat3("Translate", &transform.translate.x, 0.01f);
		ImGui::DragFloat3("Rotate", &transform.rotate.x, 0.01f);
		ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f);
	}
	ImGui::End();
	ImGui::PopID();*/
#endif // USE_IMGUI


    // 親ボーン追従 + ローカルオフセット
    object3d_->SetTranslate(transform.translate);
    object3d_->SetRotate(transform.rotate);
    object3d_->SetScale(transform.scale);
    object3d_->Update();

    // ---- コライダー更新 ----
    // 剣のワールド行列から中心を作る（簡易：剣の位置＋オフセット）
    // Matrix4x4 に Translation() がある前提（Model.cppで使ってたのでOK）
   /* Vector3 worldPos = object3d_->GetWorldMatrix().Translation();

    Sphere& sp = multiCollider_->MutableSphere(0);
    sp.center = worldPos + hitOffset_;
    sp.radius = hitEnabled_ ? hitRadius_ : 0.0f;

    isHit_ = false;*/


}

void Sword::BackGroundDraw()
{
}

void Sword::Draw()
{
    // コライダー可視化
    multiCollider_->Draw();
    // 剣モデル描画
    object3d_->Draw();
}

void Sword::ForeGroundDraw()
{
}

void Sword::ParticleDraw()
{
}

void Sword::AnimationDraw()
{
    
}

void Sword::OnCollision()
{
    // ここではフラグだけ立てる（ダメージ処理はWeapon側に寄せるのがおすすめ）
    isHit_ = true;
}
