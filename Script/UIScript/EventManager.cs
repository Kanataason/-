using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;
using static UiUtility;

[RequireComponent(typeof(UiGimickManager))]
[RequireComponent(typeof(SwitchAcitonMap))]
public class EventManager : MonoBehaviour
{
    public static EventManager Instance { get; private set; }

    [Header("Reference")]
     private UiGimickManager gimickManager;
     private SwitchAcitonMap SwichActionmap;

    private PlayerInput currentUiPlayer;//現在のUI操作デバイス

    private Dictionary<UiState, Action> ActionList;//Uiのステートに紐づいたアクションリスト

    /* Setplayer 用*/

    public event Action<StateClass.UiEventType> OnUpdatePanelAction;//現在のUiパネルを更新するためのアクション
    public event Action OnProcessedAction;//デバイスの際ペアリングをする際のアクション
    public event Action OnSelectCancelAction;//プレイヤーがキャンセルボタンを押した時のアクション

    //シーン上に一つしかないことを保証
    private void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
        }
        else
        {
            Destroy(Instance);
        }
    }
    private void OnDestroy()
    {
        Instance = null;
    }
    private void OnDisable()//イベントの解除
    {
        if (InputUserMakeManager.Instance != null)
            InputUserMakeManager.Instance.OnCreateCharacter -= OnSetCahracter;
    }
    private void SubscribeEvents()//イベントの登録
    {
        if (InputUserMakeManager.Instance != null)
            InputUserMakeManager.Instance.OnCreateCharacter += OnSetCahracter;
    }
    private void OnSetCahracter(PlayerInput input)//現在Uiを操作しているUserをイベントでセット
    {
        currentUiPlayer = input;
    }
    private void Start()
    {
        TryGetScriptComponent();
        SubscribeEvents();
        InitList();
    }
    private void InitList()
    {
        //リストの初期化 ステータスを設定、アクションを設定
        ActionList = new Dictionary<UiState, Action>()
        {
            {UiState.GameStart,CancelGameStart},
            {UiState.SetPlayer,CancelSetPlayer},
            {UiState.Setting,CancelSetting},
            {UiState.Manual,CancelManual},
            {UiState.SelectCharactor,CancelSelectChatracter },
            {UiState.Battle, PlayPose},
            {UiState.Pose,CancelPose },
            {UiState.SettingInfo,CancelSettingInfo},
            {UiState.ManualInfo,CancelManualInfo }
        };
    }
    private void TryGetScriptComponent()//コンポーネントを取得
    {
        TryGetComponent<UiGimickManager>(out gimickManager);
        TryGetComponent<SwitchAcitonMap>(out SwichActionmap);
    }
    public void GameStart() => UpdatePanel(StateClass.UiEventType.GameStart);//ゲームメニュー用のパネルを表示
    public void BattleStart() => UpdatePanel(StateClass.UiEventType.BattleStart);//セレクトパネルを非表示にしてバトルシーンに遷移
    public void BattlePoseCancel()//ポーズ中にプレイヤーがキャンセルボタンを押したら流れる
    {
        BattleStateManager.Instance.SetPauseFlagAndInvokeAction(false);

        UpdatePanel(StateClass.UiEventType.BattlePoseExit);//ポーズパネルを消す
    }
    public void BattleResalt() => UpdatePanel(StateClass.UiEventType.Resalt);//リザルト用のパネルに変更
    public void BattlePoseExit()//ポーズ画面からホーム画面に遷移ボタンを押したら流れる
    {
        //プレイヤーの入力をUiに変える
        if (BattleStateManager.Instance != null) BattleStateManager.Instance.OnDestroys();
        gimickManager.OnSetActive(StateClass.UiEventType.BattlePoseSinceMenu);

    }
    public void NextRound()//バトル終了時に継続か終わるかを判断
    {
        BattleStateManager.Instance.NextRound();
        gimickManager.SetNextRoundUiInfo();
        UIAudioSound.Instance.RandomBGM();//ランダムなBGMを流す
       //// プレイヤーの入力をPlayerに変える
       // if (BattleStateManager.Instance != null) BattleStateManager.Instance.OnDestroys();
       // TatuGameManager.Instance.SceneChenge(StageInfo.Battle);
    }
    public void RaiseEvent()
    {
        OnProcessedAction?.Invoke();//Ui操作用のデバイスを生成
    }
    public void CharactorSelect()//ここらの引数すべてにintがたを設定する
    {
        if (BattleStateManager.Instance != null) 
            BattleStateManager.Instance.OnDestroys();//ゴミをなくすためにインスタンスを消す

        PlayerDataManager.Instance.CharacterReset();//キャラクターの情報だけを消す

        UIAudioSound.Instance.PlayBGM(UIAudioSound.BGMState.Select);//セレクト用のBGMを流す

        TatuGameManager.Instance.ChangeState(UiState.SelectCharactor);//ステータスを変える

        TatuGameManager.Instance.ChangeScene(StageInfo.SelectCharacter);//シーンを変更
    }
    public void SetPlayer()//1p,2p選択時に呼ばれる
    {
        UpdatePanel(StateClass.UiEventType.SetPlayerStart);//プレイヤー選択用のパネルに変更
        PlayerDataManager.Instance.ClearPlayerData();//データに保存されている情報をすべて消す
    }

    public void SettingReturn() => EscCancel();//セッティングの際プレイヤーがキャンセルを押したら流れる
    public void MainGameVsStart()//セレクトシーンに遷移時に呼ばれる
    {
        InputUserMakeManager.Instance.RemoveUser();

        InputUserMakeManager.Instance.BreakAndRemoveUser();//リストに入っているプレイヤーを破棄

        TimeLineManager.Instance.PlayTimeLine(TimeLineManager.TimeLineState.MainGame);

        UpdatePanel(StateClass.UiEventType.MainGameStart);//メインゲーム用のパネルを消す

    }
    public void EscCancel()//Escをおしたら呼ばれる
    {
        //現在のUiステータスに紐づいているアクションがあれば取得して実行
        if(!ActionList.TryGetValue(TatuGameManager.Instance.CurrentUiState,out var action))
        {
            return;
        }
        action.Invoke();//実行
    }
    #region
    private void CancelGameStart()
    {
        UpdatePanel(StateClass.UiEventType.GameStartCancel);
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Return);
    }
    private void CancelSetPlayer()//1p,2p選択画面キャンセル時呼ばれる
    {
        PlayerDataManager.Instance.ClearPlayerData();
        InputUserMakeManager.Instance.UiSetUser();
        UpdatePanel(StateClass.UiEventType.GameStart);
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Return);
    }
    private void CancelSetting()
    {
        SwichActionmap.PlayerInputUiModuleDisable(currentUiPlayer);

        UpdatePanel(StateClass.UiEventType.SettingCancel);
        UpdatePanel(StateClass.UiEventType.SettingPanelCancel);
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Return);
    }
    private void CancelManual()
    {
        SwichActionmap.PlayerInputUiModuleDisable(currentUiPlayer);

        UpdatePanel(StateClass.UiEventType.ManualCancel);
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Return);
    }
    private void CancelSelectChatracter()
    {
        OnSelectCancelAction?.Invoke();
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Return);
    }
    private void CancelPose() => UpdatePanel(StateClass.UiEventType.BattlePoseExit);//ポーズパネルを閉じるとき呼ばれる
    private void CancelSettingInfo() => UpdatePanel(StateClass.UiEventType.SettingPanelCancel);//セッティングの各パネルを閉じる際に呼ばれる
    private void CancelManualInfo() => UpdatePanel(StateClass.UiEventType.ManualPanelCancel);//マニュアルの各パネルを閉じる際に流れる
    private void PlayPose()//バトル中にポーズ画面を開く際呼ばれる
    {
        BattleStateManager.Instance.SetPauseFlagAndInvokeAction(true);//時間を止める
        Debug.Log("表示");
        UpdatePanel(StateClass.UiEventType.BattlePose);
    }
    #endregion
    //-------------Menu表示ボタンアクション------//
    public void MenuMove()=> UpdatePanel(StateClass.UiEventType.ManualMove);//マニュアルのMove操作パネルを表示
    public void MenuAttack() => UpdatePanel(StateClass.UiEventType.ManualAttack);//マニュアルのAttack操作パネルを表示
    public void MenuUi() => UpdatePanel(StateClass.UiEventType.ManualUi);//マニュアルのUi操作パネルを表示

    //マニュアル時にプレイヤーがキャンセルを押したら流れる
    public void CancelMenu() => UpdatePanel(StateClass.UiEventType.ManualPanelCancel);//マニュアルのパネルを非表示にする

    //--------------------Setting------------------------
    public void SettingOnClick(int type)//セッティング時に決定ボタンキャンセルボタンを押したら流れる
    {
        //セッティング時のステータスによって処理を変える
            switch (type)
        {
            case 0:gimickManager.OnSetActive(StateClass.UiEventType.SettingDispray);break;
            case 1:gimickManager.OnSetActive(StateClass.UiEventType.SettingVolume);break;
            case 2:gimickManager.OnSetActive(StateClass.UiEventType.SettingController);break;
            case 3:gimickManager.OnSetActive(StateClass.UiEventType.SettingPanelCancel);break;
        }
    }
    public void Staffroll() => TatuGameManager.Instance.ChangeScene(StageInfo.Staffroll);//スタッフロール遷移時に呼ばれる
    public void SetSoloKeyBoard()//ソロキーボードボタンを押したら流れる
    {
        TatuGameManager.Instance.ChangeSoloKeyBoard();//ソロキーボードフラグを変更

        //スプライトをソロキーボードフラグによって変更
        gimickManager.ChangeButtonSprite(StateClass.ButtonState.SettingSoloKeyBoard);
    }
    public void SettingStart()//セッティングボタンを押したら呼ばれる
    {
        PlayerInput input = currentUiPlayer;
        SwichActionmap.PlayerInputUiModuleDisable(input);//Uiを無効にしてプレイヤーの入力を不可にする

        TimeLineManager.Instance.PlayTimeLine(TimeLineManager.TimeLineState.Setting);//セッティング用のtimelineを流す

        UpdatePanel(StateClass.UiEventType.SettingStart);//セッティング用のパネルを出す
    }
    //--------------------

    //----------------Manual-----------------------
    public void ManualStart()//マニュアルボタンを押したら呼ばれる
    {
        PlayerInput input = currentUiPlayer;
        //Uiを無効にしてプレイヤーの入力を不可にする
        if (input != null) SwichActionmap.PlayerInputUiModuleDisable(input);

        UpdatePanel(StateClass.UiEventType.ManualStart);//マニュアル用のパネルを出す
    }
    public void ManualTimeLineYet()//マニュアル用のたいむらいんを再生終えたら呼ばれる
    {
        PlayerInput input = currentUiPlayer;
        //Uiを有効にしてプレイヤーの入力を可能にする
        if (input != null) SwichActionmap.PlayerInputUiModuleEnable(input);
    }
    //--------------------------------
    public void Startfade(float duration, float startAlpha,float endAlpha)
    {
        //引数によってフェードアウトかフェードインかを分ける
        gimickManager.StartFade(duration,startAlpha,endAlpha);
    }
    public void IdleVideoPlayer(int IsStop)//ビデオ再生時、終了時に呼ばれる
    {
        gimickManager.IdleVideoPlay(IsStop);//再生、ストップかを変更
    }
    public void ResetFirstSelectButton()
    {
        //最初に選ぶボタンの初期化
        gimickManager.CheckSelectButton(StateClass.ButtonState.SelectCharacterStart);
    }

    public void UpdatePanel(StateClass.UiEventType state)//GimickManagerにイベントタイプを送って処理する関数
    {
        OnUpdatePanelAction?.Invoke(state);
    }
    public void ExitGame() => TatuGameManager.Instance.ExitGame();//ゲームを終了時に呼ばれる関数

    /// <summary>
    /// Uiの入力を切断
    /// </summary>
    /// <param name="playerinput"></param>
    public void UiModuleDisable(PlayerInput playerinput)
    {
        SwichActionmap.PlayerInputUiModuleDisable(playerinput);//UiModuleのOff
    }
    /// <summary>
    /// Uiの入力を復活
    /// </summary>
    /// <param name="playerinput"></param>
    public void UiModuleEnable(PlayerInput playerinput)
    {
        SwichActionmap.PlayerInputUiModuleEnable(playerinput);//UiModuleのOn
    }
    /// <summary>
    /// Uiの入力を有効にする関数 UnityEventから呼ばれる
    /// </summary>
    public void UiModuleEnableEvent()
    {
        var playerinput = currentUiPlayer;
        SwichActionmap.PlayerInputUiModuleEnable(playerinput);//UiModuleのOn
    }
    /// <summary>
    /// プレイヤーの入力コンポーネントを有効にするか無効にするかを確認して変更する関数
    /// </summary>
    public void EnableOrDisablePlayerInput(PlayerInput input,bool IsEnable = true)
    {
        SwichActionmap.EnableOrDisablePlayerInput(input,IsEnable);
    }
    /// <summary>
    ///プレイヤーの入力だけを有効か無効かを確認してから変換する関数s
    /// </summary>
    public void ChangeActivePlayerInput(PlayerInput input,bool IsActive = false)
    {
        SwichActionmap.ActivePlayerInput(input, IsActive);
    }
}

/// <summary>
/// 指定した時間止めることができる関数
/// </summary>
public static class Delay
{
    /// <summary>
    /// １フレームだけ待つ
    /// </summary>
    /// <param name="owner"></param>
    /// <param name="action"></param>
    public static void OneFrame(MonoBehaviour owner,Action action)
    {
        owner.StartCoroutine(StartOneFrame(action));
    }
    /// <summary>
    /// 指定したフレームの数待ってアクションを実行する
    /// </summary>
    /// <param name="owner"></param>
    /// <param name="frame"></param>
    /// <param name="func"></param>
    /// <param name="cancelAction">
    /// 条件が合ったら実行される処理
    /// </param>
    /// <param name="completeAction">
    /// 指定されたフレーム数流れたら実行される処理
    /// </param>
    public static void WaitFrame(MonoBehaviour owner,int frame,Func<bool> func =null,
        Action cancelAction = null,Action completeAction = null)
    {
        owner.StartCoroutine(StartWaitFrame(frame,func,cancelAction,completeAction));
    }
    /// <summary>
    /// 一定時間待つ 
    /// </summary>

    public static void WaitTime(MonoBehaviour owner,float time,Action action)
    {
        owner.StartCoroutine(StartWaitTime(time,action));
    }

    private static System.Collections.IEnumerator StartWaitTime(float time,Action action)
    {
        yield return new WaitForSeconds(time);

        action?.Invoke();
    }
    private static System.Collections.IEnumerator StartOneFrame(Action action)
    {
        yield return null;

        action?.Invoke();
    }
    private static System.Collections.IEnumerator StartWaitFrame
        (int waitframe,Func<bool> func,Action cancelAction,Action completeAction)
    {
        //現在のフレームを指定したフレームと合わせる
        int targetFrame = Time.frameCount + waitframe;

        //指定された時間待つ
        while (Time.frameCount < targetFrame)
        {
            if (func != null)
            {
                var isCancel = func();
                if (isCancel)
                {
                    cancelAction?.Invoke();
                    yield break;
                }
            }

            yield return null;
        }


        completeAction?.Invoke();
    }
}