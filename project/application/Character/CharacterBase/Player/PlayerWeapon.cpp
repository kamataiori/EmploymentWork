#include "PlayerWeapon.h"
#include "IPlayerInput.h"
#include "Player.h"
#include "Sword.h"
#include "SpinSlashSkill.h"
#include "RushSlashSkill.h"
#include "UltimateSkill.h"
#include "ITarget.h"   // IEnemyTargetProvider::AllowsUltimate を呼ぶため
#include "engine/TimeManager.h"
#include "ImGuiManager.h"
#include "MathFunctions.h"

#include <cmath>

void PlayerWeapon::Initialize()
{
	// E キーのスキル1（回転斬り）を生成
	// IPlayerSkill で保持するので、スキル追加時はここに別クラスを足すだけ
	auto spinSlash = std::make_unique<SpinSlashSkill>();
	spinSlash->Initialize(owner_, sword_);
	eSkill_ = std::move(spinSlash);

	// V キーのスキル2（突進乱舞）を生成
	// 元はアルティメットだった枠を新スキルへ移設したもの。
	// 攻撃対象の供給元は Scene 構築後に SetEnemyTargetProvider() で後から
	auto rushSlash = std::make_unique<RushSlashSkill>();
	rushSlash->Initialize(owner_);
	skill2_ = std::move(rushSlash);

	// Q キーのアルティメット（斬奪）を生成。
	// 敵一覧の供給元は Scene 構築後に SetEnemyTargetProvider() で後から注入する。
	auto ultimate = std::make_unique<UltimateSkill>();
	ultimate->Initialize(owner_);
	ultSkill_ = std::move(ultimate);
}

void PlayerWeapon::SetEnemyTargetProvider(IEnemyTargetProvider* provider)
{
	// アルティメットを撃ってよい局面かの判定にも使うので、ここでも持っておく
	provider_ = provider;

	// 敵一覧を必要とするスキル（突進乱舞・アルティメット）へそれぞれ橋渡しする。
	if (auto* rush = dynamic_cast<RushSlashSkill*>(skill2_.get())) {
		rush->SetTargetProvider(provider);
	}
	if (auto* ult = dynamic_cast<UltimateSkill*>(ultSkill_.get())) {
		ult->SetTargetProvider(provider);
	}
}

void PlayerWeapon::ForeGroundDraw()
{
	// アルティメット発動中のロックオン枠・斬撃線はスキル側が描画する。
	if (ultSkill_) ultSkill_->Draw();
}

void PlayerWeapon::StartAttack(int index)
{
	if (index < 0 || index >= kAttackStageCount_) return;

	activeAttackIndex_ = index;
	attacking_ = true;
	comboReserve_ = false;

	// 振り始めに敵の方を向く（段ごとに選び直す）
	ApplySoftLock();

	if (owner_) {
		const AttackInfo& a = normalAttacks_[index];
		// lockSec は使わない（攻撃の継続は進行度で管理する）速度倍率を渡す
		owner_->RequestAnimaKey(a.key, 10, 0.0f, a.speed);

		// ズーム演出（攻撃時の一瞬寄って戻るパンチズーム）
		if (auto* effect = owner_->GetCameraEffect()) {
			// Attack01 / Attack02 の寄り幅（従来どおり）
			constexpr float kDefaultZoomAmount = 0.04f;

			if (a.key == PlayerAnimKey::Attack03) {
				// Attack03 専用演出：「最初はゆっくりズーム → 一気にぐんっと寄る → 素早く戻す」
				//   溜め … 控えめな量を Linear でじわっと寄せて助走をつける
				//   寄り … 残りを一気に EaseOutExpo で「ぐんっ」と寄せる
				//   戻り … EaseOutCubic で素早く戻す
				constexpr float kAttack03ZoomAmount = 0.10f;  // 最終的な寄り幅
				constexpr float kAttack03PreZoomAmount = 0.025f; // 溜めで先に寄る量
				constexpr float kAttack03PreDuration = 0.50f;  // 溜めの時間（ゆっくり）
				constexpr float kAttack03InDuration = 0.08f;  // 一気に寄る時間
				constexpr float kAttack03OutDuration = 0.18f;  // 戻る時間

				CameraEffectController::ZoomPunchParams punch{};
				punch.zoomAmount = kAttack03ZoomAmount;
				punch.preZoomAmount = kAttack03PreZoomAmount;
				punch.preDuration = kAttack03PreDuration;
				punch.inDuration = kAttack03InDuration;
				punch.outDuration = kAttack03OutDuration;
				punch.preEasing = Tween::Easing::Linear;       // 溜め
				punch.inEasing = Tween::Easing::EaseOutExpo;  // 寄り
				punch.outEasing = Tween::Easing::EaseOutCubic; // 戻り
				effect->StartZoomPunch(punch);
			}
			else {
				// Attack01 / Attack02：従来どおりの素直なパンチズーム
				constexpr float kDefaultInDuration = 0.06f;
				constexpr float kDefaultOutDuration = 0.18f;
				effect->StartZoomPunch(kDefaultZoomAmount,
					kDefaultInDuration, kDefaultOutDuration);
			}
		}
	}
}

void PlayerWeapon::Update()
{
	if (!owner_) return;

	// ===== スキル発動中は通常攻撃の処理を止める =====
	// （剣のワールド配置・当たり判定はスキル側が行う。駆動は UpdateSkills() で）
	if (IsAnySkillActive()) {
		return;
	}

	// ===== 通常攻撃の進行（コンボ／キャンセル／終了） =====
	if (attacking_) {
		// アニメ進行度（再生速度を変えても 0〜1 で進む）
		const float progress = owner_->GetAnimationProgress();
		const AttackInfo& cur = normalAttacks_[activeAttackIndex_];

		// コンボ予約があり、受付進行度を超えたら次段へ
		if (comboReserve_ && activeAttackIndex_ < kAttackStageCount_ - 1 && progress >= cur.comboAt) {
			StartAttack(activeAttackIndex_ + 1);
		}
		// 振り終わり後に移動入力が入ったら、戻りモーションを打ち切って移動へ移れる。
		// （攻撃中は移動できない代わりに、振り終わった瞬間から動けるようにして待ち時間を無くす）
		else if (progress >= cur.cancelAt && HasMoveInput()) {
			EndAttack();
		}
		// 何も入力が無ければアニメを最後まで再生してから終了
		else if (progress >= kAttackEndProgress_) {
			EndAttack();
		}
	}

	// 通常攻撃の当たり判定ウィンドウを剣へ反映（非攻撃時は false ＝判定なし）
	if (sword_) sword_->SetHitEnabled(IsHitActive());
}


void PlayerWeapon::UpdateSkills()
{
	if (!owner_) return;
	if (!IsAnySkillActive()) return;

	const float dt = TimeManager::GetInstance()->GetDeltaTime();
	// 同時に発動しない前提だが、発動中のスキルだけを駆動する。
	if (eSkill_ && eSkill_->IsActive())     eSkill_->Update(dt);
	if (skill2_ && skill2_->IsActive())     skill2_->Update(dt);
	if (ultSkill_ && ultSkill_->IsActive()) ultSkill_->Update(dt);
}

void PlayerWeapon::Draw()
{

}

void PlayerWeapon::DebugImGui()
{
#ifdef USE_IMGUI
	if (!showDebug_) return;

	// 調整スライダーの範囲（進行度 0〜1）
	constexpr float kProgressMin = 0.0f;
	constexpr float kProgressMax = 1.0f;

	ImGui::Begin("Player Attack");

	ImGui::Text("attacking : %s", attacking_ ? "YES" : "no");
	ImGui::Text("stage     : %d", activeAttackIndex_);
	ImGui::Text("progress  : %.2f", owner_ ? owner_->GetAnimationProgress() : 0.0f);
	ImGui::Text("combo     : %s", comboReserve_ ? "reserved" : "-");
	ImGui::Separator();
	ImGui::TextWrapped("cancelAt を過ぎると移動入力で戻りモーションを打ち切れる（振りの長さ自体はアニメ側）。");

	for (int i = 0; i < kAttackStageCount_; ++i) {
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(i == 0 ? "Stage 1" : (i == 1 ? "Stage 2" : "Stage 3"))) {
			AttackInfo& a = normalAttacks_[i];
			ImGui::SliderFloat("hitStart", &a.hitStart, kProgressMin, kProgressMax);
			ImGui::SliderFloat("hitEnd", &a.hitEnd, kProgressMin, kProgressMax);
			ImGui::SliderFloat("comboAt", &a.comboAt, kProgressMin, kProgressMax);
			ImGui::SliderFloat("cancelAt", &a.cancelAt, kProgressMin, kProgressMax);
		}
		ImGui::PopID();
	}

	ImGui::Separator();

	// ===== ソフトロック（振り始めに敵の方を向く）=====
	// スライダーの範囲
	constexpr float kTurnSecMax = 0.40f;    // 向き切る時間の上限[秒]
	constexpr float kRangeMin = 1.0f;     // 射程の下限[m]
	constexpr float kRangeMax = 20.0f;    // 射程の上限[m]
	constexpr float kAngleMin = 10.0f;    // 許容角の下限[度]
	constexpr float kAngleMax = 180.0f;   // 許容角の上限[度]
	constexpr float kWeightMax = 10.0f;    // スコア重みの上限

	ImGui::Text("Soft Lock");
	ImGui::Checkbox("enabled", &softLockEnabled_);
	ImGui::SliderFloat("turn sec", &softLockTurnSeconds_, 0.0f, kTurnSecMax);

	TargetPicker::Params& pick = targetPicker_.MutableParams();
	ImGui::SliderFloat("range [m]", &pick.maxDistance, kRangeMin, kRangeMax);
	ImGui::SliderFloat("angle [deg]", &pick.maxAngleDeg, kAngleMin, kAngleMax);
	ImGui::SliderFloat("weight angle", &pick.angleWeight, 0.0f, kWeightMax);
	ImGui::SliderFloat("weight dist", &pick.distanceWeight, 0.0f, kWeightMax);

	// 直近の選択結果（向かないときの切り分け用）
	if (targetPicker_.WasFound()) {
		ImGui::Text("last pick : %.1f deg / %.1f m",
			targetPicker_.GetLastAngleDeg(), targetPicker_.GetLastDistance());
	}
	else {
		ImGui::Text("last pick : none");
	}

	ImGui::End();
#endif
}

void PlayerWeapon::ApplySoftLock()
{
	if (!softLockEnabled_ || !owner_ || !provider_) return;

	// 方向が決まらないとみなす長さ
	constexpr float kDirectionEpsilon = 0.0001f;

	const Vector3 selfPos = owner_->GetTransform().translate;

	// 基準方向：移動入力があればその方向、無ければ今の向き。
	// 「倒している方向の敵を斬る」ので、対象の切り替えが操作でできる。
	Vector3 basisDir = owner_->GetInputDirectionXZ();
	if (Length(basisDir) <= kDirectionEpsilon) {
		const float currentYaw = owner_->GetTransform().rotate.y;
		basisDir = { std::sin(currentYaw), 0.0f, std::cos(currentYaw) };
	}

	ITarget* target = targetPicker_.Pick(provider_, selfPos, basisDir);
	if (!target) return;   // 見つからなければ向きを変えない（空振りはそのまま空振る）

	const Vector3 to = target->GetTargetCenter() - selfPos;
	if (std::fabs(to.x) <= kDirectionEpsilon && std::fabs(to.z) <= kDirectionEpsilon) return;

	// 高さは無視してヨー角だけ合わせる
	owner_->StartFaceTurn(std::atan2(to.x, to.z), softLockTurnSeconds_);
}

void PlayerWeapon::EndAttack()
{
	attacking_ = false;
	comboReserve_ = false;
	if (owner_) {
		owner_->EndAttackState(); // 優先度を解除して移動アニメへ戻れるようにする
	}
}

bool PlayerWeapon::HasMoveInput() const
{
	if (!playerInput_) return false;

	// 軸はデッドゾーン処理済みなので、0 でなければ入力が入っている
	return playerInput_->GetMoveX() != 0.0f || playerInput_->GetMoveY() != 0.0f;
}

void PlayerWeapon::NormalAttack()
{
	// スキル中は通常攻撃を出せない
	if (IsAnySkillActive()) return;

	// 攻撃入力（どのボタンに割り当てるかはシーンの入力クラスが持つ）
	const bool attackHeld = playerInput_ && playerInput_->IsAttackHeld();
	const bool attackTriggered = playerInput_ && playerInput_->IsAttackTriggered();

	// ===== 攻撃中：長押しでも次段を予約できる =====
	if (attacking_) {
		if (attackHeld) {
			comboReserve_ = true; // 押されてる間ずっと予約ON
		}
		return;
	}

	// ===== 攻撃してない：開始は「押した瞬間」だけ =====
	if (!attackTriggered) {
		return;
	}

	// 新規コンボは必ず1段目から
	StartAttack(0);
}


bool PlayerWeapon::IsHitActive() const
{
	if (!attacking_ || !owner_) return false;

	// 当たり判定は進行度ウィンドウ内だけ有効。
	// 振りの当たりフレーム帯（hitStart〜hitEnd）に合わせて調整する
	const float progress = owner_->GetAnimationProgress();
	const AttackInfo& a = normalAttacks_[activeAttackIndex_];
	return (progress >= a.hitStart && progress <= a.hitEnd);
}


void PlayerWeapon::Skill()
{
	// 発動は押した瞬間だけ（立ち上がり検出は入力側が持つ）
	const bool triggered = playerInput_ && playerInput_->IsSkillTriggered();

	if (!triggered) return;
	if (IsAnySkillActive()) return; // 多重発動はしない
	if (!eSkill_) return;
	if (eCharges_ <= 0) return;     // 使い切ったら出せない

	// 進行中の通常攻撃は打ち切ってからスキルへ移行する
	attacking_ = false;
	comboReserve_ = false;

	--eCharges_; // 発動が成立したここで1回消費する
	eSkill_->Start();
}

void PlayerWeapon::Skill2()
{
	// 発動は押した瞬間だけ（立ち上がり検出は入力側が持つ）
	const bool triggered = playerInput_ && playerInput_->IsSkill2Triggered();

	if (!triggered) return;
	if (IsAnySkillActive()) return; // 多重発動はしない（他スキル・乱舞中は不可）
	if (!skill2_) return;
	if (skill2Charges_ <= 0) return; // 使い切ったら出せない

	// 前哨戦（四隅の Sentinel・中央コア）しか居ない局面では撃たせない。
	// 突進乱舞は敵の位置まで飛び込んで斬る技なので、動かない前哨相手では成立しない。
	if (provider_ && !provider_->AllowsDashSkills()) return;

	// 進行中の通常攻撃は打ち切ってからスキル2（突進乱舞）へ移行する
	attacking_ = false;
	comboReserve_ = false;

	--skill2Charges_; // 発動が成立したここで1回消費する
	skill2_->Start();
}

void PlayerWeapon::Ultimate()
{
	// 発動は押した瞬間だけ（立ち上がり検出は入力側が持つ）
	const bool triggered = playerInput_ && playerInput_->IsUltimateTriggered();

	if (!triggered) return;
	if (IsAnySkillActive()) return; // 多重発動はしない（他スキル発動中は不可）
	if (!ultSkill_) return;
	if (ultCharges_ <= 0) return;   // 使い切ったら出せない

	// 前哨戦（四隅の Sentinel・中央コア）しか居ない局面では撃たせない。
	// 斬奪は敵の真上へ飛び込んで斬る技なので、動かない前哨相手では成立しない。
	if (provider_ && !provider_->AllowsDashSkills()) return;

	// 進行中の通常攻撃は打ち切ってからアルティメットへ移行する
	attacking_ = false;
	comboReserve_ = false;

	--ultCharges_; // 発動が成立したここで1回消費する
	ultSkill_->Start();
}
