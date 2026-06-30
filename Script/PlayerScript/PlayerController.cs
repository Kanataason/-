using System;
using System.Collections.Generic;
using UnityEngine;
using static TatsuAnimationController;
using State = StateMachine<PlayerController>.State;

/// <summary>
/// プレイヤーがいま何の状態かのステート
/// </summary>
public enum PlayerAction
{
    Attack,
    Inoperable,
    Idle,
    Jump,
    Crouch,
    Die,
}
/// <summary>
/// プレイヤーの入力を受け取り条件を見て実行可能ならそれを通知するスクリプト
/// </summary>

[RequireComponent(typeof(PlayerEffectController))]
public class PlayerController : MonoBehaviour
{
    public enum Event : int//ステートの移行用ステート
    {
        MoveEvent, JumpEvent, CrouchEvent, AttackEvent,
        DieEvnet, GuardEvent, StepEvent, IsHitEvent, WinEvent,InoperableEvent
    }
    //プレイヤー入力時のアクションの情報をセットする関数
    [Serializable]
    public class ActionsInfo
    {
        public Event ActionEvent;//イベントのタイプ
        public Action TransitionAction;//遷移後に処理する関数
        public Func<bool> IsBlocked;//特定のflagの状態をいれる
    }
    #region
    private TatsuAnimationController controller;
    private PlayerState playerState;
    private SimpleMovement simpleMovement;
    private PlayerInputHandler playerInputHandler;
    private TatuCollderManager colliderManager;
    private PlayerHitDirector hitDirector;
    private PlayerKnockBackController backMotion;
    private PlayerEffectController playerEffectController;
    #endregion
    //デバックようの管理クラス
    PlayerDebagTextManager textmanager;
    StateMachine<PlayerController> stateMachine;


    public bool IsHit = false;
    public bool IsGuarding = false;
    public bool IsThrown = false;

    private bool IsAction = false;
    private bool IsCanNextAction = true;
    private bool IsCanRotation = true;
    private bool IsHitStop = false;


    private PlayerTranstionState targetCurrentState = PlayerTranstionState.Nomal;
    //技がキャンセル可能かどうかを判断するためのやつ
    public AcceptInput CurrentCancelState = AcceptInput.Disable;

    private PlayerInputHandler.ActionState currentActionState = PlayerInputHandler.ActionState.None;
    public PlayerAction PlayerActionState = PlayerAction.Idle;

    private Dictionary<PlayerInputHandler.ActionState, ActionsInfo> ActionList;
    private Dictionary<PlayerInputHandler.ActionState, PlayerInputHandler.ActionState> neutralList;

    private Queue<PlayerInputHandler.ActionState> prevComandList = new();//
    private List<PlayerInputHandler.ActionState> comandList = new();//プレイヤーの入力を保管するリスト

    //アニメーションの現在再生フレーム
    private int currentAnimationFrame;

    //ジャンプ専用の再生フレームとトータルフレーム
    private int currentJumpFrame;
    private int currentJumpTotalFrame;


    //------event-------//
    public event Action OnRemovePos;//生成場所に戻すアクション
    public event Action OnStopAction;//アニメーションが終わったことを通知するアクション
    public event Action<bool> OnSetCanMoveFlagAction;//動けることを通知するアクション
    public event Action<PlayerTranstionState> OnUpdataPlayerStateAction;//ステートを変更アクション
    public event Action<int> OnJumpTransitionAction;//イベントのJumpの処理開始通知アクション
    public event Action<string, bool> OnActiveCollisionAction;//当たり判定のアクション

    void TryGetComponents()
    {
        simpleMovement = GetComponent<SimpleMovement>();
        controller = GetComponent<TatsuAnimationController>();
        playerState = GetComponent<PlayerState>();
        playerInputHandler = GetComponent<PlayerInputHandler>();
        colliderManager = GetComponent<TatuCollderManager>();
        hitDirector = GetComponent<PlayerHitDirector>();
        backMotion = GetComponent<PlayerKnockBackController>();
        playerEffectController = GetComponent<PlayerEffectController>();
    }

    /// <summary>
    /// 遷移パターンを登録
    /// </summary>
    private void InitTransition()
    {
        stateMachine = new StateMachine<PlayerController>(this);
        stateMachine.AddTransition<MoveAndIdle, Jump>((int)Event.JumpEvent);
        stateMachine.AddTransition<MoveAndIdle, Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<MoveAndIdle, Guard>((int)Event.GuardEvent);
        stateMachine.AddTransition<MoveAndIdle, Attack>((int)Event.AttackEvent);
        stateMachine.AddTransition<MoveAndIdle, Step>((int)Event.StepEvent);

        stateMachine.AddTransition<Attack, Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<Attack, MoveAndIdle>((int)Event.MoveEvent);
        stateMachine.AddTransition<Attack, Jump>((int)Event.JumpEvent);

        stateMachine.AddTransition<Crouch, Attack>((int)Event.AttackEvent);
        stateMachine.AddTransition<Crouch, Guard>((int)Event.GuardEvent);
        stateMachine.AddTransition<Crouch, Crouch>((int)Event.CrouchEvent);

        stateMachine.AddTransition<Guard, Attack>((int)Event.AttackEvent);
        stateMachine.AddTransition<Guard, Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<Guard, Jump>((int)Event.JumpEvent);

        stateMachine.AddTransition<Jump, Jump>((int)Event.JumpEvent);
        stateMachine.AddTransition<Jump, Attack>((int)Event.AttackEvent);

        stateMachine.AddTransition<Hitting, Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<Hitting, MoveAndIdle>((int)Event.MoveEvent);
        stateMachine.AddTransition<Hitting, Guard>((int)Event.GuardEvent);

        stateMachine.AddTransition<Die, MoveAndIdle>((int)Event.MoveEvent);
        stateMachine.AddTransition<Win, MoveAndIdle>((int)Event.MoveEvent);

        stateMachine.AnyAddTrasition<MoveAndIdle>((int)Event.MoveEvent);
        stateMachine.AnyAddTrasition<Die>((int)Event.DieEvnet);
        stateMachine.AnyAddTrasition<Hitting>((int)Event.IsHitEvent);
        stateMachine.AnyAddTrasition<Inoperable>((int)Event.InoperableEvent);
        stateMachine.AnyAddTrasition<Win>((int)Event.WinEvent);
        stateMachine.Start<MoveAndIdle>();
    }

    /// <summary>
    /// リストの初期化
    /// </summary>
    private void InitList()
    {
        //ステートによって処理するアクションを変えて登録している
        //プレイヤーの入力によって実行するアクションを設定
        ActionList = new()
        {
            {PlayerInputHandler.ActionState.WeakAttack,new ActionsInfo()
            {ActionEvent = Event.AttackEvent,TransitionAction =controller.OnWeakAttack} },

            {PlayerInputHandler.ActionState.MidlleAttack,new ActionsInfo() 
            { ActionEvent = Event.AttackEvent, TransitionAction = controller.OnMiddleAttack }},

            {PlayerInputHandler.ActionState.StrongAttack,new ActionsInfo()
            { ActionEvent = Event.AttackEvent, TransitionAction = controller.OnStrongAttack } }, 

            {PlayerInputHandler.ActionState.SpecialAttack,new ActionsInfo()
            { ActionEvent = Event.AttackEvent, TransitionAction = controller.OnSpecialAttack } },

            {PlayerInputHandler.ActionState.TiltAttack,new ActionsInfo()
            { ActionEvent = Event.AttackEvent, TransitionAction = controller.OnTiltAttack }},

            {PlayerInputHandler.ActionState.SmashAttack,new ActionsInfo()
            { ActionEvent = Event.AttackEvent, TransitionAction = controller.OnSmashAttack 
            ,IsBlocked = ()=>simpleMovement.IsJumping}},

            {PlayerInputHandler.ActionState.Graping,new ActionsInfo() 
            { ActionEvent = Event.AttackEvent, TransitionAction = controller.OnThrowAttack } },

            {PlayerInputHandler.ActionState.Jumping,new ActionsInfo() 
            { ActionEvent = Event.JumpEvent,TransitionAction = null,
                IsBlocked = ()=>playerState.currentState.HasFlag(StatusFlags.IsThrowing)
                ||simpleMovement.IsJumping||PlayerActionState == PlayerAction.Attack}},

            {PlayerInputHandler.ActionState.Crouching,new ActionsInfo()
            { ActionEvent = Event.CrouchEvent, TransitionAction = null ,
                IsBlocked = ()=>simpleMovement.IsJumping||PlayerActionState == PlayerAction.Attack}},
        };

        neutralList = new()
        {
            {PlayerInputHandler.ActionState.Crouching,PlayerInputHandler.ActionState.Jumping },
             {PlayerInputHandler.ActionState.Jumping,PlayerInputHandler.ActionState.Crouching},
        };
    }

    #region Debag
    //デバッグ用のスクリプトを取得
    public void SetDebagTextManager(PlayerDebagTextManager manager)
    {
        textmanager = manager;
        SetUpDebagInfo();
    }

    //debagのためにテキストマネージャーに渡す処理
    private void SetUpDebagInfo()
    {
        textmanager.Register("IsCanNext", () => IsCanNextAction.ToString());
        textmanager.Register("IsAction", () => IsAction.ToString());
        textmanager.Register("CurrentCancel", () => CurrentCancelState.ToString());
        textmanager.Register("Jyoutai",()=> playerState.currentState.ToString());
        textmanager.Register("PlayerCurrentAction",()=>PlayerActionState.ToString());
        textmanager.Register("StateMachine.CurrentState", () => stateMachine.CurrentState.ToString());
        textmanager.Register("PlayerState", () => playerState.currentTranstionState.ToString());
        textmanager.Register("IsJump", () => simpleMovement.IsJumping.ToString());
        textmanager.SetEndText();
    }
    #endregion
    private void Start()
    {
        //コンポーネントを取得
        TryGetComponents();

        //現在のHpをUiに反映させる
        playerState.OffHpbarChenge();

        //イベントの登録
        SubscribeEvents();

        //リストの初期化
        InitList();

        //１フレーム待つ
        Delay.OneFrame(this, InitTransition);
    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }
    private void SubscribeEvents()//イベント登録
    {
        //入力を受け取って処理するイベントを登録
        if (playerInputHandler != null)
        playerInputHandler.OnAction += CheckAction;

        //地面についた通知をするイベントを登録
        if (backMotion != null)
            backMotion.OnLanded += HasInoperable;


        if (BattleStateManager.Instance == null) return;

        //プレイヤーが死んだときのイベントを登録
        BattleStateManager.Instance.OnGameEndAction += OnPlayGameEndAnimation;

        //プレイヤーの状態を初期化のイベントを登録
        BattleStateManager.Instance.OnNextRoundAction += InitPlayerState;
    }
    private void UnSubscribeEvents()//イベント解除
    {
        Debug.Log("解除");
        //入力を受け取って処理するイベントを解除
        if (playerInputHandler != null)
        playerInputHandler.OnAction -= CheckAction;

        //地面についた通知をするイベントを解除
        if (backMotion != null)
            backMotion.OnLanded -= HasInoperable;

        if (BattleStateManager.Instance == null) return;

        //プレイヤーが死んだときのイベントを解除
        BattleStateManager.Instance.OnGameEndAction -= OnPlayGameEndAnimation;

        //プレイヤーの状態を初期化のイベントを解除
        BattleStateManager.Instance.OnNextRoundAction -= InitPlayerState;
    }

    //ラウンドが終わったら流れる 
    private void OnPlayGameEndAnimation()
    {
        //自分が死んでいたらDie 死んでなかったらWinに遷移
        var currentEvent = playerState.IsDead ? Event.DieEvnet : Event.WinEvent;

        stateMachine.Dispatch((int)currentEvent);
    }
    private void CheckSpecialPlayerState()
    {
        //パワーアップ状態ならノーマル状態に戻す
        if (playerState.currentState.HasFlag(StatusFlags.IsPowerUp))
        {
            //タイマーをリセットさせるために０を送る
            playerState.OnStartPowerUp(true);
        }
    }

    //ネクストラウンドに移るときに呼ばれる
    private void InitPlayerState()
    {
        //位置を生成元に戻す
        OnRemovePos?.Invoke();

        //体力やゲージ残量を初期化
        playerState.Init();

        //アニメーションを最初のアニメーションにする
        controller.NextRoundAnimation();

        //状態をIdleに
        stateMachine.Dispatch((int)Event.MoveEvent);
    }
    private void Update()
    {
        if (BattleStateManager.Instance == null) return;


        //ゲームの進行が不可能にならないようにするため
        if(transform.position.y < -10)
        {
            //めり込んで下に落ちたときに元の場所に戻るよう
            OnRemovePos?.Invoke();
        }

        //時間が止まっている時は更新をしない
        if(!BattleStateManager.Instance.GetPauseFlag())
        stateMachine?.Updata();

    }

    private void FixedUpdate()
    {
        stateMachine?.FixedUpdates();
    }
    /// <summary>
    /// プレイヤーが入力した動作に紐づいているアクション実行やステートに変更する関数
    /// </summary>
    /// <param name="inputState"></param>
    /// <param name="actionState"></param>
    private void CheckAction(PlayerInputHandler.InputState inputState, 
        PlayerInputHandler.ActionState actionState = PlayerInputHandler.ActionState.None)
    {
        //ポーズ中なら何もしない
        if (BattleStateManager.Instance.GetPauseFlag()) return;

        //ゲーム中でしか動けないようにする
        if (BattleStateManager.Instance.CurrentBattleState is not BattleStateManager.BattleState.GamePlaying)
            return;

        //ボタンを離したか確認
        bool IsCancel = inputState == PlayerInputHandler.InputState.Canceled;

        //ボタンを押した瞬間か確認
        bool IsPerform = inputState == PlayerInputHandler.InputState.Performed;


        if (IsCancel)
        {
            if(comandList.Contains(actionState))
            {
                RemoveComandList(actionState);
            }
        }
        if (IsPerform)
        {
            if (!comandList.Contains(actionState))
            {
                comandList.Add(actionState);
            }

        }


        //アクションが登録されているかを確認
        if (!ActionList.TryGetValue(actionState, out var currentAction))
        {
            return;
        }

        if (CanPlayComand(inputState, currentAction, actionState)) return;

        if (IsCancel)
        {
            Debug.Log("キャンセルしました");
            
            RemoveComandList(actionState);

            if (CanNeuTralPlay()) return;
            
            currentActionState = PlayerInputHandler.ActionState.None;

            CanMoveState();

            stateMachine.Dispatch((int)Event.MoveEvent);
            return;
        }
        if (IsPerform)
        {
            //キャンセル可能の時ならアクション中でも行動可能

            //ニュートラルの行動かを見る
            if (CheckNeutral(currentActionState, actionState)) return;

            ChangeIsActionFlag(true);
            ChangeCanNextActionFlag();
            //現在のアクションステータスを設定
            currentActionState = actionState;

            currentAction.TransitionAction?.Invoke();

            stateMachine.Dispatch((int)currentAction.ActionEvent);

        }

    }
    private bool CanPlayComand(PlayerInputHandler.InputState inputState,
      ActionsInfo currentAction, PlayerInputHandler.ActionState actionState = PlayerInputHandler.ActionState.None)
    {
        //行動禁止の条件があったらみる
        if (currentAction.IsBlocked != null && currentAction.IsBlocked())
        {
            Debug.Log("ブロックされた");
            return true;
        }
        //捕まれていたら何もできないようにする
        if (playerState.currentState.HasFlag(StatusFlags.IsThrowing)
            && actionState is not PlayerInputHandler.ActionState.Graping)
        {
            Debug.Log("現在捕まえられている");
            return true;
        }

        //アクション中で行動可能かどうかみる可能じゃなければ返す
        if ((!IsCanNextAction && IsAction) || PlayerActionState == PlayerAction.Inoperable)
        {
            Debug.Log($"まだ行動できない{actionState}cancel{inputState}");
            return true;
        }

        //キャンセルできる状態じゃなかったら返す
        if (CurrentCancelState == AcceptInput.Disable)
        {
            Debug.Log($"キャンセル可能状態にできない{actionState}cancel{inputState}");
            return true;
        }return false;
    }






    private bool CanNeuTralPlay()//これはCPT方式のための関数
    {
        if (prevComandList.Count > 0)//ニュートラル時の物があったら再生
        {
            var eventId = prevComandList.Dequeue();
            bool isCanNeutral = false;

            foreach (var state in comandList)
            {
                if (state == eventId)
                    isCanNeutral = true;
            }
            if (!isCanNeutral) return false;

            if (!ActionList.TryGetValue(eventId, out var actions)) return false;

            ChangeIsActionFlag(true);
            currentActionState = eventId;
            stateMachine.Dispatch((int)actions.ActionEvent);
            actions.TransitionAction?.Invoke();
            return true;
        }
        return false;
    }

    //リストに入っているステート同士ならニュートラルにする
    private bool CheckNeutral(PlayerInputHandler.ActionState currentState, PlayerInputHandler.ActionState prevState)
    {
        if(neutralList.TryGetValue(currentState,out var resaltState))
        {
            if(resaltState == prevState)
            {
                Debug.Log("ニュートラル");
                if(prevComandList.Count == 0)
                prevComandList.Enqueue(currentState);//今のステートを入れる

                CheckAction(PlayerInputHandler.InputState.Canceled, currentState);
                currentActionState = PlayerInputHandler.ActionState.None;
                return true;
            }
        }
        return false;
    }
    //プレイヤーの向きを取る処理 バックならtrueがかえる
    private bool PlayerCheckDirection()//自分の向きを取得 フラグで取得
    {
        int right = 1;
        int left = -1;
        float backBorder = -0.5f;

        Vector2 input = playerInputHandler.MoveInput;

        int facingDir = transform.forward.x > 0 ? right : left;

        return input.x * facingDir < backBorder;
    }
    private int GetDirection()//数値で取得
    {
        int right = 1;
        int left = -1;
        float backBorder = 0.5f;

        float moveInputX = playerInputHandler.MoveInput.x;

        // ニュートラル
        if (Mathf.Abs(moveInputX) < backBorder)
            return 0;

        // キャラの向き
        int facingDir = transform.forward.x > 0 ? right :left;

        // 入力方向
        int inputDir = moveInputX > 0 ? right : left;

        // 向き考慮
        return inputDir * facingDir;
    }
    private PlayerTranstionState CheckGuard()//ガードしているか確認
    {
        var dir = GetDirection();

        bool isBack = dir == -1;
        bool isCrouching = playerInputHandler.IsCrouching;
        PlayerTranstionState nextState;

        if (!isBack)
        {
            return isCrouching ? PlayerTranstionState.Crouch :
                                 PlayerTranstionState.Nomal;
        }

        nextState = isCrouching
    ? PlayerTranstionState.CrouchGuard
    : PlayerTranstionState.StandGuard;

        return nextState;
    }

    /// <summary>
    /// 現在の行動ステートに変更
    /// </summary>
    /// <param name="state"></param>
    public void ChangePlayerActionState(PlayerAction state)
    {
        PlayerActionState = state;
    }
    /// <summary>
    /// 技のキャンセルステートを変更
    /// </summary>
    /// <param name="state"></param>
    public void ChangeCancelActionState(AcceptInput state)
    {
        CurrentCancelState = state;
    }
    /// <summary>
    /// 行動可能フラグを変更
    /// </summary>
    /// <param name="isNextAction"></param>
    public void ChangeCanNextActionFlag(bool isNextAction = false)
    {
        IsCanNextAction = isNextAction;
    }
    /// <summary>
    /// アクション実行フラグを変更
    /// </summary>
    /// <param name="StartAction"></param>
    private void ChangeIsActionFlag(bool StartAction = false)
    {
        IsAction = StartAction;
    }
    /// <summary>
    /// 入力履歴を消す
    /// </summary>
    /// <param name="state"></param>
    private void RemoveComandList(PlayerInputHandler.ActionState state)
    {
      comandList.Remove(state);
    }

    /// <summary>
    /// プレイヤーの回転を止めるフラグを変更
    /// </summary>
    /// <param name="isRotation"></param>
    public void SetCanRotationFlag(bool isRotation = false)
    {
        IsCanRotation = isRotation;
    }
    /// <summary>
    /// ヒットorガードのフラグを変更
    /// </summary>
    /// <param name="hitState"></param>
    /// <param name="isTrue"></param>
    public void SetHitOrGuardFlag(PlayerHitDirector.HitResult hitState,bool isTrue = true)
    {
        switch (hitState)
        {
            case PlayerHitDirector.HitResult.Hit:
                IsHit = isTrue; break;
            case PlayerHitDirector.HitResult.Guard:
                IsGuarding = isTrue; break;
            default: break;
        }
    }
    /// <summary>
    /// 引数で与えられたステートが遷移可能かチェックしに行く
    /// </summary>
    /// <param name="actionState"></param>
    public void ChangePlayerState(PlayerInputHandler.ActionState actionState)
    {
        CheckAction(PlayerInputHandler.InputState.Performed, actionState);
    }
    /// <summary>
    /// ヒットステートに強制遷移
    /// </summary>
    public void ProcessHit()
    {
        stateMachine.Dispatch((int)Event.IsHitEvent);
    }
    /// <summary>
    /// 行動不能ステートに強制遷移
    /// </summary>
    public void HasInoperable()
    {
        if (playerState.IsDead) return;
        stateMachine.Dispatch((int)Event.InoperableEvent);
    }

    //演出用の関数
    public void PlayerStartPresentation(Action<Transform,Action> presentationEvent, Transform targetPos,Action receiver)
    {
        PlayPresentation();

        //演出を再生するためのイベント
        presentationEvent?.Invoke(targetPos,receiver);
    }
    private void PlayPresentation()
    {
        //ここが依存しまくっているから変える
        var effectType = playerEffectController.CanPlayPresentationEffect(controller.GetPresentationState());

        if (effectType is not EffectType.None)
        {
            Vector3 root = controller.GetAnchor(TatsuAnimationController.EffectAnchor.Root).position + Vector3.up;
            int returnTime = 60;

            playerEffectController.PlayEffect(effectType, root, Quaternion.identity, returnTime);
        }
    }
    /// <summary>
    /// 行動可能にするためのフラグやステートを変更
    /// </summary>
    public void CanMoveState()
    {
        ChangeCanNextActionFlag(true);
        ChangeIsActionFlag();
        ChangePlayerActionState(PlayerAction.Idle);
        ChangeCancelActionState(AcceptInput.Normal);
    }
    public bool GetCanRotation() { return IsCanRotation; }


    public void SetHitStopFlag(bool isStop = false) => IsHitStop = isStop;
    public bool GetHitStopFlag() => IsHitStop;
    public void SetTargetTranstionState(PlayerTranstionState state) => targetCurrentState = state;
    //行動をステートにしてステートにして管理することで視認性があがりデバックもやりやすい
    private class MoveAndIdle : State
    {
        private float _prevDir;//前フレームのプレイヤーが向いている方向

        protected override void OnEnter(State prevstate)
        {
            owner.SetCanRotationFlag(true);
           _prevDir = 0;

            InitState();
          
            InvokeEventAction();
        }
        private void InitState()
        {
            if (owner.IsCanNextAction)
                owner.ChangePlayerActionState(PlayerAction.Idle);

            owner.currentActionState = PlayerInputHandler.ActionState.None;
        }
        private void InvokeEventAction()//イベントを発火
        {
            owner.OnUpdataPlayerStateAction?.Invoke(PlayerTranstionState.Nomal);//これを登録する
            owner.OnActiveCollisionAction?.Invoke("Idle", true);
            owner.OnSetCanMoveFlagAction?.Invoke(false);
        }
        protected override void OnUpdata()
        {
            var Input = owner.playerInputHandler.MoveInput;
            if (owner.PlayerActionState == PlayerAction.Inoperable|| BattleStateManager.Instance == null) return;

            //ゲームが始まっていたらアニメーション開始
           if(BattleStateManager.Instance.CurrentBattleState is not BattleStateManager.BattleState.GameStating)
                owner.controller.PlayMoveAnimation(Input.x);

           //コマンドが押されていたか確認
            if (CheckBufferComand()) return;

            //ステップできるか確認
            if (TryStep(Input)) return;

            //ガードしているか確認
            if (TryGuard()) return;

        }
        private bool CheckBufferComand()
        {
            if (owner.IsAction) return true;
            foreach (var state in owner.comandList)
            {
                owner.CheckAction(PlayerInputHandler.InputState.Performed, state);
                return true;
            }
            return false;
        }
        //ステップするかの確認
        private bool TryStep(Vector2 moveInput)
        {
            //ステップできない状態か、しゃがんでいたらステップしない
            if (owner.simpleMovement.IsStepping()||owner.playerInputHandler.IsCrouching) return false;

            bool IsAction = false;
            int dir = Mathf.RoundToInt(moveInput.x);
            if (_prevDir == 0 && dir != 0)
            {
                //行動不可能じゃなかったらステップ
                if (owner.PlayerActionState is not PlayerAction.Inoperable)
                {
                    Vector3 playerPos = owner.transform.position;
                    bool IsBack = owner.PlayerCheckDirection();
                    int waitTime = 9;

                    if (owner.simpleMovement.CheckCanStep(dir,playerPos,IsBack,waitTime))
                    {
                        Debug.Log("Step");
                        owner.stateMachine.Dispatch((int)Event.StepEvent);
                        IsAction = true;
                    }
                }
            }
            _prevDir = dir;
            return IsAction;
        }
        //ガードするかの確認
        private bool TryGuard()
        {
            //立ガードがしゃがみガードを見ている
            var currentGuard = owner.CheckGuard();
            bool isGuard = currentGuard == PlayerTranstionState.CrouchGuard ||
                           currentGuard == PlayerTranstionState.StandGuard;


            if (isGuard&&owner.PlayerActionState != PlayerAction.Attack)
            {
                stateMachine.Dispatch((int)Event.GuardEvent);
                return true;
            }
            return false;
        }
        protected override void FixedUpdate()
        {
            if (owner.PlayerActionState == PlayerAction.Inoperable) return;

            if (!owner.IsAction)
            owner.simpleMovement.UpdateMoving(owner.playerInputHandler.MoveInput);
        }
        protected override void OnExit(State nextstate)
        {
            RemoveDirection();
            //動きをのフラグを止める
            owner.OnSetCanMoveFlagAction?.Invoke(true);
        }
        private void RemoveDirection()
        {
            //後ろ方向の入力以外なら消す
            if (stateMachine.NextState is not (int)Event.GuardEvent)
                owner.simpleMovement.InitDirection();
        }
    }
    private class Jump : State
    {
        protected override void OnEnter(State prevstate)
        {
            EnterJump();
            owner.colliderManager.ClearThrowBox();
        }

        private void EnterJump()//ジャンプの処理を通知
        {
            SetUpJump();

            owner.SetCanRotationFlag();
            owner.ChangePlayerActionState(PlayerAction.Jump);
            owner.ChangeCanNextActionFlag(true);//キャンセル可能な状態にする

            InitFrame();

            InvokeEventAction();
        }

        private void InitFrame()//アニメーションやジャンプに必要なフレームの初期化
        {
            var currentData = owner.controller.GetCurrentAttackData();
            owner.currentJumpTotalFrame = currentData.frameData[^1].FrameNumber;
            owner.currentJumpFrame = 0;
            owner.currentAnimationFrame = 1;
        }
        private void SetUpJump()//ジャンプの処理を準備
        {
            //方向を0,1.-1で取得してジャンプ方向を変える
            var Movex = owner.playerInputHandler.MoveInput.x;
            var dir = Mathf.RoundToInt(Movex);

            owner.OnJumpTransitionAction?.Invoke(dir);
        }
        private void InvokeEventAction()//イベントを発火
        {
            owner.OnUpdataPlayerStateAction?.Invoke(PlayerTranstionState.Jump);//ステートを変更
            owner.OnActiveCollisionAction?.Invoke("Jump", false);//コライダーをアップデート
        }
        protected override void OnUpdata()
        {
            if (owner.PlayerActionState is not PlayerAction.Jump)
                return;

            //アニメーション更新
            owner.controller.PlayAnimation(owner.currentAnimationFrame);

            //ジャンプの動作更新
            owner.simpleMovement.UpdateJumps(owner.currentJumpFrame,owner.currentJumpTotalFrame);

            //フレームを更新
            owner.currentAnimationFrame++;
            owner.currentJumpFrame++;

            //ジャンプが終わったらステートを遷移
            if (!owner.simpleMovement.IsJumping)
                CheckComand();

            
        }
        private void CheckComand()
        {
            if (CheckCancelAction()) return;

            owner.CheckAction(PlayerInputHandler.InputState.Canceled,PlayerInputHandler.ActionState.Jumping);
        }
        private bool CheckCancelAction()//キャンセルが押されていたかどうかを判断
        {
            if(owner.comandList.Count > 0)
            {
                foreach (var cancel in owner.comandList)
                {
                    if (cancel != PlayerInputHandler.ActionState.Jumping) continue;

                    owner.CheckAction(PlayerInputHandler.InputState.Performed,
                       cancel);
                    return true;

                }
            }
            return false;
        }
        protected override void OnExit(State nextstate)
        {
            //遷移先がアタックイベントならアニメーションを初期化しない
            if (stateMachine.NextState is (int)Event.AttackEvent) return;

            owner.simpleMovement.ResetJumpFlag(false);

            if (stateMachine.NextState is (int)Event.IsHitEvent) return;

            owner.controller.EndAnime();
        }
    }
    private class Crouch : State//ガード時はステートだけ変わり、クラスは変わらない
    {
        float firstDir = 0f;
        protected override void OnEnter(State prevstate)
        {
            //ステップの方向を初期化
            owner.simpleMovement.InitDirection();

            //すべての行動制限フラグを行動可能にする
            owner.CanMoveState();

            //遷移直後の入力方向を取得
            firstDir = owner.GetDirection();

            //しゃがみアニメーションを再生
            owner.controller.PlayNormalAnimation<bool>(TatsuAnimationController.AnimationId.Crouching, true);

            UpdateCollider();

            var state = owner.CheckGuard();
            InvokeUpdateStateAction(state);
        }
        private void InvokeUpdateStateAction(PlayerTranstionState state)//イベントを発火
        {
            owner.ChangePlayerActionState(PlayerAction.Crouch);
            owner.OnUpdataPlayerStateAction?.Invoke(state);
        }
        private void UpdateCollider()//コライダーをアップデート
        {
            owner.OnActiveCollisionAction?.Invoke("Crouch", true);
        }
        protected override void OnUpdata()
        {
            //入力方向と見ている方向が違ったらステータスを変える
            CheckCrouching();

            //遷移直後の入力方向と現在の入力方向を見てガードしているかを確認
            CheckDirection();

        }
        private void CheckCrouching()
        {
            //しゃがんでなかったらidleに戻す
            if(!owner.comandList.Contains(PlayerInputHandler.ActionState.Crouching))
            {
                stateMachine.Dispatch((int)Event.MoveEvent);
            }
        }

        private void CheckDirection()//入力方向を見てガードしているかをチェック
        {
            //現在方向を入力方向を取得
            var dir = owner.GetDirection();
            
            //遷移直後と現在の入力方向が違ったら戻す
            if (dir == firstDir)
                return;

            bool isBack = dir == -1;
            bool isCrouching = owner.playerInputHandler.IsCrouching;

            PlayerTranstionState nextState;

            if (isBack)
            {
                nextState = isCrouching
                    ? PlayerTranstionState.CrouchGuard
                    : PlayerTranstionState.StandGuard;

            }
            else
            {
                nextState = isCrouching
                    ? PlayerTranstionState.Crouch
                    : PlayerTranstionState.Nomal;
            }

            //変えようとしているステートが現在のステートなら変更しない
            if (owner.playerState.currentTranstionState == nextState)
                return;

            //入力方向の変更
            firstDir = dir;

            //
            InvokeUpdateStateAction(nextState);
        }
        protected override void OnExit(State nextstate)
        {
            //遷移先がガードならしゃがみ状態のまま遷移
            if (reverceState.TryGetValue(nextstate, out var state)
                && state == (int)Event.GuardEvent) return;

            //しゃがみアニメーション解除
            owner.controller.PlayNormalAnimation<bool>
                (TatsuAnimationController.AnimationId.Crouching, false);

        }
    }
    private class Guard : State
    {
        PlayerTranstionState currentState;
        bool isCrouching;
        protected override void OnEnter(State prevstate)
        {
            isCrouching = owner.comandList.Contains(PlayerInputHandler.ActionState.Crouching);
            //しゃがんでいなかったらアニメーション再生終了
            if(!isCrouching)
                owner.controller.PlayNormalAnimation<bool>(TatsuAnimationController.AnimationId.Crouching,false);

            InvokeEventAction(owner.CheckGuard());
        }
        private void InvokeEventAction(PlayerTranstionState state)//イベントを発火
        {
            owner.OnSetCanMoveFlagAction?.Invoke(false);
            owner.OnUpdataPlayerStateAction?.Invoke(state);
        }

        protected override void OnUpdata()
        {
            if (!isCrouching)
            {
                var moveInput = owner.playerInputHandler.MoveInput;
                owner.controller.PlayMoveAnimation(moveInput.x);
            }
            //ガードをしているかの確認
            CheckGuarding();
        }


        private void CheckGuarding()
        {
            // ガード維持条件 ガードボタン + 逆方向入力
            bool isStillGuarding = owner.PlayerCheckDirection();

            if (!isStillGuarding)
            {
                stateMachine.Dispatch((int)Event.MoveEvent);
                return;
            }
            if (isStillGuarding)
            {

                var IsChangeState = currentState != owner.CheckGuard();
                if(IsChangeState)
                {
                    var state = owner.CheckGuard();
                    InvokeEventAction(state);
                    currentState = state;
                }
            }


        }
        protected override void FixedUpdate()
        {
            if (!owner.IsAction&&!isCrouching)
                owner.simpleMovement.UpdateMoving(owner.playerInputHandler.MoveInput);
        }
        protected override void OnExit(State nextstate)
        {
            owner.OnSetCanMoveFlagAction?.Invoke(true);
            Debug.Log("ガード解除");
        }
    }
    private class Attack : State
    {
        private int prevEvent;//この前のイベントをintで管理

        private PlayerAction currentState;//現在のプレイヤーの行動
        private PlayerInputHandler.ActionState prevState;//

        private TatuAttackData currentAttackData;//現在の技データ

        private int currentHitStopFrame;

        protected override void OnEnter(State prevstate)
        {

            SetState(prevstate);
            currentAttackData = owner.controller.GetCurrentAttackData();
            if (currentAttackData == null)
            {
                //無限にこのステートにならないようにIdleにする
                Delay.OneFrame(owner, () => { stateMachine.Dispatch((int)Event.MoveEvent); });
                return;
            }
            //debagのために現在のフレームをセットしている
            owner.textmanager.SetAttackDataInfo(currentAttackData);

            if (owner.GetHitStopFlag()) owner.controller.StopClipAnimation();

            owner.SetCanRotationFlag();

            InitFrame();

            owner.colliderManager.InitColliderInfo(currentAttackData);


            if(prevEvent != (int)Event.JumpEvent)
                owner.simpleMovement.LockRotationPlayer();//向きを固定
        }
        private void InitFrame()
        {
            owner.currentAnimationFrame = 1;
            currentHitStopFrame = 1;
        }

        private void SetState(State state)//ステートをセットする
        {
            //現在のステートを取得する
            prevState = owner.currentActionState;

            //ジャンプしていたらActionStateをジャンプにする
            reverceState.TryGetValue(state, out prevEvent);
            currentState = prevEvent is (int)Event.JumpEvent ? PlayerAction.Jump : PlayerAction.Attack;

            if (currentState is not PlayerAction.Jump)
                owner.simpleMovement.ResetJumpFlag(false);

            owner.ChangePlayerActionState(currentState);
            owner.ChangeCancelActionState(AcceptInput.Disable);
        }

        /// <summary>(
        /// 攻撃のフレームが終わるもしくは、敵もしくは自分が当たると次の処理開始
        /// </summary>
        protected override void OnUpdata()//スーパークラスからのUpdate関数
        {
            // ジャンプ攻撃中はジャンプ処理を優先し、終了したら通常遷移を行う
            if (CurrentPlayingJump()) return;

            if (currentAttackData == null) return;

            if (owner.GetHitStopFlag())
            {
                UpdateHitStopFrame();
                return;
            }
            //debagのために現在のフレームを渡している
            owner.textmanager.UpdateFrame(owner.currentAnimationFrame);
            //当たり判定更新
            owner.colliderManager.UpdateColliders(currentAttackData, owner.currentAnimationFrame);

            //アニメーション更新
            owner.controller.PlayAnimation(owner.currentAnimationFrame);

            CheckEndAnimation();

            //フレーム更新
            owner.currentAnimationFrame++;
            owner.currentJumpFrame++;

        }
        private void CheckEndAnimation()
        {
            if (currentAttackData == null) return;

            if (owner.currentAnimationFrame >= currentAttackData.frameData[^1].FrameNumber)
            {
                if (currentState == PlayerAction.Jump)
                    owner.OnUpdataPlayerStateAction?.Invoke(PlayerTranstionState.Flying);
                //ジャンプ攻撃中ならジャンプが終わるまで終了しない
                if (!owner.simpleMovement.IsJumping)
                {
                    owner.CanMoveState();
                    owner.CheckAction(PlayerInputHandler.InputState.Canceled, prevState);
                }
            }
        }
        private void UpdateHitStopFrame()//ヒットストップの処理
        {
            int hitStopFrame = currentAttackData.HitStopFrame;

            //相手がガードしていたらガード用のフレームにする
            if (owner.targetCurrentState is PlayerTranstionState.StandGuard 
                or PlayerTranstionState.CrouchGuard) 
                hitStopFrame = currentAttackData.GuardStopFrame;

            if (currentHitStopFrame >= hitStopFrame)
            {
                if (owner.GetHitStopFlag()) owner.controller.StartClipAnimation();
                owner.SetHitStopFlag();
            }
            currentHitStopFrame++;
        }
        private bool CurrentPlayingJump()//ジャンプしているかを見ている
        {
            if (currentState != PlayerAction.Jump) return false;

          if(owner.simpleMovement.UpdateJumps(owner.currentJumpFrame, owner.currentJumpTotalFrame))
            {
                owner.CanMoveState();
                owner.CheckAction(PlayerInputHandler.InputState.Canceled, prevState);
                return true;
            }
            return false;
        }


        protected override void OnExit(State nextstate)
        {
            Debug.Log("終わり");
            currentAttackData = null;
            RemoveComandList();
            owner.controller.SetThrowingFlag();//投げフラグを解除
            owner.OnStopAction?.Invoke();

            //次の遷移ステートがヒットか行動不可能ステートなら以降の処理をしない
            if (stateMachine.NextState is (int)Event.IsHitEvent or (int)Event.InoperableEvent)
                return;

            owner.controller.EndAnime();

            owner.CanMoveState();

        }//スーパークラスからのEnd関数　次のステートに移行
        private void RemoveComandList()//実行したコマンドを消去
        {
            owner.RemoveComandList(prevState);
        }
    }
    private class Step: State
    {
        private bool isBack;
        protected override void OnEnter(State prevstate)
        {

            owner.SetCanRotationFlag();
            owner.ChangePlayerActionState(PlayerAction.Inoperable);

            //バックならtrueが返ってくる
            isBack = owner.PlayerCheckDirection();

            InitializeStep();
            DisableCollision();
            owner.currentAnimationFrame = 1;
        }
        private void InitializeStep()//エフェクト再生
        {
            //アニメーション再生箇所を取得
            Transform leftLeg = owner.controller.GetAnchor(TatsuAnimationController.EffectAnchor.LeftReg);
            Transform rightLeg = owner.controller.GetAnchor(TatsuAnimationController.EffectAnchor.RightLeg);

            //エフェクト消滅フレームを設定
            int returnTime = 20;
            owner.playerEffectController.PlayEffect
              (EffectType.Step, leftLeg.position, Quaternion.identity, returnTime);

            owner.playerEffectController.PlayEffect
              (EffectType.Step, rightLeg.position, Quaternion.identity, returnTime);

            

            //アニメーションを初期化
            if (isBack) return;//バックなら返す

            owner.controller.PlayAnimation(TatsuAnimationController.AnimationId.Stepping);
        }
        private void DisableCollision()//イベントを発火
        {
            owner.OnActiveCollisionAction?.Invoke("", false);
        }
        protected override void OnUpdata()
        {
            //プレイヤーの位置を更新
            owner.simpleMovement.UpdataStep(owner.transform.position,owner.currentAnimationFrame);

            //アニメーションを更新
            if(!isBack)
                owner.controller.PlayAnimation(owner.currentAnimationFrame);

            owner.currentAnimationFrame++;

            //ステップが終わったらステートを遷移
            if (!owner.simpleMovement.IsStepping())
                owner.stateMachine.Dispatch((int)Event.MoveEvent);
        }
      
        protected override void OnExit(State nextstate)
        {
            owner.simpleMovement.InitStepInfo();//ステップのステータスを初期化

            owner.controller.EndAnime();
        }
    }
    private class Die : State
    {
        TatuAttackData currentAnimationData;
        TatuAttackData NextAnimationData;

        bool isUsingAnimatorDeath;
        int animationTotalFrame;
        protected override void OnEnter(State prevstate)
        {
            Delay.OneFrame(owner, () =>
            {
                currentAnimationData = owner.controller.GetCurrentAttackData();

                //特定のアニメーションが再生されていたらDieアニメーションは再生しない
                if (!CanCurrentAnimation(currentAnimationData.ActionName))
                {
                    isUsingAnimatorDeath = true;
                    owner.controller.StopAnimation();
                    owner.controller.PlayDieAnimation();
                }
                InitFrame();

                owner.ChangePlayerActionState(PlayerAction.Die);
            });
        }
        private void InitFrame()
        {
            animationTotalFrame = currentAnimationData.frameData[^1].FrameNumber -2;
            if (owner.currentAnimationFrame >= animationTotalFrame) 
                owner.currentAnimationFrame = 1;

            owner.SetCanRotationFlag();
            owner.playerState.OffHpbarChenge();
        }
        //特定のアニメーションが再生されていないか確認
        private bool CanCurrentAnimation(AnimationId id)
        {
            //空中やられなら強制的にtrueに
            if (owner.playerState.currentTranstionState is PlayerTranstionState.Flying)
            {
                NextAnimationData = owner.controller.GetAttackData(AnimationId.Launching);
                return true;
            }

            bool isPlaying = id switch
            {
                AnimationId.Down => true,
                AnimationId.Thrown =>true,
                _=>false
            };

            return isPlaying;
        }
        protected override void OnUpdata()
        {
            if (currentAnimationData == null||isUsingAnimatorDeath) return;
            //アニメーション更新
            owner.controller.PlayAnimation(owner.currentAnimationFrame);

            //再生が終了したら流す
            if (owner.currentAnimationFrame >= animationTotalFrame)
            {
                //次に再生するアニメーションがあれば情報をセット
                if (NextAnimationData != null)
                    SetAnimationInfo();
                
                return;
            }
            //フレーム更新
            owner.currentAnimationFrame++;
            base.OnUpdata();
        }
        private void SetAnimationInfo()//アニメーション再生に必要なフレームの初期化
        {
            //トータルフレーム初期化
            animationTotalFrame = NextAnimationData.frameData[^1].FrameNumber-2;

            //現在のフレームを初期化
            owner.currentAnimationFrame = 1;

            //アニメーション情報をセット
            owner.controller.PlayAnimation(NextAnimationData.ActionName);
            NextAnimationData = null;
        }
        protected override void OnExit(State nextstate)//バトル最初の状態に初期化
        {
            //フラグを戻す
            isUsingAnimatorDeath = false;
            
            //データを外す
            currentAnimationData = null;
            NextAnimationData = null;

            SetNextRoundInfo();

            base.OnExit(nextstate);
        }
        private void SetNextRoundInfo()//次の試合に行くときの初期化処理
        {
            owner.CheckSpecialPlayerState();                              // 特殊な状態か確認
            owner.SetHitStopFlag();                                       // ヒットストップを解除
            owner.CanMoveState();                                         // 行動可能な状態に戻す
            owner.SetHitOrGuardFlag(PlayerHitDirector.HitResult.Hit, false); // ヒット・ガード状態を解除
            owner.controller.EndAnime();                                 // アニメーションを終了
            owner.playerState.OnResetComboCount();                       // コンボ情報をリセット
            owner.colliderManager.ResetColliderInfo();                   // 当たり判定を初期化
            owner.comandList.Clear();                                    // 入力コマンドをクリア
            owner.simpleMovement.ResetJumpFlag(false);                   // ジャンプのフラグを直す
        }
    }
    private class Hitting : State
    {

        private PlayerTranstionState currentState;
        private bool isGuarding;

        private TatuAttackData currentHitAttackData;
        private TatuAttackData currentAnimationData;

        private Queue<TatuAttackData> animationQueue;

        private int stiffnesesFrame;
        private int currentTotalAnimationFrame;
        private int currentHitStopFrame;
        protected override void OnEnter(State prevstate)
        {
            //初期化
            Init();
            Debug.Log(prevstate);

            SetStunFrameTime();
            //イベント発火
            InvokeEventAction();
        }
        private void Init()
        {
            currentState = owner.playerState.currentTranstionState;

            //ガード状態か確認
            isGuarding = currentState is
                PlayerTranstionState.CrouchGuard or PlayerTranstionState.StandGuard;

            InitState();

            if (owner.GetHitStopFlag()) owner.controller.StopClipAnimation();

            //受けた攻撃の情報を取得
            currentHitAttackData = owner.hitDirector.GetAttackData();

            //やられアニメーションの情報を取得
            currentAnimationData = owner.controller.GetCurrentAttackData();


            owner.textmanager.SetAttackDataInfo(currentHitAttackData);
            SetupAnimationQueue();

            InitFrame();

        }
        private void InitState()
        {
            //行動不能状態にする
            owner.ChangePlayerActionState(PlayerAction.Inoperable);
            owner.ChangeCanNextActionFlag();
            owner.ChangeIsActionFlag(true);
            owner.SetCanRotationFlag();
        }
        private void InitFrame()
        {
            //やられアニメーションの全体フレームを取得
            currentTotalAnimationFrame = currentAnimationData.frameData[^1].FrameNumber - 2;
            //現在のフレームを初期化
            owner.currentAnimationFrame = 1;
            currentHitStopFrame = 1;
        }
        private void SetupAnimationQueue()//リストの初期化
        {
            animationQueue ??= new();

            animationQueue.Clear();

            //再生リストにアニメーションクリップが入っていたらキューに保存
            foreach (var animation in currentAnimationData.AnimationQueue)
            {
                animationQueue.Enqueue(animation);
            }
        }
        private void SetStunFrameTime()
        {
            int normalFrame = currentHitAttackData.HitStunTime;

            if (isGuarding) normalFrame = currentHitAttackData.BlockStunTime;

            stiffnesesFrame = normalFrame;

        }
        private void InvokeEventAction()//動けない状態にする
        {
            owner.OnSetCanMoveFlagAction?.Invoke(false);
        }

        protected override void OnUpdata()
        {
            if (currentAnimationData == null) return;

            //ヒットストップフラグが経っていたら処理する
            if (owner.GetHitStopFlag())
            {
                UpdateHitStopFrame();
                return;
            }
            owner.textmanager.HitUpdateFrame(owner.currentAnimationFrame);
            //アニメーションを再生
            owner.controller.PlayAnimation(owner.currentAnimationFrame);

            //硬直
            UpdateStunTime();

            //フレームを更新
            owner.currentAnimationFrame++;

        }
        private void UpdateHitStopFrame()//ヒットストップの処理
        {
            int hitStopFrame = currentHitAttackData.HitStopFrame;

            if (isGuarding) hitStopFrame = currentHitAttackData.GuardStopFrame;

            if (currentHitStopFrame >= hitStopFrame)
            {
                if (owner.GetHitStopFlag()) owner.controller.StartClipAnimation();
                owner.SetHitStopFlag();
            }
            currentHitStopFrame++;
        }

        private void UpdateStunTime()
        {
            //動けるかどうかを確認
            if (CanMove())
            {
                //再生するアニメーションが合ったら再生
                if (animationQueue.Count <= 0) return;

                //終わりのフレームになったら０に戻してアニメーションを再生
                if (owner.currentAnimationFrame >= currentTotalAnimationFrame)
                {
                    SetAnimationInfo();//アニメーションを再セット
                }
                return;
            }
            //硬直が終わったら遷移前のステートに戻す
            if (owner.currentAnimationFrame >=  stiffnesesFrame)
            {
                owner.currentAnimationFrame = 1;

                //やられフラグを初期化
                owner.SetHitOrGuardFlag(PlayerHitDirector.HitResult.Guard,false);
                owner.SetHitOrGuardFlag(PlayerHitDirector.HitResult.Hit, false);

                owner.CanMoveState();
                //行動開始
                stateMachine.Dispatch((int)Event.MoveEvent);
            }
        }
        private void SetAnimationInfo()
        {
            var anima = animationQueue.Dequeue();

            //アニメーションの情報をセット
            owner.controller.PlayAnimation(anima.ActionName);
            owner.currentAnimationFrame = 1;
            currentTotalAnimationFrame = anima.frameData[^1].FrameNumber - 2;
        }
        private bool CanMove()//ここに条件を書く
        {
            return owner.playerState.currentTranstionState is PlayerTranstionState.Flying;
        }
        protected override void OnExit(State nextstate)
        {
            Debug.Log(nextstate);
            currentAnimationData = null;

            //ヒットかダウンステートに遷移するならアニメーションの初期化はしない
            if (ShouldSkipAnimationReset(stateMachine.NextState))
                return;

            EndHitState();

            ResetComboCount();
        }
        private void EndHitState()
        {
            owner.controller.EndAnime();// アニメーションを終了
            owner.controller.StopAnimation();
            owner.colliderManager.ResetColliderInfo();// 当たり判定を初期化
            owner.playerState.OffHpbarChenge();//Ui更新
        }
        //特殊なアニメーションかどうか確認
        private bool ShouldSkipAnimationReset(int nextState)
        {
            return nextState is
                (int)Event.IsHitEvent or
                (int)Event.InoperableEvent or
                (int)Event.DieEvnet;
        }
        private void ResetComboCount()
        {
            owner.playerState.OnResetComboCount();
        }
    }
    private class Win : State
    {
        private TatuAttackData currentAnimationData;
        private int winAnimationTotalFrame;
        protected override void OnEnter(State prevstate)
        {
            owner.playerState.ChangeState(PlayerTranstionState.Win);
            Delay.WaitTime(owner, 1, () =>
            {
                owner.controller.SetWinAnimation();
                InitFrame();
            });
           
        }
        private void InitFrame()
        {
            currentAnimationData = owner.controller.GetCurrentAttackData();

            winAnimationTotalFrame = currentAnimationData.frameData[^1].FrameNumber;
            owner.currentAnimationFrame = 1;
        }
        protected override void OnUpdata()
        {
            if (currentAnimationData == null) return;

            //特定のアニメーションまで再生したら止める
            if (owner.currentAnimationFrame >= winAnimationTotalFrame) return;

            //アニメーション更新
            owner.controller.PlayAnimation(owner.currentAnimationFrame);

            //フレーム更新
            owner.currentAnimationFrame++;
        }
        protected override void OnExit(State nextstate)//初期化処理
        {
            currentAnimationData = null;

            SetNextRoundInfo();

            base.OnExit(nextstate);
        }
        private void SetNextRoundInfo()//次の試合に行くときの初期化処理
        {
            owner.CheckSpecialPlayerState();// 特殊な状態か確認
            owner.SetHitStopFlag();// ヒットストップを解除
            owner.CanMoveState();// 行動可能な状態に戻す
            owner.controller.EndAnime();// アニメーションを終了
            owner.colliderManager.ResetColliderInfo();// 当たり判定を初期化
            owner.comandList.Clear(); // 入力コマンドをクリア
            owner.simpleMovement.ResetJumpFlag(false);// ジャンプのフラグを直す
        }
    }

    private class Inoperable:State
    {
        private TatuAttackData currentAnimationData;
        private Queue<TatuAttackData> animationQueue;

        private PlayerInputHandler.ActionState prevAction;//遷移前の行動
        private int animationTotalFrame;
        protected override void OnEnter(State prevstate)
        {
            prevAction = owner.currentActionState;
            //当たり判定のリストをクリアにする
            ClearCollider();

            //動けないようにする
            CantMove();

            //一フレーム待ってデータを取得
            Delay.OneFrame(owner,() =>
            {
                currentAnimationData = owner.controller.GetCurrentAttackData();

                //データがなかった時ずっとこのステートになってしまうから無かったらIdleにする
                if (currentAnimationData == null)
                {
                    stateMachine.Dispatch((int)Event.MoveEvent);
                    return;
                }
                
                owner.colliderManager.InitColliderInfo(currentAnimationData);

                InitFrame();
              
                SetupAnimationQueue();

            });
            InvokeEventAction();

        }
        private void InitFrame()
        {
            animationTotalFrame = currentAnimationData.frameData[^1].FrameNumber;
            owner.currentAnimationFrame = 1;
        }
        private void ClearCollider()//当たり判定をクリアにする
        {
            owner.colliderManager.StopCollider();
            owner.colliderManager.ClearHurtBox();
        }
        private void CantMove()
        {
            owner.ChangeIsActionFlag(true);
            owner.ChangeCanNextActionFlag();
        }
        private void InvokeEventAction()//イベントを発火
        {
            owner.OnActiveCollisionAction?.Invoke("", false);//当たり判定をなくす
            owner.OnUpdataPlayerStateAction?.Invoke(PlayerTranstionState.PlayerDown);//これを登録する
            owner.ChangePlayerActionState(PlayerAction.Inoperable);
        }
        private void SetupAnimationQueue()//リストの初期化
        {
            animationQueue ??= new();

            animationQueue.Clear();
            //再生リストにアニメーションクリップが入っていたらキューに保存
            foreach (var anima in currentAnimationData.AnimationQueue)
            {
                animationQueue.Enqueue(anima);
            }
        }

        protected override void OnUpdata()
        {
            if (currentAnimationData == null||owner.playerState.IsDead) return;

            //フレーム情報更新 
            owner.colliderManager.UpdateColliders(currentAnimationData, owner.currentAnimationFrame);

            //アニメーションを更新
            owner.controller.PlayAnimation(owner.currentAnimationFrame);

            CheckEndAnimation();

            //フレームを更新
            owner.currentAnimationFrame++;
        }
        private void CheckEndAnimation()
        {
            //アニメーションが再生終了したらステートを変更
            if (owner.currentAnimationFrame > animationTotalFrame)
            {
                if (animationQueue.Count > 0)
                {
                    PlayNextAnimation();

                    return;
                }
                //ステートを変更
                owner.CanMoveState();
                stateMachine.Dispatch((int)Event.MoveEvent);

            }
        }
        //アニメーション再生に必要な情報をセット
        private void PlayNextAnimation()
        {
            var anima = animationQueue.Dequeue();

            //アニメーションの情報をセット
            animationTotalFrame = anima.frameData[^1].FrameNumber;
            owner.controller.PlayAnimation(anima.ActionName);
            owner.currentAnimationFrame = 1;
        }
        protected override void OnExit(State nextstate)
        {
            currentAnimationData = null;

            //ステートをクリアにする
            ResetHitState();

            ResetComboCount();
            owner.RemoveComandList(prevAction);
            owner.playerState.OffHpbarChenge();//Uiに反映

            //特定のアニメーションだったらアニメーションを初期化しない
            if (stateMachine.NextState is (int)Event.DieEvnet) return;
            owner.controller.EndAnime();
        }
        private void ResetHitState()//状態をキレイにする
        {
            owner.SetHitOrGuardFlag(PlayerHitDirector.HitResult.Hit, false);//フラグを直す

            owner.simpleMovement.ResetJumpFlag(false);

            //投げられている状態がまだだったら消す
            if (owner.playerState.currentState.HasFlag(StatusFlags.IsThrowing))
            {
                owner.playerState.RemoveStatusFlag(StatusFlags.IsThrowing);
            }

        }
        private void ResetComboCount()//コンボカウントをリセット
        {
            owner.playerState.OnResetComboCount();
        }
    }
}

