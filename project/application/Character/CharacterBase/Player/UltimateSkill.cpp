#include "UltimateSkill.h"
#include "Player.h"
#include "ITarget.h"
#include "FollowCamera.h"
#include "Camera/Camera.h"
#include "Sprite.h"
#include "WinApp.h"
#include "engine/TimeManager.h"
#include "MathFunctions.h"
#include <Input.h>

#include <cmath>
#include <algorithm>

namespace {
	// 角度差を [-π, π] に丸める（最短回りで補間するため）。
	float WrapPi(float a) { return std::atan2(std::sin(a), std::cos(a)); }

	// cur を target へ、時定数 sharp で滑らかに寄せる（フレームレート非依存）。
	float Approach(float cur, float target, float sharp, float dt) {
		return cur + (target - cur) * (1.0f - std::exp(-sharp * dt));
	}

	// なめらかな 0→1 補間（両端で速度0）。
	float SmoothStep(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
}

UltimateSkill::UltimateSkill() = default;
UltimateSkill::~UltimateSkill() = default;

void UltimateSkill::Initialize(Player* owner)
{
	owner_ = owner;

	// 斬撃線・ロックオン枠はすべて白テクスチャを引き伸ばして色を乗せて使う。
	const std::string kWhiteTex = "Resources/white.png";

	slashLine_ = std::make_unique<Sprite>();
	slashLine_->Initialize(kWhiteTex);
	slashLine_->SetAnchorPoint({ 0.5f, 0.5f }); // 中心を軸に回転させる

	for (auto& edge : reticle_) {
		edge = std::make_unique<Sprite>();
		edge->Initialize(kWhiteTex);
		edge->SetAnchorPoint({ 0.5f, 0.5f });
	}

	for (auto& edge : innerBox_) {
		edge = std::make_unique<Sprite>();
		edge->Initialize(kWhiteTex);
		edge->SetAnchorPoint({ 0.5f, 0.5f });
	}
}

void UltimateSkill::Start()
{
	if (!owner_ || !provider_) return;

	// 進行中の通常攻撃などを打ち切ってからアルティメットへ移行する前提で呼ばれる。
	struck_.clear();

	// 最初のターゲットはプレイヤー位置から一番近い敵。
	ITarget* first = FindNearestUnstruck(owner_->GetTransform().translate);
	if (!first) {
		// 対象がいなければ発動不成立。
		active_ = false;
		return;
	}

	active_ = true;
	phase_ = Phase::Aim;
	currentTarget_ = first;
	steerTimer_ = 0.0f; // 最初の対象へカメラを寄せ始める
	groundY_ = owner_->GetTransform().translate.y; // 終了時にこの高さへ戻す
	arrived_ = false;
	cineT_ = 0.0f;
	cineWeight_ = 0.0f;
	camHeightDrop_ = 0.0f; // 到着後に高さを落とす進捗を初期化
	sideTarget_ = 1.0f;  // 既定は右
	sideCurrent_ = 1.0f;
	lockOnTimer_ = 0.0f; // 最初の対象へロックオン枠を集める
	commitTimer_ = 0.0f;
	blinkPhase_ = 0.0f;
	slowEngaged_ = false; // スローは真上到着時に入れる（Q直後はまだ入れない）
	approachStartDist_ = -1.0f; // 飛び込み進捗を最初から測り直す

	// 発動中は被弾しない＆通常移動・通常攻撃を止める（狙いに集中させる）。
	owner_->SetInvincible(true);
	owner_->SetInputLocked(true);

	// マウスは斬撃の照準に使うが、カメラも動かせるようにしておく。
	// ただしスローに合わせて感度を落とし、ゆっくり回るようにする。
	if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
		fc->SetMouseSensitivityScale(kSlowCameraSensScale_);
	}

	// スローは「敵の真上に着いた瞬間」に入れる（Update内で arrived_ を見て一度だけ）。
	// Q直後は通常速度のまま高速で敵の真上へ飛び込ませ、着いてから斬奪のスローに入る。
	// （SetTimeScale はヒットストップ層と乗算合成されるため既存演出を壊さない）
}

void UltimateSkill::Update(float /*dt*/)
{
	if (!active_) return;

	switch (phase_) {
	case Phase::Aim:
	{
		// 対象が途中で消えた（別要因で撃破された等）なら次の最近傍へ。いなければ終了。
		if (!currentTarget_ || !currentTarget_->IsAlive()) {
			currentTarget_ = FindNearestUnstruck(owner_->GetTransform().translate);
			if (!currentTarget_) { Finish(); return; }
			steerTimer_ = 0.0f; // 乗り換えた対象へ寄せ直す
			arrived_ = false;
			cineT_ = 0.0f;
			lockOnTimer_ = 0.0f; // 新しい対象へロックオン枠を集め直す
			commitTimer_ = 0.0f;
			blinkPhase_ = 0.0f;
		}

		const float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

		// 接近が終わるまでは通常追従＋自動フレーミングでリードインする。
		if (!arrived_ && steerTimer_ < kFrameSteerSec_) {
			steerTimer_ += unscaledDt;
			SteerCameraFraming(unscaledDt);
		}

		// ロックオン中の対象へ滑らかに接近する（テレポートではなくグライド）。arrived_ を更新。
		ApproachTarget(unscaledDt);

		// 敵の真上に到着した瞬間だけスローへ入れ、以降ロックオン枠の収束時間を計る。
		// （到着前は四角・線・スローを一切出さず、通常速度の高速接近だけを見せる）
		if (arrived_) {
			if (!slowEngaged_) {
				TimeManager::GetInstance()->SetTimeScale(kSlowTimeScale_);
				slowEngaged_ = true;
			}
			lockOnTimer_ += unscaledDt;
		}

		Input* input = Input::GetInstance();

		// 接近完了後は追従を離れ、見下ろし演出カメラへ移行する（上空・見下ろしの位置で止める）。
		if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
			if (arrived_) {
				cineWeight_ = Approach(cineWeight_, 1.0f, kCineBlendSharp_, unscaledDt);
				// 到着後、カメラの高さだけを徐々にプレイヤーの高さへ落とす（水平位置は動かさない）。
					camHeightDrop_ = Approach(camHeightDrop_, 1.0f, kCamHeightDropSharp_, unscaledDt);
					// 真上（見下ろし）にいるとき、右クリックでカメラの左右を反対側へ入れ替える。
				if (input->TriggerMouseButton(1)) {
					sideTarget_ = -sideTarget_;
				}
			} else {
				cineT_ = 0.0f;
				cineWeight_ = Approach(cineWeight_, 0.0f, kCineBlendSharp_, unscaledDt);
					camHeightDrop_ = 0.0f; // 接近中は演出の高さのまま
			}
			// 左右は滑らかに反対側へスライドさせる。
			sideCurrent_ = Approach(sideCurrent_, sideTarget_, kSideSwapSharp_, unscaledDt);

			ComputeCinematicPose(cineHoldPos_, cineHoldLook_);
			fc->SetCinematicBlend(cineHoldPos_, cineHoldLook_, cineWeight_);
		}

		// 枠が集まりきり、かつ見下ろし位置に着いたら「コミット猶予」開始。
		// 内側の色違い四角がだんだん速く点滅し、猶予内に斬らなければ Ultimate 終了。
		if (arrived_ && lockOnTimer_ >= kReticleConvergeSec_) {
			commitTimer_ += unscaledDt;
			const float ct = std::clamp(commitTimer_ / kCommitWindowSec_, 0.0f, 1.0f);
			const float freq = kBlinkFreqStart_ + (kBlinkFreqEnd_ - kBlinkFreqStart_) * ct;
			blinkPhase_ += freq * unscaledDt;
			if (commitTimer_ >= kCommitWindowSec_) { Finish(); return; } // 時間切れ＝終了
		}

		// 斬撃線の向きは斬るたびに交互（偶数=縦/左右, 奇数=横/上下）。
		lineVertical_ = (struck_.size() % 2 == 0);

		// 斬撃線はマウス位置に追従。縦線はX(左右)、横線はY(上下)だけ動かす。画面内へクランプ。
		const POINT m = input->GetMousePosition();
		if (lineVertical_) {
			lineX_ = std::clamp(static_cast<float>(m.x), 0.0f,
			                    static_cast<float>(WinApp::kClientWidth));
		} else {
			lineY_ = std::clamp(static_cast<float>(m.y), 0.0f,
			                    static_cast<float>(WinApp::kClientHeight));
		}

		// ロックオン枠の位置（対象のスクリーン座標）と、線が枠に合っているかを判定。
		targetOnScreen_ = WorldToScreen(currentTarget_->GetTargetCenter(),
		                                targetScreenX_, targetScreenY_);
		aligned_ = targetOnScreen_ && IsLineOnTarget();

		// 左クリックで斬撃を確定。枠内なら大ダメージ、枠外なら通常ダメージ。
		// （真上に着いてロックオン枠が出てからのみ斬れる＝接近中の誤クリックを弾く）
		if (arrived_ && input->TriggerMouseButton(0)) {
			currentTarget_->ApplyDamage(aligned_ ? kSuccessDamage_ : kFailDamage_);

			// 斬りモーション：縦線=Attack03 / 横線=Attack01・02（横同士は交互）。
			// 連続でも毎回頭から再生させるため forceRestart=true。
			PlayerAnimKey animKey;
			if (lineVertical_) {
				animKey = PlayerAnimKey::Attack03;
			} else {
				animKey = ((struck_.size() / 2) % 2 == 0)
					? PlayerAnimKey::Attack01 : PlayerAnimKey::Attack02;
			}
			struck_.push_back(currentTarget_);
			owner_->RequestAnimaKey(animKey, kSlashAnimPriority_,
			                        kSlashAnimLockSec_, kSlashAnimSpeed_, /*forceRestart=*/true);

			// 斬った瞬間の手応え：ズームパンチ＋シェイク（Slash中は等倍時間で効く）。
			if (auto* fx = owner_->GetCameraEffect()) {
				fx->StartZoomPunch(kZoomPunchAmount_, kZoomPunchIn_, kZoomPunchOut_);
				fx->StartSimpleShake(kShakeDuration_, kShakeAmplitude_);
			}

			// 演出カメラ→通常追従へのイージング戻しを開始する（今の weight を起点に 0 へ）。
			returnTimer_ = 0.0f;
			returnStartWeight_ = cineWeight_;

			// 斬った瞬間だけ通常速度へ戻し、振りが終わるまで待つ（Slash 局面へ）。
			TimeManager::GetInstance()->SetTimeScale(1.0f);
			slowEngaged_ = false; // 次の対象へは通常速度で寄せ、真上到着で再スロー
			phase_ = Phase::Slash;
		}
		break;
	}
	case Phase::Slash:
	{
		const float unscaledDt = TimeManager::GetInstance()->GetUnscaledDeltaTime();

		// 撃った瞬間から、演出カメラを通常追従へイージングで戻す（ポーズは撃った時点で固定）。
		// ease-in-out で「スッと動いて、ふわっと収まる」戻り方にする。
		if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
			returnTimer_ += unscaledDt;
			const float t = std::clamp(returnTimer_ / kCineReturnDuration_, 0.0f, 1.0f);
			cineWeight_ = returnStartWeight_ * (1.0f - SmoothStep(t));
			if (t >= 1.0f || cineWeight_ < 0.001f) {
				cineWeight_ = 0.0f;
				fc->ClearCinematic();
			} else {
				fc->SetCinematicBlend(cineHoldPos_, cineHoldLook_, cineWeight_);
			}
		}

		// 通常速度で斬りモーションを再生中。振り終わりを進行度で待つ。
		if (owner_->GetAnimationProgress() < kSlashEndProgress_) return;

		// 次に近い未斬撃の敵へ。いなくなったら終了。
		currentTarget_ = FindNearestUnstruck(owner_->GetTransform().translate);
		if (!currentTarget_) { Finish(); return; }

		// 次の対象へは通常速度のまま高速で寄せ直し、真上到着でまた斬奪スローに入る。
		// （slowEngaged_ は斬撃時に false 済み。ApproachTarget 到着時に再度スローを入れる）
		steerTimer_ = 0.0f;
		arrived_ = false;
		cineT_ = 0.0f;
		lockOnTimer_ = 0.0f; // 次の対象へロックオン枠を集め直す
		commitTimer_ = 0.0f;
		blinkPhase_ = 0.0f;
		approachStartDist_ = -1.0f; // 次の対象への飛び込み進捗を測り直す
		phase_ = Phase::Aim;
		break;
	}
	}
}

void UltimateSkill::Draw()
{
	// ロックオン枠・斬撃線は狙い(Aim)中だけ出す。斬り(Slash)中はカメラ演出に集中させる。
	// さらに「敵の真上に着いてから」だけ描画する（到着前は四角・線を一切出さない）。
	if (!active_ || phase_ != Phase::Aim || !arrived_ || !targetOnScreen_) return;

	// 成功域(=貫通中)は緑、外している間は赤で「まだ」を伝える。
	const Vector4 kAlignedColor{ 0.2f, 1.0f, 0.4f, 1.0f };
	const Vector4 kNormalColor { 1.0f, 0.25f, 0.25f, 0.9f };
	const Vector4 color = aligned_ ? kAlignedColor : kNormalColor;

	const float screenW = static_cast<float>(WinApp::kClientWidth);
	const float screenH = static_cast<float>(WinApp::kClientHeight);

	// --- 斬撃線：縦なら幅いっぱいの縦線(マウスX追従)、横なら幅いっぱいの横線(マウスY追従) ---
	if (lineVertical_) {
		slashLine_->SetPosition({ lineX_, screenH * 0.5f });
		slashLine_->SetSize({ kLineThickness_, screenH });
	} else {
		slashLine_->SetPosition({ screenW * 0.5f, lineY_ });
		slashLine_->SetSize({ screenW, kLineThickness_ });
	}
	slashLine_->SetRotation(0.0f);
	slashLine_->SetColor(color);
	slashLine_->Update();
	slashLine_->Draw();

	// --- ロックオン枠：四辺が外側から集まってくる演出 ---
	// 取得直後は大きな枠(kReticleStartSize_)から始まり、kReticleConvergeSec_ で本来サイズへ収束。
	const float convT = SmoothStep((kReticleConvergeSec_ > 0.0f)
		? (lockOnTimer_ / kReticleConvergeSec_) : 1.0f);
	const float half = kReticleStartSize_ + (kReticleHalfSize_ - kReticleStartSize_) * convT;
	const float thick = kReticleThickness_;
	const float span = half * 2.0f;
	const float tx = targetScreenX_;
	const float ty = targetScreenY_;

	// 上辺・下辺は横長、左辺・右辺は縦長。
	reticle_[0]->SetPosition({ tx,        ty - half }); reticle_[0]->SetSize({ span,  thick }); // 上
	reticle_[1]->SetPosition({ tx,        ty + half }); reticle_[1]->SetSize({ span,  thick }); // 下
	reticle_[2]->SetPosition({ tx - half, ty        }); reticle_[2]->SetSize({ thick, span  }); // 左
	reticle_[3]->SetPosition({ tx + half, ty        }); reticle_[3]->SetSize({ thick, span  }); // 右

	for (auto& edge : reticle_) {
		edge->SetRotation(0.0f);
		edge->SetColor(color);
		edge->Update();
		edge->Draw();
	}

	// --- 収束後：内側で点滅する小さな色違いの四角（コミット猶予の合図）---
	// 枠が集まりきり、見下ろし位置に着いたら表示。点滅は blinkPhase_ でだんだん速くなる。
	if (arrived_ && lockOnTimer_ >= kReticleConvergeSec_) {
		const bool blinkOn = std::fmod(blinkPhase_, 1.0f) < 0.5f;
		if (blinkOn) {
			const Vector4 kInnerColor{ 1.0f, 0.85f, 0.1f, 1.0f }; // 黄色系（枠と色違い）
			const float ih = kInnerReticleHalf_;
			const float it = kReticleThickness_;
			const float ispan = ih * 2.0f;
			// 外枠と同じく四辺のみ（中は塗りつぶさない）。
			innerBox_[0]->SetPosition({ tx,      ty - ih }); innerBox_[0]->SetSize({ ispan, it    }); // 上
			innerBox_[1]->SetPosition({ tx,      ty + ih }); innerBox_[1]->SetSize({ ispan, it    }); // 下
			innerBox_[2]->SetPosition({ tx - ih, ty      }); innerBox_[2]->SetSize({ it,    ispan }); // 左
			innerBox_[3]->SetPosition({ tx + ih, ty      }); innerBox_[3]->SetSize({ it,    ispan }); // 右
			for (auto& edge : innerBox_) {
				edge->SetRotation(0.0f);
				edge->SetColor(kInnerColor);
				edge->Update();
				edge->Draw();
			}
		}
	}
}

ITarget* UltimateSkill::FindNearestUnstruck(const Vector3& fromPos)
{
	scratch_.clear();
	provider_->CollectAliveTargets(scratch_);

	ITarget* best = nullptr;
	float bestDistSq = 0.0f;

	for (ITarget* t : scratch_) {
		if (!t || !t->IsAlive()) continue;
		// この発動で既に斬った敵は除外（同じ敵を二度斬らない）。
		if (std::find(struck_.begin(), struck_.end(), t) != struck_.end()) continue;

		const Vector3 d = t->GetTargetCenter() - fromPos;
		const float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
		if (!best || distSq < bestDistSq) {
			best = t;
			bestDistSq = distSq;
		}
	}
	return best;
}

bool UltimateSkill::WorldToScreen(const Vector3& world, float& outX, float& outY) const
{
	if (!owner_) return false;
	Camera* cam = owner_->GetCamera();
	if (!cam) return false;

	// ワールド座標 → クリップ → NDC → スクリーン（DamagePopupManager と同じ手順）。
	const Matrix4x4& vp = cam->GetViewProjectionMatrix();
	const float clipX = world.x * vp.m[0][0] + world.y * vp.m[1][0] + world.z * vp.m[2][0] + vp.m[3][0];
	const float clipY = world.x * vp.m[0][1] + world.y * vp.m[1][1] + world.z * vp.m[2][1] + vp.m[3][1];
	const float clipW = world.x * vp.m[0][3] + world.y * vp.m[1][3] + world.z * vp.m[2][3] + vp.m[3][3];
	if (clipW <= 1e-5f) return false; // カメラ背後は投影しない

	const float ndcX = clipX / clipW;
	const float ndcY = clipY / clipW;
	outX = (ndcX + 1.0f) * 0.5f * WinApp::kClientWidth;
	outY = (1.0f - ndcY) * 0.5f * WinApp::kClientHeight;
	return true;
}

bool UltimateSkill::IsLineOnTarget() const
{
	const float half = kReticleHalfSize_;
	if (lineVertical_) {
		// 縦線(スクリーンX)が枠の横範囲内。
		return (lineX_ >= targetScreenX_ - half) && (lineX_ <= targetScreenX_ + half);
	}
	// 横線(スクリーンY)が枠の縦範囲内。
	return (lineY_ >= targetScreenY_ - half) && (lineY_ <= targetScreenY_ + half);
}

void UltimateSkill::ComputeCinematicPose(Vector3& outPos, Vector3& outLook) const
{
	const Vector3 p = owner_->GetTransform().translate;             // プレイヤー（上空にいる）
	const Vector3 c = currentTarget_ ? currentTarget_->GetTargetCenter() : p; // ロックオン対象

	// プレイヤー→敵の水平方向（＝前方）と、その右方向。
	Vector3 toH = c - p;
	toH.y = 0.0f;
	const Vector3 dirH = (Length(toH) > 1e-4f) ? Normalize(toH) : Vector3{ 0.0f, 0.0f, 1.0f };
	const Vector3 rightH = { dirH.z, 0.0f, -dirH.x }; // 前方に対する右

	// 開始(cineT_=0)：プレイヤーの後ろ・右・上から、前方の敵を見下ろす。
	// （プレイヤーが画面手前＝下、敵が画面中央になる構図）
	const float rightAmount = kCineStartRight_ * sideCurrent_; // +で右 / -で左（右クリックで反転）
	const Vector3 startPos = {
		p.x - dirH.x * kCineStartBack_ + rightH.x * rightAmount,
		p.y + kCineStartHeight_,
		p.z - dirH.z * kCineStartBack_ + rightH.z * rightAmount,
	};
	const Vector3 startLook = c;

	// 終了(cineT_=1)：ロックオン枠（＝敵）へ寄る。真上すぎないよう少し手前に引く。
	const Vector3 endPos = {
		c.x - dirH.x * kCineEndBack_,
		c.y + kCineEndHeight_,
		c.z - dirH.z * kCineEndBack_,
	};
	const Vector3 endLook = c;

	const float s = SmoothStep(cineT_);
	outPos = startPos + (endPos - startPos) * s;
	outLook = startLook + (endLook - startLook) * s;

	// 真上到着後、カメラの「高さだけ」をプレイヤーの肩あたりへ徐々に寄せる。
	// （MGR斬撃モードのように肩口へカメラが来る画。camHeightDrop_ が 0→1 に上がるほど
	//   outPos.y が肩の高さへ近づく。x/z は触らないのでプレイヤーへ水平に寄るのではなく、
	//   あくまで高さだけが肩口まで下りてくる）
	const float shoulderY = p.y + kShoulderHeight_;
	outPos.y += (shoulderY - outPos.y) * camHeightDrop_;
}

void UltimateSkill::SteerCameraFraming(float unscaledDt)
{
	auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera());
	if (!fc || !currentTarget_) return;

	// プレイヤー→対象の水平方向。
	Vector3 to = currentTarget_->GetTargetCenter() - owner_->GetTransform().translate;
	to.y = 0.0f;
	if (Length(to) < 1e-4f) return;
	const Vector3 d = Normalize(to);

	// FollowCamera はプレイヤーから (sinθ,cosθ)*距離 にカメラを置き、プレイヤーを注視する。
	// 対象をプレイヤーの向こう側（画面の前方）へ収めるには、カメラを対象と反対側へ回す。
	// さらに kFrameYawOffset_ ぶん振って、線合わせの余地を残す（真正面=枠が中央で簡単すぎるのを防ぐ）。
	const float desired = std::atan2(-d.x, -d.z) + kFrameYawOffset_;

	// 現在角から最短回りで desired へ指数補間（unscaled なのでスローでも実時間で寄る）。
	const float cur = fc->GetAngle();
	const float diff = WrapPi(desired - cur);
	const float k = 1.0f - std::exp(-kFrameSteerSharp_ * unscaledDt);
	fc->SetAngle(cur + diff * k);
}

void UltimateSkill::ApproachTarget(float unscaledDt)
{
	if (!currentTarget_) return;

	Transform tf = owner_->GetTransform();
	const Vector3 center = currentTarget_->GetTargetCenter();

	// 水平方向（真上直下だと見下ろしが急すぎるので手前へ standoff だけ残す）。
	Vector3 toH = center - tf.translate;
	toH.y = 0.0f;
	const float horizDist = Length(toH);
	const Vector3 dirH = (horizDist > 1e-4f) ? Normalize(toH) : Vector3{ 0.0f, 0.0f, 1.0f };

	// 目標位置：対象の手前 kApproachStandoff_ かつ kAboveHeight_ ぶん上空。
	// これで移動後も常に対象より上にいて見下ろす形になる。
	const Vector3 landing = {
		center.x - dirH.x * kApproachStandoff_,
		center.y + kAboveHeight_,
		center.z - dirH.z * kApproachStandoff_,
	};

	// そこへ 3D（高さ込み）で滑らかに詰める。
	Vector3 delta = landing - tf.translate;
	const float d = Length(delta);
	arrived_ = (d <= kArriveEps_); // 十分近ければ接近完了（見下ろし演出へ）
	if (d > 1e-4f) {
		const float step = (std::min)(kApproachSpeed_ * unscaledDt, d);
		tf.translate = tf.translate + Normalize(delta) * step;
	}

	// --- 飛び込み中の見せ：前傾90度＋横回転(yaw)を1回転ぶん加える ---
	// 対象が切り替わったら飛び込み開始距離を測り直す（進捗を0からやり直す）。
	if (approachSpinTarget_ != currentTarget_) {
		approachSpinTarget_ = currentTarget_;
		approachStartDist_ = -1.0f;
	}
	if (approachStartDist_ < 0.0f) {
		approachStartDist_ = (std::max)(d, 1e-3f); // この接近の開始距離を記録
	}
	// 飛び込み進捗 0→1（開始位置→到着）。到着までにちょうど1回転させる。
	const float progress = std::clamp(1.0f - d / approachStartDist_, 0.0f, 1.0f);

	// yaw は対象を向く角度を基準にする。
	Vector3 look = center - tf.translate;
	const float faceYaw = std::atan2(look.x, look.z);
	const float leanK = 1.0f - std::exp(-kLeanSharp_ * unscaledDt);

	constexpr float kTwoPi  = 6.28318530718f; // 横回転1回転ぶん
	constexpr float kHalfPi = 1.57079632679f; // 前傾90度

	if (arrived_) {
		// 到着後も前傾姿勢(90度)は維持する。ドリル回転だけ止めて、敵の方へ向き直す。
		tf.rotate.x += (kHalfPi - tf.rotate.x) * leanK;       // 前傾90度を保つ
		tf.rotate.z += (0.0f - tf.rotate.z) * leanK;          // ドリル回転を止める
		tf.rotate.y += WrapPi(faceYaw - tf.rotate.y) * leanK; // 最短回りで敵の方を向く
	} else {
		// 飛び込み中：フィギュアスケーターのスピン軸＝背骨まわりのドリル回転にする。
		// 前傾90度で背骨をワールド前方(+Z)へ寝かせ、yaw を0にして背骨を Z 軸へ揃える。
		// こうすると外掛けの Z 回転がちょうど「背骨軸まわりの回転（ドリル）」になる。
		// （yaw で敵を向けると背骨が Z 軸からズレ、Z 回転が前転/後転に見えてしまう）
		tf.rotate.y = 0.0f;                              // 背骨をワールドZ軸へ揃える
		tf.rotate.x += (kHalfPi - tf.rotate.x) * leanK;  // 前傾90度へなじませる
		tf.rotate.z = (1.0f - progress) * kTwoPi;        // 背骨軸まわりに1回転ドリル
	}

	owner_->SetTransform(tf);
}

void UltimateSkill::Finish()
{
	active_ = false;
	currentTarget_ = nullptr;

	// 演出カメラを解除して通常追従へ戻す。
	if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
		fc->ClearCinematic();
	}
	cineWeight_ = 0.0f;
	cineT_ = 0.0f;
	arrived_ = false;
	slowEngaged_ = false;

	// 上空・見下ろし姿勢から、地上・直立へ戻して通常状態へ返す。
	{
		Transform tf = owner_->GetTransform();
		tf.translate.y = groundY_;
		tf.rotate.x = 0.0f; // 前傾を解除して直立へ
		tf.rotate.z = 0.0f; // ドリルのロールも解除
		owner_->SetTransform(tf);
	}

	// スローを解除して通常時間へ戻す。
	TimeManager::GetInstance()->SetTimeScale(1.0f);

	// 無敵・入力ロック・アニメ優先度を解除して通常状態へ戻す。
	owner_->SetInvincible(false);
	owner_->SetInputLocked(false);
	owner_->EndAttackState();

	// マウス感度を通常へ戻す。
	if (auto* fc = dynamic_cast<FollowCamera*>(owner_->GetCamera())) {
		fc->SetMouseSensitivityScale(1.0f);
	}
}
