#include "Title.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include "GameObject.h"
#include "CameraController.h"
#include "UiRenderer.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "StageRoster.h"

void Title::Initialize(ServiceLocator& locator)
{
	_stages = locator.stages;
	BaseScene::Initialize(locator);

	_menuPage.Initialize(locator);
	_stageSelectPage.Initialize(locator);
	_characterSelectPage.Initialize(locator);

	SubscribeEvents();
	OpenPage(_menuPage);

	if (locator.sound)
		locator.sound->PlayBgm("title");

	//W/Sはメニュー選択に使うので、タイトルのカメラは動かさない
	for (const auto& object : _objects)
	{
		if (auto cameraController = object->GetComponent<CameraController>())
			cameraController->SetMoveEnabled(false);
	}
}
void Title::SubscribeEvents()
{
	//メニューから、各画面へ
	_menuPage.OnOpenStageSelect.Subscribe([this]() { OpenPage(_stageSelectPage); });
	_menuPage.OnOpenCharacterSelect.Subscribe([this]() { OpenPage(_characterSelectPage); });

	//ゲームを終了する。メッセージループが終わって、GameManager::Release の後片付けが行われる
	_menuPage.OnQuit.Subscribe([this]() { PostQuitMessage(0); });

	//各画面から、メニューへ戻る
	_stageSelectPage.OnClose.Subscribe([this]() { OpenPage(_menuPage); });
	_characterSelectPage.OnClose.Subscribe([this]() { OpenPage(_menuPage); });

	//ステージを決定したら、選んだステージを覚えて、始める(シーンの切り替えは予約。次のフレームの頭に入れ替わる)
	_stageSelectPage.OnStageChosen.Subscribe([this](int index)
		{
			if (!_stages)
				return;
			_stages->Select(index);
			_sceneManager->ChangeScene(SceneName::Stage);
		});
}

void Title::OpenPage(TitlePage& page)
{
	_currentPage = &page;
	_currentPage->Open();
}

void Title::Update(float deltaTime)
{
	if (_currentPage)
		_currentPage->Update();

	BaseScene::Update(deltaTime);
}

void Title::DrawUi()
{
	_uiRenderer->Begin();

	if (_currentPage)
		_currentPage->Draw();

	_uiRenderer->DrawString(L"ロケットにんげん", 380.0f, 40.0f, 1.0f,
		DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
	_uiRenderer->End();
}
