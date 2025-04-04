#include "Framework.h"
#include "SceneFactory.h"
#include "ImGuiManager.h"
#include "OffscreenRendering.h"
#include "GlobalVariables.h"

class MyGame : public Framework
{
public:
	//------メンバ関数------//

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

private:

	std::unique_ptr<ImGuiManager> imGuiManager_ = nullptr;

	std::unique_ptr<OffscreenRendering> offscreenRendering = std::make_unique<OffscreenRendering>();

};
