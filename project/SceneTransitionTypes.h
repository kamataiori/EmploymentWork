#pragma once
#include <string>

// 遷移演出の種類
enum class TransitionType {
    None,
    Fade,
    Shutter,
};

// 遷移要求パラメータ
struct TransitionRequest {
    TransitionType type = TransitionType::Fade;
    float fadeOutSec = 0.3f;
    float fadeInSec = 0.3f;
};
