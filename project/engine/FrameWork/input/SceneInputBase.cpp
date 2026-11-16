#include "SceneInputBase.h"

float SceneInputBase::ReadKeyAxis(BYTE negativeKey, BYTE positiveKey) const
{
	if (!input_) {
		return kAxisNeutral_;
	}

	float axis = kAxisNeutral_;
	if (input_->PushKey(negativeKey)) axis -= kKeyAxisAmount_;
	if (input_->PushKey(positiveKey)) axis += kKeyAxisAmount_;
	return axis;
}

float SceneInputBase::ReadAxisX(BYTE negativeKey, BYTE positiveKey) const
{
	// キーボード (デジタル) とスティック (アナログ) を足して値域に収める。
	// 両方同時に入っている場合は倒し切り扱いになる。
	return ClampAxis(ReadKeyAxis(negativeKey, positiveKey) + ReadLeftStickX());
}

float SceneInputBase::ReadAxisY(BYTE negativeKey, BYTE positiveKey) const
{
	return ClampAxis(ReadKeyAxis(negativeKey, positiveKey) + ReadLeftStickY());
}

bool SceneInputBase::PushAny(BYTE key, PadButton button) const
{
	if (!input_) {
		return false;
	}
	return input_->PushKey(key) || input_->PushButton(button);
}

bool SceneInputBase::TriggerAny(BYTE key, PadButton button) const
{
	if (!input_) {
		return false;
	}
	return input_->TriggerKey(key) || input_->TriggerButton(button);
}

float SceneInputBase::ClampAxis(float value)
{
	if (value < kAxisMin_) return kAxisMin_;
	if (value > kAxisMax_) return kAxisMax_;
	return value;
}

float SceneInputBase::ReadLeftStickX() const
{
	// Input 側でデッドゾーン処理済み。未接続なら 0 が返る。
	return input_ ? input_->GetLeftStickX() : kAxisNeutral_;
}

float SceneInputBase::ReadLeftStickY() const
{
	return input_ ? input_->GetLeftStickY() : kAxisNeutral_;
}
