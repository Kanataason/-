using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class UiTextMoveManager : MonoBehaviour
{
    // KO演出終了後にゲーム終了状態へ遷移するためのイベント
    public static event Action<BattleStateManager.BattleState> OnDisplayDeadTextedAction;
    public class ResultText
    {
        public string First;
        public string Second;

        public ResultText(string first, string second)
        {
            First = first;
            Second = second;
        }
    }
    //テキストの処理のステート
    private enum CoroutineState
    {
        Dead,
        TimeOver,
        Round,
        Next,
        Win
    }

    [SerializeField] TextMeshProUGUI timerText;

    //前フレームの値を保存
    private int lastTime = 0;
    private bool isPlayDeadText = false;

    //テキストの種類に合ったIEnumeratorを保存するリスト
    private Dictionary<CoroutineState, Func<UITextMove, IEnumerator>> corutineList = new();

    private Dictionary<CoroutineState, ResultText> resultTexts = new();

    [SerializeField] private GameObject UiPanel;

    [SerializeField] private UITextMove mainText;
    [SerializeField] private UITextMove subText;


    private void OnDisable()
    {
        ResetText();
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
        SetTimerText();

        //ラウンドテキストを表示
        RoundTextDisplayText(CoroutineState.Round);
    }
    private void InitList()
    {
        //リストに追加
        corutineList.Add(CoroutineState.Round, PlayTextAnimation);
        corutineList.Add(CoroutineState.Dead, PlayDeadTextAnimation);
        corutineList.Add(CoroutineState.TimeOver, PlayDeadTextAnimation);
        corutineList.Add(CoroutineState.Next, PlayTextAnimation);

        resultTexts.Add(CoroutineState.Round, new ResultText("Round{0}", "Fight"));
        resultTexts.Add(CoroutineState.Dead, new ResultText("Knock", "Out"));
        resultTexts.Add(CoroutineState.TimeOver, new ResultText("Time", "Over"));
        resultTexts.Add(CoroutineState.Win, new ResultText("Player{0}Win", "Draw Game"));
        resultTexts.Add(CoroutineState.Next, new ResultText("Round{0}", "Fight"));
    }
    public void SetTimerText()
    {
        //GCを少なくするためにバッファーにする
        timerText.SetText("{0}", (int)BattleStateManager.Instance.RemainingTime);
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
    /// <param name="state"></param>
    /// <param name="texts"></param>
    /// <returns></returns>
    private IEnumerator DisplayTextSequence(CoroutineState state, params UITextMove[] texts)
    {

        //存在してなかったら返す
        if (!corutineList.TryGetValue(state, out var coroutine))
            yield break;
        
        //引数で渡された数だけ回す
        foreach (var text in texts)
        {
            SetActiveText(text,true);
            //ここでテキストをセット
            UpdateText(text, state, text.GetTextPosition());

            //処理
            yield return StartCoroutine(coroutine(text));

            //ここでオブジェクトの非、表示
            if (state != CoroutineState.TimeOver &&  state != CoroutineState.Dead)
            {
                SetActiveText(text);
            }
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
                ActiveUiPanel(true);
                PlayerInitializeManager.Instance.StartSet();
                BattleStateManager.Instance.ToPlay();
                break;

            case CoroutineState.Dead:
                // KO演出終了通知
                float waitTime = 1;
                Delay.WaitTime(this,waitTime,()=> { ActiveUiPanel(); });
                OnDisplayDeadTextedAction?.Invoke(
                    BattleStateManager.BattleState.GameEnd);
                break;
            case CoroutineState.Next:
                ActiveUiPanel(true);
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
        CheckFirstRoundText(currentText);
        //表示処理
        yield return StartCoroutine(currentText.FadeText());

        Debug.Log("表示処理終わり");
        //消す処理
        yield return StartCoroutine(currentText.StartFade());

        Debug.Log("非表示処理終わり");

    }
    //表示テキストの更新
    private void UpdateText(UITextMove currentText,CoroutineState state,UITextMove.TextPosition textPosition)
    {
        if (resultTexts.TryGetValue(state, out var textintfo))
        {
            string text = textPosition is UITextMove.TextPosition.First
                ? textintfo.First : textintfo.Second;

            if (state is CoroutineState.Round or CoroutineState.Next)
            {
                text = string.Format(text, BattleStateManager.Instance.CurrentRound);
            }
            currentText.text.text = text;
        }
    }
    private void ResetText()
    {
        mainText.text.text = "";
        subText.text.text = "";
    }
    private void ActiveUiPanel(bool isActive = false)
    {
        UiPanel.SetActive(isActive);
    }
    private void CheckFirstRoundText(UITextMove currentText)//ラウンドテキストを表示しているか確認
    {
        if(currentText.GetTextPosition() is UITextMove.TextPosition.First)
            UIAudioSound.Instance.PlayRoundVoice(BattleStateManager.Instance.CurrentRound);
    }
    private IEnumerator PlayDeadTextAnimation(UITextMove currentText)
    {
        IEnumerator animation = currentText.AnimationStartDeadText();

        //もし対象のテキストならIEnumeratorを変える
        if (currentText.GetTextPosition() is UITextMove.TextPosition.Second)
        {
            animation = currentText.AnimationEndDeadText();
        }
        //再生
        yield return StartCoroutine(animation);
    }
    private void Update()
    {
        //残り時間をUiに反映
        if (timerText != null && BattleStateManager.Instance != null)
        {
            int time = (int)BattleStateManager.Instance.RemainingTime;
            if (time != lastTime) // 値が変わったときだけ反映
            {
                timerText.SetText("{0}", time);
                lastTime = time;
            }
        }
    }

    private void SetActiveText(UITextMove text,bool isActive = false)
    {
        text.gameObject.SetActive(isActive);
    }
    //ラウンド開始時にボイスをラウンドによって流す
    private ResultText GetTextInfo(CoroutineState state)
    {
        if (resultTexts.TryGetValue(state, out var textClass))
        {
            return textClass;
        }
        return null;

    }
    private void OnDisplayDeadText()//プレイヤーが死んだら呼ばれる
    {
        ResetText();
        //テキスト設定
        SetActiveText(mainText,true);
        SetActiveText(subText, true);
        //再生中なら流さない
        if (isPlayDeadText) return;
        isPlayDeadText = true;
        //ボイス再生
        UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.KnockOut);

        //再生
        StartCoroutine(DisplayTextSequence(CoroutineState.Dead, mainText, subText));


    }
    private void OnDisplayPlayerWinText(int Id)//誰かが勝ったら呼ばれる
    {
        var textClass = GetTextInfo(CoroutineState.Win);

        //テキスト設定
        SetActiveText(mainText, true);
        SetActiveText(subText);
        string text = Id != 3 ? textClass.First : textClass.Second;

        //指定した時間待つ
        float waitTime = 1;
        Delay.WaitTime(this, waitTime, () =>
        {
            //BGM再生
            UIAudioSound.Instance.PlayBGM(UIAudioSound.BGMState.Win);

            if (Id != 3)
            {
                UIAudioSound.Instance.PlayVoice(UIAudioSound.VoiceState.Win);
                text = string.Format(text, Id);
            }

            mainText.text.text = text;
            //再生
            mainText.WinText();
        });
    }
    private void OnDisplayTimeOutText()
    {
        ResetText();
        //テキスト設定
        SetActiveText(mainText, true);
        SetActiveText(subText, true);
        //再生中なら流さない
        if (isPlayDeadText) return;

        //再生
        StartCoroutine(DisplayTextSequence(CoroutineState.TimeOver, mainText,subText));
    }
    private void RoundTextDisplayText(CoroutineState state)//ラウンドテキスト表示処理
    {
        //テキスト設定
        SetTimerText();
        SetActiveText(subText);

        //一秒待機して　開始テキストを出す
        float waitTime = 1.2f;
        Delay.WaitTime(this, waitTime,
            () =>
            {
                StartCoroutine(DisplayTextSequence(state, mainText,subText));
            });
    }


    //次のラウンド遷移時に呼ばれる
    private void OnDisplayNextRountText()
    {
        isPlayDeadText = false;
        ResetText();
        RoundTextDisplayText(CoroutineState.Next);
    }


}
