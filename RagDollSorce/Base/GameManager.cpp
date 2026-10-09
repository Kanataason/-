#include <Windows.h>
#include <string>

#include "GameManager.h"

#include"BaseSystem.h"
#include"Renderer.h"
#include "BaseCamera.h"

#include "Transform.h"
#include "Mesh.h"
#include "ServiceLocator.h"
//60fps固定で物理演算を回すための固定タイムステップ
constexpr float fixedDeltaTime = 1.0f / 60.0f;
void GameManager::Initialize(HWND hwnd)
{
    _input = std::make_unique<InputSystem>(hwnd);

    InitializeSystem(hwnd);
    InitializeFactory();

    //各システムへの参照をまとめてシーン側に渡す
    auto context = CreateServiceLocator();
    _soundManager.Initialize(context); //シーンの初期化で、BGMを鳴らせるように、先に作る
    _debugDraw.Initialize(context);
    _sceneManager.Initialize(context,&_physicsSystem);
    _uiRenderer.Initialize(context.device, context.context,
        L"Asset/Text/UIFont.spritefont", (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

    LoadData();
}
void GameManager::LoadData()
{
    _characterRoster.Load("Asset/Data/Characters.json");
    _stageRoster.Load("Asset/Data/Stages.json");
}

void GameManager::InitializeFactory()
{
    _textureManager.SetDeviceAndContext(_baseSystem.GetDevice(), _baseSystem.GetContext());
    _modelManager.SetDevice(_baseSystem.GetDevice());
    _materialManager.Initialize();
}
void GameManager::InitializeSystem(HWND hwnd)
{
    //描画・入力まわりの基盤システムを初期化
    _baseSystem.Initialize(hwnd);
    _renderer.Initialize(_baseSystem.GetDevice(), _baseSystem.GetContext());

    _timer.Initialize();
}

void GameManager::Update()
{
    _baseSystem.ClearScreen();//画面の初期化

    //実際に経った時間で、スローモーションの戻りを進め、倍率をかけた時間をゲームの更新に使う
    const float realDeltaTime = _timer.Tick();
    _timeScale.Update(realDeltaTime);
    const float deltaTime = realDeltaTime * _timeScale.Get();

    _input->Update();
    _soundManager.Update();

    _sceneManager.Update(deltaTime);

    //固定ステップで物理更新（フレームレート非依存にするため）
    accumulator += deltaTime;
    while (accumulator >= fixedDeltaTime)
    {
        _physicsSystem.FixedUpdate(fixedDeltaTime);
        accumulator -= fixedDeltaTime;
    }

    Draw();

    _baseSystem.Present();//描画
}
void GameManager::Draw()
{
    _sceneManager.Draw(&_renderer);
}

void GameManager::Release()
{
    _baseSystem.Release();

    _sceneManager.Release();

    //音が鳴っている最中に片付けると、終了のときに落ちることがあるので、ここで先に止める
    _soundManager.Shutdown();
}

ServiceLocator GameManager::CreateServiceLocator() 
{
    ServiceLocator locator = { };
    locator.input = _input.get();
    locator.context = _baseSystem.GetContext();
    locator.device = _baseSystem.GetDevice();
    locator.physicsSystem = &_physicsSystem;
    locator.uiRenderer = &_uiRenderer;
    locator.factory = &_gameObjectFactory;
    locator.sceneManager = &_sceneManager;
    locator.timeScale = &_timeScale;
    locator.sound = &_soundManager;
    locator.characters = &_characterRoster;
    locator.stages = &_stageRoster;

    return locator;
}