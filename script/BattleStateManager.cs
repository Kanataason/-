using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;
using State = StateMachine<BattleStateManager>.State;

public class BattleStateManager : MonoBehaviour
{
    private StateMachine<BattleStateManager> stateMachine;
    public static BattleStateManager Instance { get; private set; }
    private enum IntState : int {GameStating,GamePose,GameEnd,GamePlaying }
    public int TotalRound = 1;
    public  int CurrentRound = 1;
    public float TimeLimit = 99;
    public float RemainingTime =0;
    public int Player1Wins = 0;
    public int Player2Wins = 0;

    public static bool IsPlayed = false;
    [SerializeField] private testUIInitialize testUIInitializes;
    private UiTextMoveManager stateManager;
    private List<PlayerInput> input = new List<PlayerInput>();

    public static event System.Action<int,int> OnWinChengeUi;
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
    }
    public void ResetBattleInfo()
    {
        Player1Wins = 0;
        Player2Wins = 0;
        CurrentRound = 1;
        IsPlayed = false;
        init();
        InitUi();
    }
    void init()
    {
  
        var ini = GameObject.Find("Battle Manager");
        testUIInitializes = ini.GetComponent<testUIInitialize>(); 
        RemainingTime = TimeLimit;
        if (!IsPlayed) return;
        ResetBattleInfo();
    }
    public void InitUi()
    {
        OnWinChengeUi?.Invoke(Player1Wins, Player2Wins);
    }
    void Start()
    {
        init();
        stateMachine = new StateMachine<BattleStateManager>(this);

        stateMachine.AddTransition<MatchStart, MatchPlaying>((int)IntState.GamePlaying);
        stateMachine.AddTransition<MatchEnd, MatchStart>((int)IntState.GameStating);
        stateMachine.AddTransition<MatchPlaying, MatchEnd>((int)IntState.GameEnd);
        stateMachine.AnyAddTrasition<MatchPose>((int)IntState.GamePose);
        stateMachine.Start<MatchStart>();
    }

    // Update is called once per frame
    void Update()
    {
        stateMachine.Updata();
        
    }
    public void ToPlay() => stateMachine.Dispatch((int)IntState.GamePlaying);
    public void ToEnd(int Id)
    {
        Debug.Log(Id);
        if (Id == 1) Player1Wins++;
        else if (Id == 2) Player2Wins++;
        else
        {
            Player1Wins++;
            Player2Wins++;
        }
        stateMachine.Dispatch((int)IntState.GameEnd);
    }
    public List<PlayerInput> Players()
    {
        return input;
    }
    public void SetPlayerinput(PlayerInput inputs) => input.Add(inputs);

    public void OnDestroys()
    {
        foreach (var pl in input)
        {
            if (pl != null)
            {
                Destroy(pl.gameObject);
            }
        }
        input.Clear(); // 参照解放

        OnWinChengeUi = null; // イベント購読解除
        Instance = null;      // シングルトン解除
        stateMachine = null;  // ステートマシン参照解除
        testUIInitializes = null;
        stateManager = null;

        Destroy(gameObject);
    }
    public void NextRound() { TatuGameManager.Instance.SceneChenge("tatuyaScene"); }
    public void CharactorSelect() { TatuGameManager.Instance.SceneChenge("TatsuyaSelectCharaactor"); }
    private class MatchStart : State
    {
        protected override void OnEnter(State prevstate)
        {
          owner.init();
        }
        protected override void OnUpdata()
        {
            base.OnUpdata();
        }
        protected override void OnExit(State nextstate)
        {
            base.OnExit(nextstate);
        }
    }
    private class MatchPlaying : State
    {
        int Player1Id;
        int Player2Id;
        float player1Hp = 0;
        float player2Hp = 0;
        protected override void OnEnter(State prevstate)
        {
            owner.RemainingTime = owner.TimeLimit;
        }
        protected override void OnUpdata()
        {
            owner.RemainingTime -= Time.deltaTime;
            if(owner.RemainingTime <=0)
            {
                owner.RemainingTime = owner.TimeLimit;
                var s = owner.Players();
                foreach (var inp in s)
                {
                    var state = inp.GetComponent<PlayerState>();
                    Debug.Log($"{state.gameObject.name}/ {state.PlayerNumber}");
                    int Id = state.PlayerNumber;
                    if (Id == 1)
                    {
                        Player1Id = Id;
                        player1Hp = state.NowHp;
                    }
                    else
                    {
                        Player2Id = Id;
                        player2Hp = state.NowHp;
                    }
                }
                if (player1Hp > player2Hp)
                {
                    owner.ToEnd(Player1Id);
                }
                else if(player1Hp < player2Hp)
                {
                    owner.ToEnd(Player2Id);
                }
                else
                {
                    owner.ToEnd(3);
                }
                UiManager.Instance.TimeOver();
            }
        }
        protected override void OnExit(State nextstate)
        {
            base.OnExit(nextstate);
        }
    }
    private class MatchEnd : State
    {
        float timer = 0;
        float Interval = 5f;
        int WiniD = 0;

        protected override void OnEnter(State prevstate)
        {
            owner.testUIInitializes.UserRemove();
            timer = Time.time;
            if (owner.Player1Wins < owner.TotalRound && owner.Player2Wins < owner.TotalRound)
            {
                Debug.Log("まだどっちも勝っていない");
                // どちらもまだ勝っていない => 継続
               owner.CurrentRound++;
                foreach (var inp in owner.input)
                {
                    inp.enabled = false;
                }
                owner.input.Clear();
            }
            else
            {
                OnWinChengeUi?.Invoke(owner.Player1Wins, owner.Player2Wins);
                var text = GameObject.Find("RoundOnetext");
                if (text == null) Debug.Log("nai");
                owner.stateManager = text.GetComponent<UiTextMoveManager>();
                Debug.Log("owari");
                foreach (var inp in owner.input)
                {
                    inp.enabled = false;
                }
                owner.input.Clear();
                // どちらかが勝った => 試合終了
                if (owner.Player1Wins >= owner.TotalRound) WiniD = 1;
                else if (owner.Player2Wins >= owner.TotalRound) WiniD = 2;
                IsPlayed = true;
               if(owner.stateManager != null) owner.stateManager.PlayerWinText(WiniD);
            }
        }
        protected override void OnUpdata()
        {
            if(Time.time >= timer + Interval)
            {
                timer = 0;
                stateMachine.Dispatch((int)IntState.GameStating);
            }
        }
        protected override void OnExit(State nextstate)
        {
            var pl = IsPlayed ? true : false;
            if (pl)
            {
                UIAudioSound.Instance.StopBGM();
               // owner.stateManager.PlayerWinText(WiniD);
            }
            else { owner.NextRound(); }
        }
    }
    private class MatchPose : State
    {
        protected override void OnEnter(State prevstate)
        {
            base.OnEnter(prevstate);
        }
        protected override void OnUpdata()
        {
            base.OnUpdata();
        }
        protected override void OnExit(State nextstate)
        {
            base.OnExit(nextstate);
        }
    }
}
