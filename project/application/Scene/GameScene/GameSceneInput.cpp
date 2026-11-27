#include "GameSceneInput.h"
#include "WinApp.h"   // 照準の可動範囲（画面サイズ）

namespace {

	//======================================================
	// 割り当て表（ゲームプレイ）
	//   ここがキー/マウス/パッド割り当ての唯一の置き場所。
	//======================================================

	// 移動：WASD ＋ 左スティック。
	// 十字キーは FollowCamera の旋回に割り当てられているので移動には入れない。
	constexpr AxisBinding kMoveXBinding{
		.negativeKey = DIK_A,
		.positiveKey = DIK_D,
		.stick = StickAxis::LeftX,
	};

	constexpr AxisBinding kMoveYBinding{
		.negativeKey = DIK_S,
		.positiveKey = DIK_W,
		.stick = StickAxis::LeftY,
	};

	// ジャンプ：SPACE / パッドA
	constexpr ButtonBinding kJumpBinding{
		.key = DIK_SPACE,
		.padButton = PadButton::A,
	};

	// ダッシュ（ブリンク）：右クリック / RT
	constexpr ButtonBinding kDashBinding{
		.mouseButton = MouseButton::Right,
		.padTrigger = PadTrigger::Right,
	};

	// 通常攻撃：左クリック / パッドX
	constexpr ButtonBinding kAttackBinding{
		.padButton = PadButton::X,
		.mouseButton = MouseButton::Left,
	};

	// スキル1（回転斬り）：E / RB
	constexpr ButtonBinding kSkillBinding{
		.key = DIK_E,
		.padButton = PadButton::RB,
	};

	// スキル2（突進乱舞）：V / パッドY
	constexpr ButtonBinding kSkill2Binding{
		.key = DIK_V,
		.padButton = PadButton::Y,
	};

	// アルティメット（斬奪）：Q / LB
	constexpr ButtonBinding kUltimateBinding{
		.key = DIK_Q,
		.padButton = PadButton::LB,
	};

	//------------------------------------------------------
	// 視点（感度はここが唯一の置き場所。オプション画面を作るならここを差し替える）
	//   マウス : 移動量[px] × 感度
	//   スティック : 倒し量 × 速度(rad/frame)
	//   キーボード : 押している間 一定速度(rad/frame)
	//------------------------------------------------------

	// 周回角（ヨー）：←→キー ／ マウス左右 ／ 右スティックX
	constexpr LookBinding kYawLookBinding{
		.negativeKey = DIK_LEFT,
		.positiveKey = DIK_RIGHT,
		.keySpeed = 0.01f,
		.mouseAxis = MouseAxis::X,
		.mouseSensitivity = 0.0025f,
		.stick = StickAxis::RightX,
		.padSpeed = 0.035f,
	};

	// 見下ろし角（ピッチ）：マウス上下 ／ 右スティックY
	// マウスを下げる＝見下ろす。スティックは上が正なので符号を反転して向きを揃える。
	constexpr LookBinding kPitchLookBinding{
		.mouseAxis = MouseAxis::Y,
		.mouseSensitivity = 0.0025f,
		.stick = StickAxis::RightY,
		.padSpeed = 0.02f,
		.invertStick = true,
	};

	//------------------------------------------------------
	// 照準（アルティメットの斬撃線）
	//   マウスはカーソル座標へ直接合わせ、動いていない間だけ右スティックで動かす。
	//   速度[px/秒]＝画面幅を約1.4秒で端から端まで振れる速さ。
	//------------------------------------------------------

	// 縦線：画面X（右スティックX）
	constexpr float kAimPadSpeed = 900.0f;

	constexpr AimBinding kAimXBinding{
		.mouseAxis = MouseAxis::X,
		.stick = StickAxis::RightX,
		.padSpeed = kAimPadSpeed,
		.max = static_cast<float>(WinApp::kClientWidth),
	};

	// 横線：画面Y（右スティックY。スティックは上が正、画面Yは下が正なので反転）
	constexpr AimBinding kAimYBinding{
		.mouseAxis = MouseAxis::Y,
		.stick = StickAxis::RightY,
		.padSpeed = kAimPadSpeed,
		.invertStick = true,
		.max = static_cast<float>(WinApp::kClientHeight),
	};

	// ポーズ：ESC / パッドStart（三本線）
	constexpr ButtonBinding kPauseBinding{
		.key = DIK_ESCAPE,
		.padButton = PadButton::Start,
	};

	// デバッグ：タイトルへ戻る
	constexpr ButtonBinding kDebugReturnToTitleBinding{
		.key = DIK_T,
	};

} // namespace

GameSceneInput::GameSceneInput(Input* input)
	: SceneInputBase(input)
	, moveX_(kMoveXBinding)
	, moveY_(kMoveYBinding)
	, jump_(kJumpBinding)
	, dash_(kDashBinding)
	, attack_(kAttackBinding)
	, skill_(kSkillBinding)
	, skill2_(kSkill2Binding)
	, ultimate_(kUltimateBinding)
	, yawLook_(kYawLookBinding)
	, pitchLook_(kPitchLookBinding)
	, aimX_(kAimXBinding)
	, aimY_(kAimYBinding)
	, menu_(input)
	, pause_(kPauseBinding)
	, debugReturnToTitle_(kDebugReturnToTitleBinding)
{
}

void GameSceneInput::Update()
{
	moveX_.Update(Reader());
	moveY_.Update(Reader());

	jump_.Update(Reader());
	dash_.Update(Reader());
	attack_.Update(Reader());
	skill_.Update(Reader());
	skill2_.Update(Reader());
	ultimate_.Update(Reader());

	yawLook_.Update(Reader());
	pitchLook_.Update(Reader());

	// ポーズ画面のメニュー操作（UI 要素はこの結果を見るだけ）
	menu_.Update();

	pause_.Update(Reader());
	debugReturnToTitle_.Update(Reader());
}
