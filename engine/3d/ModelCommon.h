#pragma once
#include "DirectXCommon.h"

class ModelCommon
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

public:
	//------Getter------//
	//DirectXCommonGetter
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:

	//DirectXCommonの初期化
	DirectXCommon* dxCommon_;

};

