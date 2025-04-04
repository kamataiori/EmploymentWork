#include <MyGame.h>

//Windowsアプリのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	
	//リークチェッカー
	D3DResourceLeakChecker leakCheck;

	Framework* game = new MyGame();

	game->Run();

	delete game;

	return 0;
}