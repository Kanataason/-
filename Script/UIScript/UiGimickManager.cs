using System;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.UI;
using static UiUtility;

public class UiGimickManager : MonoBehaviour
{
    //イベントごとにパネルやボタンを保持する関数
    [Serializable]
    public class EventInfos
    {
        public StateClass.ButtonState BState;
        public StateClass.PanelState PState = StateClass.PanelState.None;
        public UnityEvent<StateClass.ButtonState> Action;

        public StateClass.SpecialEventState Special = StateClass.SpecialEventState.None;
        
        public EventInfos(StateClass.ButtonState bstate, UnityEvent<StateClass.ButtonState> action)
        {
            BState = bstate;
            Action = action;
        }
    }

    //イベントにEventInfosを紐づける
    [Serializable]
    public class ActionEntry
    {
        public StateClass.UiEventType Key;
        public EventInfos Value;
    }

    //Stateに合ったPanelを保持するリスト
    [SerializeField] private ObjList<StateClass.PanelState, GameObject> PanelList;

    //Stateに合ったButtonを保持するリスト  最初のセレクト状態に使う
    [SerializeField] private ObjList<StateClass.ButtonState, Button> ButtonList;

    //Stateに合ったScrollBarを保持するリスト 最初のセレクト状態に使う
    [SerializeField] private ObjList<StateClass.SliderState, Scrollbar> SliderList;

    // テキストを保持するリスト
    [SerializeField] private ObjList<StateClass.TextState, TextMeshProUGUI> TextList;

    // UI状態ごとの初期選択ボタン
    private Dictionary<UiState, StateClass.ButtonState> FirstButtonList;

    //Stateに合った関数をインスペクターで生成
    private Dictionary<StateClass.UiEventType, EventInfos> ActionList;

    [Header("Raw")]//素材
    public Image fadeImage;
    [Header("Refarence")]//参照
    private Camera _cameras;

    //内部的に値を保持するための変数
    private GameObject _currentPanel;
    private Button _oldButton;

    [SerializeField] private StateClass.TextState _firstTextState = StateClass.TextState.CurrentController;
   [SerializeField] List<ActionEntry> _actionList;


    //過去３個までのボタンを保持してキャンセル時にセレクトした状態にする
    private Stack<Button> _prevButtons = new(3);
    private void Start()
    {
        Initialize();
        SetFirstButton();
    }
    private void Initialize()//初期化
    {

        InitList();

        SubscribeEvents();

        //パネルやテキストの最初に表示させるものをセット
        SetDisplayText(_firstTextState);
        SetFirstPanel();
        
        _cameras = Camera.main;
    }
    private void SetFirstPanel()//最初のパネルをセットする
    {
        switch (TatuGameManager.Instance.CurrentUiState)
        {
            case UiState.Home:
                _currentPanel = PanelList.Get(StateClass.PanelState.GameMenu);break;
            case UiState.SelectCharactor:
                _currentPanel = PanelList.Get(StateClass.PanelState.SelectCharacter);break;
            case UiState.Battle:
                _currentPanel = PanelList.Get(StateClass.PanelState.BattlePose);break;
            default:break;
        }
    }
    private void SubscribeEvents()
    {
        Debug.Log(EventManager.Instance == null);
        EventManager.Instance.OnUpdatePanelAction += OnSetActive;
    }
    private void OnDisable()
    {
        EventManager.Instance.OnUpdatePanelAction -= OnSetActive;
    }
    private void InitList()//ここで最初にフォーカスするボタンを初期化
    {
        ButtonList.Init();
        PanelList.Init();
        SliderList.Init();
        TextList.Init();

        //リストをディクショナリーに変換
        ActionList = _actionList.ToDictionary((x) => x.Key, (x) => x.Value);
        FirstButtonList = new Dictionary<UiState, StateClass.ButtonState>()
        {
            {UiState.Home,StateClass.ButtonState.FirstGameStart},
            {UiState.SelectCharactor,StateClass.ButtonState.SelectCharacterStart}
        };
    }
    /// <summary>
    /// ボタンがあるかを確認
    /// </summary>
    /// <param name="buttonstate"></param>
    public void CheckSelectButton(StateClass.ButtonState buttonstate)
    {
        var button = ButtonList.Get(buttonstate);
        Debug.Log(button);
        if (button == null) return;

        SelectedButton(button);
    }
    /// <summary>
    /// 引数で渡されたボタンをセレクト状態にする
    /// </summary>
    /// <param name="button"></param>
    public void SelectedButton(Button button)//ぼたんの選択
    {
        if (button == null)
        {
            Debug.Log($"{button}is null");return;
        }
        EventSystem.current.SetSelectedGameObject(button.gameObject);
        UiInputManager.CurrentSelectedObject = button.gameObject;
        button.Select();

    }
    /// <summary>
    /// 現在選択されているボタンを取得
    /// </summary>
    /// <returns></returns>
    private Button GetCurrentSelectButton()
    {
        GameObject current = EventSystem.current.currentSelectedGameObject;
        if (current != null)
        {
            return current.GetComponent<Button>();
        }
        return null;
    }
    /// <summary>
    /// 引数で渡されたStateに紐づいているActionがないかlistから捜して実行
    /// </summary>
    /// <param name="state"></param>
    public void OnSetActive(StateClass.UiEventType state)
    {
        //Uiの切り替え、選択ボタンの設定、画面のstate変更
        if (!ActionList.TryGetValue(state, out var action))
        {
            return;
        }
        if (action == null) return;

        //特別のStateかどうかを確認
        var specialAction = GetSpecialAction(action);
        if (specialAction != null)
        {
            specialAction.Invoke(action.PState, action.BState);
            return;
        }

        //実行
        action.Action?.Invoke(action.BState);
    }
    /// <summary>
    /// キャラクターセレクト時にキャラクターを非表示にする
    /// </summary>
    public void OFF3DModel()
    {
        PanelList.Get(StateClass.PanelState.CharacterInfoPlayer1).SetActive(false);
        PanelList.Get(StateClass.PanelState.CharacterInfoPlayer2).SetActive(false);
    }
    /// <summary>
    /// 最初のボタンがリストにあるかどうかを確認
    /// </summary>
    private void SetFirstButton()
    {
        TatuGameManager manager = TatuGameManager.Instance;
        if(!FirstButtonList.TryGetValue(manager.CurrentUiState,out var button))
        {
            return;
        }
        CheckSelectButton(button);
    }
    /// <summary>
    /// テキストに表示させる
    /// </summary>
    /// <param name="state"></param>
    private void SetDisplayText(StateClass.TextState state)
    {
        var text = TextList.Get(state);
        if (text == null) return;

        //現在接続されているコントローラーを確認andソロキーボードモードか確認
        bool isVisible;
        isVisible = TatuGameManager.Instance.IsSoloKeyboardMode;
        bool multiPad = Gamepad.all.Count > 1;
        bool noPad = Gamepad.all.Count == 0;

        string baseText;

        if (multiPad)
            baseText = "現在ゲームパッド操作です";
        else if (noPad)
            baseText = "現在キーボード操作です";
        else
            baseText = isVisible ? "現在キーボード操作です" : "現在ゲームパッド操作です";

        text.text = baseText;
    }
    /// <summary>
    /// 特別なアクションがあるかを確認
    /// </summary>
    /// <param name="eventInfos"></param>
    /// <returns></returns>
    private Action<StateClass.PanelState,StateClass.ButtonState> GetSpecialAction(EventInfos eventInfos)
    {
        switch(eventInfos.Special)
        {
            case StateClass.SpecialEventState.Setting: return SetSettingInfo;
            case StateClass.SpecialEventState.Manual:return SetManualInfo;
            case StateClass.SpecialEventState.Battle:break;//個々をどうするか
            default:break;
        }
        return null;
    }
    //ここからはイベントに設定されているアクション
    #region

    /// 共通処理  ActivePanel　１パネルを表示させたり非常時にしたり
    /// 　　　　   SetButton   ２ボタンを選んだ状態にする
    /// 　　　　  OtherProcess ３音をならす、Uiステータスを変更
    /// 　　　　 　４イベントを発火させたり
    /// 

    public void SetStartGame(StateClass.ButtonState state)//スタート時に呼ばれる
    {
        ChangePanel(StateClass.PanelState.SelectGameMenu);
        SetButton(state);

        ApplyUiState(UiState.GameStart);
        UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.GameStart);
    }
    public void CancelStartGame(StateClass.ButtonState state)//スタートをキャンセル時に呼ばれる
    {
        ChangePanel(StateClass.PanelState.GameMenu);
        SelectedButton(_prevButtons.Pop());

        ApplyUiState(UiState.Home);
    }
    public void SetStartPlayer(StateClass.ButtonState state)//順番を決める際に呼ばれる
    {
        ChangePanel(StateClass.PanelState.PlayerSet);
        SetButton(state);

        ApplyUiState(UiState.SetPlayer);
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Click);
        EventManager.Instance.RaiseEvent();
    }
    public void SetStartSetting(StateClass.ButtonState state)//セッティング時に呼ばれる
    {
        _currentPanel.SetActive(false);
        PanelList.Get(StateClass.PanelState.MainSetting).SetActive(true);
        SetButton(state,true);

        ApplyUiState(UiState.Setting);
        UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.Setting);
    }
    public void CancelSetting(StateClass.ButtonState state = default)//セッティングをキャンセル時に呼ばれる
    {
        ApplyUiState(UiState.Home, TimeLineManager.TimeLineState.Reverse);
    }

    //セッティングの情報を出す時に呼ばれる
    public void SetSettingInfo(StateClass.PanelState Pstate, StateClass.ButtonState Bstate)
    {
        _prevButtons.Push(GetCurrentSelectButton());
        CheckSelectButton(Bstate);

        ChangePanel(Pstate);

        ApplyUiState(UiState.SettingInfo);
    }

    public void CancelSetteingPanel(StateClass.ButtonState state)//セッティングをキャンセル時に呼ばれる
    {
        _currentPanel.SetActive(false);
        PanelList.Get(StateClass.PanelState.MainSetting).SetActive(true);
        SelectedButton(_prevButtons.Pop());

        ApplyUiState(UiState.Setting);
    }
    public void CancelManualPanel(StateClass.ButtonState state)//マニュアルをキャンセル時に呼ばれる
    {
        _currentPanel.SetActive(false);

        SelectedButton(_prevButtons.Pop());

        ApplyUiState(UiState.Manual);
    }
    public void CancelBattlePose(StateClass.ButtonState state)//バトル中ポーズをキャンセル時に呼ばれる
    {
        _currentPanel.SetActive(false);
        Time.timeScale = 1;
        ApplyUiState(UiState.Battle);
    }

    //マニュアルの情報を出す時に呼ばれる
    public void SetManualInfo(StateClass.PanelState Pstate, StateClass.ButtonState Bstate = default)
    {
        ChangePanel(Pstate);
        _prevButtons.Push(GetCurrentSelectButton());

        GameObject obj = GetSelectSlider(Pstate);
        if (obj == null) return;
        EventSystem.current.SetSelectedGameObject(obj);
        ApplyUiState(UiState.ManualInfo);
    }
    private GameObject GetSelectSlider(StateClass.PanelState Pstate)//どのスライダーを選んだ状態にするかを確認
    {
        GameObject obj = null;
        //引数で渡されたステータスによって選択するスライダーを変える
        switch (Pstate)
        {
            case StateClass.PanelState.MenuMove:
                obj = SliderList.Get(StateClass.SliderState.ManualMove).gameObject; break;
            case StateClass.PanelState.MenuAttack:
                obj = SliderList.Get(StateClass.SliderState.ManualAttack).gameObject; break;
            case StateClass.PanelState.MenuUi:
                obj = SliderList.Get(StateClass.SliderState.ManualUi).gameObject; break;
        }
        return obj;
    }
    public void SetStartManual(StateClass.ButtonState state)//マニュアル時に呼ばれる
    {
        _currentPanel.SetActive(false);
        PanelList.Get(StateClass.PanelState.Manual).SetActive(true);

        SetButton(state,true);

        ApplyUiState(UiState.Manual,TimeLineManager.TimeLineState.ManualStart);
        UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.Manual);

    }
    public void CancelStartManual(StateClass.ButtonState state)//マニュアルをキャンセル時に呼ばれる
    {
        ApplyUiState(UiState.Home,TimeLineManager.TimeLineState.ManualEnd);
    }

    public void SetStartMainGame(StateClass.ButtonState state)//プレイヤーの順番を決めた時に呼ばれる
    {
        _currentPanel.SetActive(false);
        ApplyUiState(UiState.SelectCharactor);

        UIAudioSound.Instance.PlayBGM(UIAudioSound.BGMState.Select);
    }
    public void SetStartBattle(StateClass.ButtonState state)//セレクトシーンで全員選択完了時に呼ばれる
    {
        _currentPanel.SetActive(false);
        ApplyUiState(UiState.Battle, TimeLineManager.TimeLineState.Battle);
    }
    public void SetStartBattlePose(StateClass.ButtonState state)//バトル中ポーズ時に呼ばれる
    {
        ApplyUiState(UiState.Pose);
        _currentPanel.SetActive(true);
        _prevButtons.Push(GetCurrentSelectButton());
        CheckSelectButton(state);
        Time.timeScale = 0;
    }
    public void SetStartBattleExit(StateClass.ButtonState state)//ポーズからホームに遷移時に呼ばれる
    {
        _currentPanel.SetActive(false);
        Time.timeScale = 1;
        ApplyUiState(UiState.Home);

        TatuGameManager.Instance.ChangeScene(StageInfo.GameMenu);
    }
    public void PlayResult(StateClass.ButtonState state)//勝者が決まった時に呼ばれる
    {
        CheckSelectButton(state);
        ChangePanel(StateClass.PanelState.Result);
    }
    public void SetNextRoundUiInfo()//リザルト画面からNextGameを押したら呼ばれる
    {
        _currentPanel.SetActive(false);
        SetFirstPanel();
       // CheckSelectButton(StateClass.ButtonState.FirstBattlePose);
        EventSystem.current.SetSelectedGameObject(null);
        UiInputManager.CurrentSelectedObject = null;
    }
    public void PlayableCancelEvent()//TimelineのUnityEventから呼ばれる
    {
        if(TatuGameManager.Instance.CurrentUiState != UiState.SelectCharactor)
        ChangePanel(StateClass.PanelState.GameMenu);

        SelectedButton(_oldButton);
        _oldButton = null;
        TatuGameManager.Instance.ChangeState(UiState.Home);
    }
    public void ChangeButtonSprite(StateClass.ButtonState state)//ボタンのスプライトをOn Offする際に呼ばれる
    {
        var button = ButtonList.Get(state);
        var sprite = button.GetComponent<Image>();

        bool isVisible;
        isVisible = TatuGameManager.Instance.IsSoloKeyboardMode;
        //引数で渡されたステータスによって処理を書く
        switch (state)
        {
            case StateClass.ButtonState.SettingSoloKeyBoard:
                SetDisplayText(StateClass.TextState.CurrentController);
                break;
        }
        
        Color color = sprite.color;
        //キーボードソロモードかどうかで数値を変える
        color.a = isVisible ? 1f : 0f;
        sprite.color = color;//色を反映
    }
    #endregion
    //----------------------------------------------------------------
    /// <summary>
    /// 引数で渡されたStateのパネルを表示
    /// </summary>
    /// <param name="state"></param>
    private void ChangePanel(StateClass.PanelState state)
    {
        _currentPanel.SetActive(false);
        _currentPanel = PanelList.Get(state);

        if (_currentPanel == null) return;
        _currentPanel.SetActive(true);
    }
    /// <summary>
    /// 引数で渡されたStateのボタンを選んだ状態にする
    /// </summary>
    /// <param name="state"></param>
    /// <param name="IsPlayable"></param>
    private void SetButton(StateClass.ButtonState state,bool IsPlayable = false)
    {
        //過去選んだボタンがあるかを確認
        AddAndCheckPrevList();

        if (IsPlayable)
            _oldButton = GetCurrentSelectButton();

        CheckSelectButton(state);
    }
    /// <summary>
    ///引数に渡されてNullじゃないものだけを実行
    /// </summary>
    /// <param name="Ustate"></param>
    /// <param name="Tstate"></param>
    private void ApplyUiState(UiState Ustate = UiState.None,TimeLineManager.TimeLineState
                                 Tstate = TimeLineManager.TimeLineState.None)
    {
        if (Ustate != UiState.None)
            TatuGameManager.Instance.ChangeState(Ustate);

        if (Tstate != TimeLineManager.TimeLineState.None)
            TimeLineManager.Instance.PlayTimeLine(Tstate);

    }
    /// <summary>
    /// パンくずリストをするため現在選んでいるボタンをStackに入れて管理
    /// </summary>

    private void AddAndCheckPrevList()
    {
        int MaxLength = 3;
        //重複がないかを確認
        if (!_prevButtons.Contains(GetCurrentSelectButton()))
        {
            _prevButtons.Push(GetCurrentSelectButton());
            Debug.Log($"push{GetCurrentSelectButton()},count{_prevButtons.Count}");
        }
        else
        {
            //無ければStackにpop
            _prevButtons.Pop();
        }
        //もし用量が無かったら流す
        if (_prevButtons.Count >= MaxLength)
        {

            var temp = new Stack<Button>(_prevButtons);

            _prevButtons.Clear();

            bool IsSkicp = false;
            //一番最初に要れたボタンを消す
            foreach (var button in temp)
            {
                if (!IsSkicp && temp.Count == _prevButtons.Count + 1)
                {
                    IsSkicp = true;
                    continue;
                }
                _prevButtons.Push(button);
            }
        }
    }

    //----------------------------------------------------------------


    /// <summary>
    /// 外部からfadeinを開始させるための関数
    /// </summary>
    /// <param name="duration"></param>
    public void StartFade(float duration,float startAlpha, float endAlpha)
    {
        DisplayEfect.Fade(this, fadeImage, startAlpha,endAlpha,duration);
    }
    /// <summary>
    /// ホーム画面時、MVを流すときに呼ばれる
    /// </summary>
    /// <param name="IsPlay"></param>
    public void IdleVideoPlay(int IsPlay)//１が再生 2が停止
    {
        float startAlpha = 0;
        float endAlpha = 1;
        if (IsPlay == 1)
        {
            UIAudioSound.Instance.BGMSource.Stop();
            DisplayEfect.Fade(this,fadeImage, startAlpha, endAlpha);

        }
        else
        {
            UIAudioSound.Instance.PlayBGM(UIAudioSound.BGMState.GameMenu);
            DisplayEfect.Fade(this, fadeImage, startAlpha, endAlpha);
        }
    }
    public void CheckScene()//シーンを遷移するための関数
    {
        //バトル中出なければ流す
        if (TatuGameManager.Instance.CurrentUiState != UiState.Battle) TatuGameManager.Instance.ChangeScene(StageInfo.SelectCharacter);
        else TatuGameManager.Instance.ChangeScene(StageInfo.Battle);
    }

    //---------------------ここからシーン切り替え時のアニメーション処理---------------//
    public void ZoomingStart()
    {
        DisplayEfect.ZoomIn(this, _cameras);
    }

}

/// <summary>
/// ジェネリック型にしてパネル、テキスト、ボタンのリストやディクショナリーを宣言しなくてよくなる
/// </summary>

[Serializable]
public class ObjList<T, T1> where T : Enum where T1 : UnityEngine.Object
{
    [Serializable]
    public class Entry
    {
        public T Key;
        public T1 Value;
    }

    [SerializeField] private System.Collections.Generic.List<Entry> List;

    private System.Collections.Generic.Dictionary<T, T1> ObjLists = new();

    public void Init()
    {
        if (List == null)
        {
            Debug.LogError("List is null");
            return;
        }

        foreach (var list in List)
        {
            if (list.Value == null) continue;
            ObjLists[list.Key] = list.Value;
        }
    }

    public T1 Get(T state)
    {
        if (ObjLists.TryGetValue(state, out var value))
        {
            return value;
        }
        return null;
    }
    public T1[] Gets(T[] states)
    {
        T1[] objs = new T1[states.Length + 1];
        for (int i = 0; i < states.Length; i++)
        {
            if (!ObjLists.TryGetValue(states[i], out var value)) continue;

            objs[i] = value;

        }
        return objs;
    }
}
