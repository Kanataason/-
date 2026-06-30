using System;
using UnityEngine;
using State = StateMachine<BattleStateManager>.State;

public class BattleStateManager : MonoBehaviour
{
    /// <summary>
    /// バトルの進行を管理するスクリプト
    /// </summary>
    private StateMachine<BattleStateManager> stateMachine;
    public static BattleStateManager Instance { get; private set; }
    //バトルの状態
    public enum BattleState : int {GameStating,GameEnd,GamePlaying }

    [Header("バトル情報")]
    public int TotalRound = 1;//すべてのラウンド
    public  int CurrentRound = 1;//現在のラウンド数
    public float TimeLimit = 99;//最大バトル時間
    public float RemainingTime =0;//現在のバトル時間
    public int Player1Wins = 0;//プレイヤー１の勝利数
    public int Player2Wins = 0;//プレイヤー2の勝利数
    public bool IsGameEnd = false;//ゲーム終了フラグ
    public bool IsPause = false;//ゲームが止まっている時
    public BattleState CurrentBattleState = BattleState.GameStating;

    //処理通知アクション
    public event Action<int,int> OnChangeWinUiAction;//現在の勝利数をUiに反映させるためのアクション
    public event Action OnUnPairUserAction;//接続されているデバイスの解除アクション
    public event Action<int> OnDisplayWinTextAction;//ディスプレイにテキストを表示させるアクション
    public event Action OnGameEndAction;//ラウンド終了アクション
    public event Action OnTimeOutGameAction;//タイムアップされたら呼ばれるアクション
    public event Action OnNextRoundAction;//次のラウンド遷移時のアクション

    public static event Action OnStopAnimation;//すべてのプレイヤーのアニメーションを止める
    public static event Action OnStartAnimation;//すべてのプレイヤーのアニメーションを再生する

    //このスクリプトが一つしかないことを圃場
    private void Awake()
    {
        if(Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
        }
    }//ここはラウンドごとにシーンを変えなかったらできる

    private void ResetBattleInfo()//ラウンドごとに初期化
    {
        Player1Wins = 0;
        Player2Wins = 0;
        CurrentRound = 1;
        IsGameEnd = false;
    }
    private void Init()
    {
        RemainingTime = TimeLimit;

        //バトルの情報をリセット
        if (IsGameEnd)
        ResetBattleInfo();

        //WinUi更新
        InitDisplayWinUiImage();
    }
    public void InitDisplayWinUiImage()
    {
        //現在の勝利数をUiに反映
        OnChangeWinUiAction?.Invoke(Player1Wins, Player2Wins);
    }
    void Start()
    {
        InitTransition();//バトルの状態をセット
        SubscribeEvents();
    }
    private void SubscribeEvents()//イベントを登録
    {
       UiTextMoveManager.OnDisplayDeadTextedAction += ChangeTranstion;
    }
    private void UnSubscribeEvents()//イベントの解除
    {
       UiTextMoveManager.OnDisplayDeadTextedAction -= ChangeTranstion;
    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }

    private void InitTransition()//遷移のペアを登録
    {
        stateMachine = new StateMachine<BattleStateManager>(this);

        stateMachine.AddTransition<MatchStart, MatchPlaying>((int)BattleState.GamePlaying);
        stateMachine.AddTransition<MatchEnd, MatchStart>((int)BattleState.GameStating);
        stateMachine.AddTransition<MatchPlaying, MatchEnd>((int)BattleState.GameEnd);
        stateMachine.Start<MatchStart>();
    }

    void Update()
    {
        stateMachine?.Updata();
    }
    //試合開始のステータスに変更
    public void ToPlay()
    {
        stateMachine.Dispatch((int)BattleState.GamePlaying);
    }
    public void ToEnd(int Id) //試合終了のステータスに変更
    {

        (int p1, int p2) = Id switch
        {
            1 => (1, 0),
            2 => (0, 1),
            3 => (1, 1),
            _ => (0, 0)
        };

        Player1Wins += p1;
        Player2Wins += p2;

    }
    private void ChangeTranstion(BattleState state)//バトルの処理状態を変える
    {
        stateMachine?.Dispatch((int)state);
    }
    public void ChangeBattleState(BattleState state)//バトルの状態を変える
    {
        CurrentBattleState = state;
    }
    //ポーズフラグの取得、セット
    public void SetPauseFlagAndInvokeAction(bool isPause = false)
    {
        var action = isPause is true ? OnStopAnimation : OnStartAnimation;

        action?.Invoke();

        IsPause = isPause;
    }
    public bool GetPauseFlag() => IsPause;
    /// <summary>
    /// ラウンド終了時少しヒットストップをさせる
    /// </summary>
    public void GameEnd()
    {
        OnGameEndAction?.Invoke();
    }
    public void OnDestroys()
    {
        Instance = null;      // シングルトン解除
        stateMachine = null;  // ステートマシン参照解除

        Destroy(gameObject);
    }
    public void NextRound()
    {
        ////次のラウンド処理
        OnNextRoundAction?.Invoke();

        ChangeTranstion((int)BattleState.GameStating);
    }
    //public void CharactorSelect() { TatuGameManager.Instance.ChangeScene(StageInfo.SelectCharacter); }
    private class MatchStart : State
    {
        protected override void OnEnter(State prevstate)
        {
            owner.Init();
            owner.ChangeBattleState(BattleState.GameStating);
        }
        protected override void OnExit(State nextstate)
        {
            base.OnExit(nextstate);
        }
    }
    /// <summary>
    /// 試合中ステート
    /// 制限時間管理や勝者判定を行う
    /// </summary>
    private class MatchPlaying : State
    {
        /// <summary>
        /// ステート開始時
        /// </summary>
        bool isEnd = false;
        protected override void OnEnter(State prevstate)
        {
            isEnd = false;
            // バトル状態を試合中へ変更
            owner.ChangeBattleState(BattleState.GamePlaying);

            // 制限時間初期化
            owner.RemainingTime = owner.TimeLimit;
        }

        /// <summary>
        /// 毎フレーム更新
        /// </summary>
        protected override void OnUpdata()
        {
            if (owner.CurrentBattleState is BattleState.GameEnd) return;
            // 残り時間減少
            owner.RemainingTime -= Time.deltaTime;
            owner.RemainingTime = Mathf.Clamp(owner.RemainingTime, 0, owner.TimeLimit);
            // 時間切れで試合終了へ
            if (owner.RemainingTime <= 0 &&!isEnd)
            {
                isEnd = true;
                owner.OnTimeOutGameAction?.Invoke();
            }
        }

        /// <summary>
        /// 勝者判定
        /// HPが一番多いプレイヤーを勝者にする
        /// 同値なら引き分け
        /// </summary>
        private void CheckWiner()
        {
            // 現在参加しているプレイヤー一覧取得
            var playersInput = PlayerInitializeManager.Instance.GetPlayersList();

            // 3は引き分け用ID
            int winnerId = 3;

            // 最も高いHP 初期値は絶対に値が入るように-1に
            float maxHp = -1;

            foreach (var inp in playersInput)
            {
                // プレイヤー状態取得
                var state = inp.GetComponent<PlayerState>();

                // 現在の最大HPを超えた場合
                if (state.CurrentHp > maxHp)
                {
                    maxHp = state.CurrentHp;

                    // 勝者更新
                    winnerId = state.PlayerNumber;
                }
                // 同じHPなら引き分け
                else if (state.CurrentHp == maxHp)
                {
                    winnerId = 3;
                }
            }

            owner.ChangeBattleState(BattleState.GameEnd);
            owner.OnUnPairUserAction?.Invoke();//操作不可にする
            owner.ToEnd(winnerId);//勝利数を設定
        }

        /// <summary>
        /// ステート終了時
        /// </summary>
        protected override void OnExit(State nextstate)
        {
            // 勝者判定
            CheckWiner();

            base.OnExit(nextstate);
        }
    }

    /// <summary>
    /// ラウンド終了ステート
    /// 続行かゲーム終了かを判定する
    /// </summary>
    private class MatchEnd : State
    {

        float timer = 0;

        float interval = 2f;

        /// <summary>
        /// ステート開始時
        /// </summary>
        protected override void OnEnter(State prevstate)
        {
            // 現在時刻保存
            timer = Time.time;

            // 試合終了判定
            CheckEndRound();
        }

        /// <summary>
        /// ラウンド継続かゲーム終了か判定
        /// </summary>
        private void CheckEndRound()
        {
            // どちらも規定勝利数に到達していない
            if (owner.Player1Wins < owner.TotalRound &&
                owner.Player2Wins < owner.TotalRound)
            {
                // 次ラウンドへ
                owner.CurrentRound++;
            }
            else
            {
                // ゲーム終了
                owner.IsGameEnd = true;

                // 勝利UI更新
                owner.OnChangeWinUiAction?.Invoke(
                    owner.Player1Wins,
                    owner.Player2Wins
                );

                //２は引き分け用のId
                int winerId = 2;

                bool isDrawGame = owner.Player1Wins == owner.Player2Wins;

                if(!isDrawGame)
                {
                    winerId = owner.Player1Wins > owner.Player2Wins?
                        PlayerDataManager.Instance.Player1Data.PlayerId:
                        PlayerDataManager.Instance.Player2Data.PlayerId;
                }

                // 表示用ID (1:Player1, 2:Player2, 3:Draw)
                const int DisplayIdOffset = 1;
                owner.OnDisplayWinTextAction?.Invoke(winerId + DisplayIdOffset);

                // 勝者判定 0~1 引き分けは2
                if (owner.Player1Wins == owner.Player2Wins)
                {
                    Debug.Log("引き分け");
                    winerId = 3;
                }
                else if (owner.Player1Wins > owner.Player2Wins)
                {
                    winerId = PlayerDataManager.Instance.Player1Data.PlayerId;
                }
                else
                {
                    winerId = PlayerDataManager.Instance.Player2Data.PlayerId;
                }
                // 勝者テキスト表示
                owner.OnDisplayWinTextAction?.Invoke(winerId + 1);
            }
        }

        /// <summary>
        /// 毎フレーム更新
        /// </summary>
        protected override void OnUpdata()
        {
            // 一定時間後に次ステートへ
            if (Time.time >= timer + interval)
            {
                timer = 0;

                stateMachine.Dispatch((int)BattleState.GameStating);
            }
        }

        /// <summary>
        /// ステート終了時
        /// </summary>
        protected override void OnExit(State nextstate)
        {

            //どちらか勝っていたらNext処理はしない
            if (owner.IsGameEnd) return;

            // 次ラウンドへ
            Next();
        }

        /// <summary>
        /// 次ラウンド用シーン遷移
        /// </summary>
        private void Next()
        {
            // フェード開始
            float duration = 0.5f;
            float startAlpha = 0;
            float endAlpha = 1;

            EventManager.Instance.Startfade(duration, startAlpha, endAlpha);

            // 少し待ってから次のラウンドへ
            float waitTime = 1.5f;

            Delay.WaitTime(owner, waitTime, owner.NextRound);
        }
    }
}
