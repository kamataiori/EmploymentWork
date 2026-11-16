#include "InputReader.h"

bool InputReader::IsPadConnected() const
{
	return input_ && input_->IsPadConnected();
}

bool InputReader::PushKey(BYTE key) const
{
	if (!input_ || key == kNoKey) {
		return false;
	}
	return input_->PushKey(key);
}

bool InputReader::PushPadButton(PadButton button) const
{
	if (!input_ || button == PadButton::None) {
		return false;
	}
	return input_->PushButton(button);
}

bool InputReader::PushMouseButton(MouseButton button) const
{
	if (!input_ || button == MouseButton::None) {
		return false;
	}
	return input_->PushMouseButton(static_cast<int>(button));
}

bool InputReader::PushPadTrigger(PadTrigger trigger) const
{
	if (!input_) {
		return false;
	}

	switch (trigger) {
	case PadTrigger::Left:  return input_->PushLeftTrigger();
	case PadTrigger::Right: return input_->PushRightTrigger();
	case PadTrigger::None:
	default:                return false;
	}
}

float InputReader::GetStick(StickAxis axis) const
{
	if (!input_) {
		return kNeutral_;
	}

	switch (axis) {
	case StickAxis::LeftX:  return input_->GetLeftStickX();
	case StickAxis::LeftY:  return input_->GetLeftStickY();
	case StickAxis::RightX: return input_->GetRightStickX();
	case StickAxis::RightY: return input_->GetRightStickY();
	case StickAxis::None:
	default:                return kNeutral_;
	}
}

float InputReader::GetMouseDelta(MouseAxis axis) const
{
	if (!input_) {
		return kNeutral_;
	}

	const POINT delta = input_->GetMouseDelta();
	switch (axis) {
	case MouseAxis::X: return static_cast<float>(delta.x);
	case MouseAxis::Y: return static_cast<float>(delta.y);
	case MouseAxis::None:
	default:           return kNeutral_;
	}
}

float InputReader::GetMousePosition(MouseAxis axis) const
{
	if (!input_) {
		return kNeutral_;
	}

	const POINT position = input_->GetMousePosition();
	switch (axis) {
	case MouseAxis::X: return static_cast<float>(position.x);
	case MouseAxis::Y: return static_cast<float>(position.y);
	case MouseAxis::None:
	default:           return kNeutral_;
	}
}

bool InputReader::IsMouseMoved() const
{
	if (!input_) {
		return false;
	}

	const POINT delta = input_->GetMouseDelta();
	return delta.x != 0 || delta.y != 0;
}
