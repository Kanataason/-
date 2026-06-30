using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class UiTextMoveManager : MonoBehaviour
{
    // KO演出終了後にゲーム終了状態へ遷移するためのイベント
    public static event Action<BattleStateManager.BattleState> OnDisplayDeadTextedAction;
    [Serializable]
    public class Entry
    {
        public UITextMove.TextState textState;
        public UITextMove text;
    }

    //テキストの処理のステート
    private enum CoroutineState
    {
        Dead,
        TimeOver,
        Round,
        Next
    }

    [SerializeField] PlayerInitializeManager testUIInitialize;
    [SerializeField] TextMeshProUGUI textMeshProUGUI;

    //リザルトパネル
    [SerializeField] GameObject KoPanel;

    //時間パネル
    [SerializeField] GameObject TimePanel;

    //前フレームの値を保存
    private int _lastTime = 0;
    private bool isPlayDeadText = false;

    private UITextMove currentTextMove;

    [SerializeField] private List<Entry> entryList;

    //テキストを保存するリスト
    private Dictionary<UITextMove.TextState, UITextMove> textList = new();

    //テキストの種類に合ったIEnumeratorを保存するリスト
    private Dictionary<CoroutineState, Func<UITextMove, IEnumerator>> corutineList = new();

    private void OnDisable()
    {
        UnSubscribeEvents();
    }
    private void Start()
    {
        Init();
    }
    private void Init()
    {
        //リスト初期化
        InitList();

        SubscribeEvents();

        //テキストに出す文字をバッファーに保存
        SetStringBuffer();

        //ラウンドテキストを表示
        RoundTextDisplayText(CoroutineState.Round);
    }
    private void InitList()
    {
        foreach (var text in entryList)
        {
            textList[text.textState] = text.text;
        }
        //リストに追加
        corutineList.Add(CoroutineState.Round, PlayTextAnimation);
        corutineList.Add(CoroutineState.Dead, PlayDeadTextAnimation);
        corutineList.Add(CoroutineState.Next, PlayTextAnimation);
    }
    public void SetStringBuffer()
    {
        //GCを少なくするためにバッファーにする

        //テキストを取得
        var textmove = GetTextMove(UITextMove.TextState.GameStartTextStart);

        var sb = new System.Text.StringBuilder();

        sb.Append("Round");
        sb.Append(BattleStateManager.Instance.CurrentRound);
        textmove.text.text = sb.ToString();

        textMeshProUGUI.SetText("{0}", (int)BattleStateManager.Instance.RemainingTime);
    }

    //バトルマネージャーの状態通知を登録
    private void SubscribeEvents()
    {
        if (BattleStateManager.Instance == null) return;

        BattleStateManager.Instance.OnDisplayWinTextAction += OnDisplayPlayerWinText;
        BattleStateManager.Instance.OnGameEndAction += OnDisplayDeadText;
        BattleStateManager.Instance.OnTimeOutGameAction += OnDisplayTimeOutText;
        BattleStateManager.Instance.OnNextRoundAction += OnDisplayNextRountText;
    }
    //バトルマネージャーの状態通知を解除
    private void UnSubscribeEvents()
    {
        if (BattleStateManager.Instance == null) return;

        BattleStateManager.Instance.OnDisplayWinTextAction -= OnDisplayPlayerWinText;
        BattleStateManager.Instance.OnGameEndAction -= OnDisplayDeadText;
        BattleStateManager.Instance.OnTimeOutGameAction -= OnDisplayTimeOutText;
        BattleStateManager.Instance.OnNextRoundAction -= OnDisplayNextRountText;
    }

    /// <summary>
    /// 順番にテキストを表示しては消す処理
    /// </summary>
    /// <param name="currentText"></param>
    /// <param name="nextState"></param>
    /// <returns></returns>
    private IEnumerator DisplayTextSequence(CoroutineState state, params UITextMove[] texts)
    {

        //存在してなかったら返す
        if (!corutineList.TryGetValue(state, out var coroutine))
            yield break;
        
        //引数で渡された数だけ回す
        foreach (var text in texts)
        {
            yield return StartCoroutine(coroutine(text));
        }

        CheckEvent(state);

        //透明にする
        foreach (var text in texts)
        {
            text.InitTextColor();
        }
    }
    private void CheckEvent(CoroutineState state)//処理するイベントの種類を確認
    {
        switch (state)
        {
            case CoroutineState.Round:
                //イベントにする
                testUIInitialize.StartSet();
                BattleStateManager.Instance.ToPlay();
                break;

            case CoroutineState.Dead:
                // KO演出終了通知
                OnDisplayDeadTextedAction?.Invoke(
                    BattleStateManager.BattleState.GameEnd);
                break;
            case CoroutineState.Next:
                BattleStateManager.Instance.ToPlay();
                PlayerInitializeManager.Instance.EnablePlayerInputList();
                break;
            default:break;
        }
    }
    /// <summary>
    /// テキストを表示させてから消す処理
    /// </summary>
    /// <param name="currentText"></param>
    /// <returns></returns>
    private IEnumerator PlayTextAnimation(UITextMove currentText)
    {
        //表示処理
        yield return StartCoroutine(currentText.FadeText());

        Debug.Log("表示処理終わり");
        //消す処理
        yield return StartCoroutine(currentText.StartFade());

        Debug.Log("消す処理終わり");

    }
    private IEnumerator PlayDeadTextAnimation(UITextMove currentText)
    {
        IEnumerator animation = currentText.AnimationStartDeadText();

        //もし対象のテキストならIEnumeratorを変える
        if (currentText.GetState() is (UITextMove.TextState.GameOverEnd or UITextMove.TextState.TimeOverEnd))
        {
            animation = currentText.AnimationEndDeadText();
        }

        //再生
        yield return StartCoroutine(animation);
    }
    private void Update()
    {
        //残り時間をUiに反映
        if (textMeshProUGUI != null && BattleStateManager.Instance != null)
        {
            int time = (int)BattleStateManager.Instance.RemainingTime;
            if (time != _lastTime) // 値が変わったときだけ反映
            {
                textMeshProUGUI.SetText("{0}", time);
                _lastTime = time;
            }
        }
    }
    //ラウンド開始時にボイスをラウンドによって流す
    public void CheckNowRound()
    {
        var AudioManager = UIAudioSound.Instance;
        //現在のラウンド数
        Debug.Log("ラウンドボイス");
        var Round = BattleStateManager.Instance.CurrentRound;
        switch(Round)
        {
            case 1:AudioManager.PlayVoice(UIAudioSound.VoiceState.Round1);break;
            case 2: AudioManager.PlayVoice(UIAudioSound.VoiceState.Round2); break;
            case 3: AudioManager.PlayVoice(UIAudioSound.VoiceState.FinalRound); break;
        }

    }

    //UiTextMoveを取得
    private UITextMove GetTextMove(UITextMove.TextState state)
    {
        if (textList.TryGetValue(state, out var uimove))
        {
            return uimove;
        }
        return null;
    }
    private void OnDisplayDeadText()//プレイヤーが死んだら呼ばれる
    {
        //再生中なら流さない
        if (isPlayDeadText) return;
        isPlayDeadText = true;
        //ボイス再生
        UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.KnockOut);

        //テキスト取得
        currentTextMove = GetTextMove(UITextMove.TextState.GameOverStart);
        var deadTextEnd = GetTextMove(UITextMove.TextState.GameOverEnd);

        //再生
        StartCoroutine(DisplayTextSequence(CoroutineState.Dead, currentTextMove,deadTextEnd));


    }
    private void OnDisplayPlayerWinText(int Id)//誰かが勝ったら呼ばれる
    {
        //テキスト取得
        currentTextMove = GetTextMove(UITextMove.TextState.Win);

        //指定した時間待つ
        float waitTime = 1;
        Delay.WaitTime(this, waitTime, () =>
        {
            //BGM再生
            UIAudioSound.Instance.PlayBGM(UIAudioSound.BGMState.Win);

            if (Id != 3)
                UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.Win);

            //再生
            currentTextMove.WinText(Id);
        });
    }
    private void OnDisplayTimeOutText()
    {
        //再生中なら流さない
        if (isPlayDeadText) return;

        //テキスト取得
        currentTextMove = GetTextMove(UITextMove.TextState.TimeOverStart);
        var timeoutTextEnd = GetTextMove(UITextMove.TextState.TimeOverEnd);

        //再生
        StartCoroutine(DisplayTextSequence(CoroutineState.Dead, currentTextMove,timeoutTextEnd ));
    }
    private void RoundTextDisplayText(CoroutineState state)//ラウンドテキスト表示処理
    {
        SetStringBuffer();

        //テキストを取得
        currentTextMove = GetTextMove(UITextMove.TextState.GameStartTextStart);
        var nextTextMove = GetTextMove(UITextMove.TextState.GameStartTextEnd);

        //一秒待機して　開始テキストを出す
        float waitTime = 1.2f;
        Delay.WaitTime(this, waitTime,
            () =>
            {
                StartCoroutine(DisplayTextSequence(state, currentTextMove, nextTextMove));
            });
    }


    //次のラウンド遷移時に呼ばれる
    private void OnDisplayNextRountText()
    {
        isPlayDeadText = false;
        RoundTextDisplayText(CoroutineState.Next);
    }


}
