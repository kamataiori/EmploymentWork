#include "LayerCompositor.h"
#include "DirectXCommon.h"

void LayerCompositor::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;
	opaquePass_.Initialize(dxCommon_);
	alphaPass_.Initialize(dxCommon_);
	additivePass_.Initialize(dxCommon_);
}

void LayerCompositor::Composite(ID3D12GraphicsCommandList* commandList, uint32_t srvIndex, BlendMode mode)
{
	switch (mode) {
	case BlendMode::Opaque:
		opaquePass_.Execute(commandList, srvIndex);
		break;
	case BlendMode::Additive:
		additivePass_.Execute(commandList, srvIndex);
		break;
	case BlendMode::Alpha:
	default:
		alphaPass_.Execute(commandList, srvIndex);
		break;
	}
}
