using System;
using UnityEngine;
using UnityEngine.Windows;
using State = StateMachine<TatsuPlayerController>.State;

public class TatsuPlayerController : MonoBehaviour
{
    #region
    private TatsuAnimationController controller;
    private PlayerState playerState;
    private SimpleMovement simpleMovement;
    private PlayerInputHandler playerInputHandler;
    private TatuCollderManager collderManager;
    private ColliderManager colliderManagers;
    #endregion
    StateMachine<TatsuPlayerController> stateMachine;
    public bool IsHit = false;
    public bool IsGuarding = false;
    public bool IsThrown = false;
    public  bool Died = false;
    public  bool Is_Attack = false;
    public bool IsSpown = false;
    public bool IsJumping = false;
    public StepInfo stepInfo = new StepInfo();

    private float Gravity = 7.8f;
    private float VerticalSpeed = 0;
    private bool IsHitFlag = false;
    private bool IsGuardingFlag = false;
    [SerializeField] private float RayOffset;
    private enum Event:int {MoveEvent,JumpEvent,CrouchEvent,AttackEvent,DieEvnet,GuardEvent,StepEvent,IsHitEvent,WinEvent }

    //------event-------//
    public event Action OnRemovePos;
    void Init()
    {
        collderManager = GetComponent<TatuCollderManager>();
        simpleMovement = GetComponent<SimpleMovement>();
        controller = GetComponent<TatsuAnimationController>();
        playerState = GetComponent<PlayerState>();
        playerInputHandler = GetComponent<PlayerInputHandler>();
        colliderManagers = GetComponent<ColliderManager>();
        Died = false;
    }

    private void Start()
    {
        Init();
        stateMachine = new StateMachine<TatsuPlayerController>(this);
        stateMachine.AddTransition<MoveAndIdle, Jump>((int)Event.JumpEvent);
        stateMachine.AddTransition<MoveAndIdle, Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<MoveAndIdle, Guard>((int)Event.GuardEvent);
        stateMachine.AddTransition<MoveAndIdle, Attack>((int)Event.AttackEvent);
        stateMachine.AddTransition<MoveAndIdle, Die>((int)Event.DieEvnet);
        stateMachine.AddTransition<MoveAndIdle, Step>((int)Event.StepEvent);
        stateMachine.AddTransition<Guard, Attack>((int)Event.AttackEvent);
        stateMachine.AddTransition<Crouch, Attack>((int)Event.AttackEvent);
        stateMachine.AddTransition<Crouch, Guard>((int)Event.GuardEvent);
        stateMachine.AddTransition<Guard,Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<Guard, Jump>((int)Event.JumpEvent);
        stateMachine.AddTransition<Hitting, Crouch>((int)Event.CrouchEvent);
        stateMachine.AddTransition<Hitting, Guard>((int)Event.GuardEvent);
        stateMachine.AnyAddTrasition<MoveAndIdle>((int)Event.MoveEvent);
        stateMachine.AnyAddTrasition<Die>((int)Event.DieEvnet);
        stateMachine.AnyAddTrasition<Hitting>((int)Event.IsHitEvent);
        stateMachine.AnyAddTrasition<Win>((int)Event.WinEvent);
        stateMachine.Start<MoveAndIdle>();
    }
    private void Update()
    {
        var opponent = collderManager.GetNormaOpponent();
        if (opponent != null&&opponent.playerState.IsDead&&playerState.PState != PLayerState.Win)
        {
            stateMachine.Dispatch(((int)Event.WinEvent));
        }
        if(transform.position.y < -10)
        {
            OnRemovePos?.Invoke();
        }
        if (IsHit && !IsHitFlag)
        {
            IsHitFlag = true;
            stateMachine.Dispatch(((int)Event.IsHitEvent));
        }
        if (IsGuarding && !IsGuardingFlag)
        {
            IsGuardingFlag = true;
            stateMachine.Dispatch(((int)Event.IsHitEvent));
        }
        if(playerState.PState == PLayerState.Hitting)
        {
            stepInfo.StopStep();
        }
        transform.position += stepInfo.UpdataStep(transform.position);
        stateMachine.Updata();
        if (Died)
        {
            Died = false;
            stateMachine.Dispatch((int)Event.DieEvnet);
        }
        if (UnityEngine.Input.GetKeyDown(KeyCode.Tab))//Debag
        {
            Debug.Log(playerState.PState);
            Debug.Log(Time.timeScale);
            Debug.Log(stateMachine.CurrentState);
        }

        RaycastHit hit;
        if(Physics.Raycast(transform.position,Vector3.down,out hit, RayOffset))
        {
            VerticalSpeed = 0;
            transform.position = new Vector3(transform.position.x, hit.point.y + RayOffset, transform.position.z);
        }
        else
        {
            VerticalSpeed -= Gravity * Time.deltaTime;
            transform.position += new Vector3(0, VerticalSpeed * Time.deltaTime, 0);
        }
        Debug.DrawRay(transform.position, Vector3.down * RayOffset, Color.red);
    }
    private void FixedUpdate()
    {
        stateMachine.FixedUpdates();
        
    }

    private bool PlayerCheckDirection()
    {
        Vector2 input = playerInputHandler.MoveInput;
        int facingDir = transform.forward.x > 0 ? 1 : -1;
        bool isBackInput = (Mathf.RoundToInt(input.x) == -facingDir && input.x != 0);
        bool isDownInput = (input.y < -0.5f);

        if (isBackInput && isDownInput) return true;
        if (isBackInput) return true;

        return false;
    }
    private int GetDirection()
    {
        float x = transform.forward.x;

        // しきい値を決めて「ニュートラル」とみなす
        if (Mathf.Abs(x) < 0.01f)
            return 0;   // ニュートラル

        return x > 0 ? 1 : -1;
    }
    public void AttackReset() { controller.AttackOff(); playerState.ChengeState(PLayerState.Nomal); }
    private class MoveAndIdle : State
    {
        private float F_lastDir = 0;
        private bool OnAction = false;
        State CurrentState = null;

        protected override void OnEnter(State prevstate)
        {
            owner.playerState.OffHpbarChenge();
            owner.collderManager.UpdateCollider("Idle");
            owner.playerState.ChengeState(PLayerState.Nomal);
            owner.colliderManagers.ThrowActive(true);
            OnAction = false;
            CurrentState = prevstate;
         if(!owner.playerInputHandler.IsCrouching&&owner.IsSpown) owner.controller.OffCrouch();
        }
        protected override void OnUpdata()
        {
            var Inputx = owner.playerInputHandler.MoveInput.x;
            if (OnAction||owner.stepInfo.IsSteping) return;
           if(owner.IsSpown) owner.controller.MoveInput(Inputx);
            var dirs = Mathf.RoundToInt(Inputx);
            if (owner.playerInputHandler.CheckAttacking() && owner.PlayerCheckDirection())
            {
                OnAction = true;
                stateMachine.Dispatch((int)Event.AttackEvent);
                return;
            }
            if (F_lastDir == 0&& dirs != 0)
            {
                if (!owner.stepInfo.IsSteping&&owner.playerState.PState != PLayerState.Graping)
                {
                    if (owner.stepInfo.CheckStep(dirs,owner.transform.position,owner.PlayerCheckDirection()))
                    {
                        Debug.Log("Step");
                        owner.stateMachine.Dispatch((int)Event.StepEvent);
                    }
                }
            }
            else if (owner.playerInputHandler.Is_Jumping && !owner.controller.Attacking&&owner.playerState.PState != PLayerState.Graping)
            {
                OnAction = true;
                owner.playerState.ChengeState(PLayerState.Jump);
                stateMachine.Dispatch((int)Event.JumpEvent);
                return;
            }
            else if (owner.playerInputHandler.CheckAttacking())
            {
                OnAction = true;
                stateMachine.Dispatch((int)Event.AttackEvent);
                return;
            }
            else if (owner.PlayerCheckDirection()&&!owner.Is_Attack)
            {
                OnAction = true;
                stateMachine.Dispatch((int)Event.GuardEvent);
                return;
            }

            else if (owner.playerInputHandler.IsCrouching)
            {
                OnAction = true;
                stateMachine.Dispatch((int)Event.CrouchEvent);
                return;
            }
            F_lastDir = Inputx;
        }

        protected override void FixedUpdate()
        {
            // owner.simpleMovement.Jumping();
            owner.simpleMovement.Moving();
        }
        protected override void OnExit(State nextstate)
        {
            CurrentState = null;
        }
    }
    private class Jump : State
    {
      //  private bool IsFlyAttack = false;
        protected override void OnEnter(State prevstate)
        {
            if (owner.Is_Attack) return;
            owner.collderManager.UpdateCollider("Jump");
            owner.colliderManagers.ThrowActive(false);
            owner.IsJumping = true;
            Debug.Log("JumpNow何もできない");
            var Movex = owner.playerInputHandler.MoveInput.x;
            var dir = Mathf.RoundToInt(Movex);
            owner.simpleMovement.Jump(dir);
            owner.simpleMovement.RotationPlayer();

        }
        protected override void OnUpdata()
        {
 
            owner.simpleMovement.Jumping();
            if (!owner.simpleMovement.isJumping)
            {
                stateMachine.Dispatch((int)Event.MoveEvent);
            }


        }
        protected override void OnExit(State nextstate)
        {
            Debug.Log("Jump終わり");
            if (owner.playerState.PState != PLayerState.Down&&owner.playerState.PState != PLayerState.Flying)
            {
                Debug.Log("ジャンプ終わり");
                owner.controller.AttackOff();
                owner.controller.StopAnimetion();
            }
            owner.simpleMovement.ResetIsJump();
            owner.playerInputHandler.ResetJump();
        }
    }
    private class Crouch : State
    {
        float firstDir = 0f;
        PLayerState laststate;
        protected override void OnEnter(State prevstate)
        {
            laststate = PLayerState.Nomal;
            firstDir = owner.GetDirection();
            Debug.Log("しゃがんでる");owner.controller.OnCrouch();
            owner.collderManager.UpdateCollider("Crouch");
            
        }
        protected override void OnUpdata()
        {

            owner.simpleMovement.OffMove();

            // 攻撃判定
            if (owner.playerInputHandler.CheckAttacking())
            {
                stateMachine.Dispatch((int)Event.AttackEvent);
                return;
            }

            // しゃがみ解除
            if (!owner.playerInputHandler.IsCrouching)
            {
                owner.stateMachine.Dispatch((int)Event.MoveEvent);
                return;
            }

            // ===== ガード判定 =====
            if (owner.PlayerCheckDirection() && !owner.Is_Attack)
            {
                // 入力方向が逆 → しゃがみガード
                owner.playerState.ChengeState(PLayerState.Guard);
            }
            else
            {
                // 入力が逆じゃない → 純粋にしゃがみ
                owner.playerState.ChengeState(PLayerState.Crouch);
            }
            var dir = owner.GetDirection();
            if (dir != 0)
            {
                if (dir != firstDir)
                {
                    if (dir == -1) return;
                    if (owner.playerState.PState == laststate) return;
                    Debug.Log("ssssss");
                    if (owner.playerInputHandler.IsCrouching) owner.playerState.ChengeState(PLayerState.Crouch);
                    else owner.playerState.ChengeState(PLayerState.Nomal);
                }
            }
            laststate = owner.playerState.PState;
        }
        protected override void OnExit(State nextstate)
        {
          if(!owner.controller.Attacked)  owner.controller.OffCrouch();
            owner.simpleMovement.OnMove();
        }
    }
    private class Guard : State
    {
        float firstDir = 0f;
        protected override void OnEnter(State prevstate)
        {
            firstDir = owner.GetDirection();
            Debug.Log("ガード中");
          if(!owner.playerInputHandler.IsCrouching)  owner.playerState.ChengeState(PLayerState.StandGuard);
            else owner.playerState.ChengeState(PLayerState.Guard);
        }

        protected override void OnUpdata()
        {
            if (owner.playerInputHandler.CheckAttacking())
            {
                stateMachine.Dispatch((int)Event.AttackEvent);
                return;
            }
            var dir = owner.GetDirection();
                if (dir != firstDir)
                {
                    Debug.Log("ガード方向が変わったので解除");
                    if (owner.playerInputHandler.IsCrouching)
                        stateMachine.Dispatch((int)Event.CrouchEvent);
                    else
                        stateMachine.Dispatch((int)Event.MoveEvent);
                    return;
                }
            if (owner.playerInputHandler.Is_Jumping)
            {
                stateMachine.Dispatch((int)Event.JumpEvent);
                return;
            }
            // ガード維持条件：ガードボタン + 逆方向入力
            bool isStillGuarding = owner.PlayerCheckDirection();

            if (!isStillGuarding)
            {
                // ガード解除 → Move or Crouchに戻す
                if (owner.playerInputHandler.IsCrouching)
                    stateMachine.Dispatch((int)Event.CrouchEvent);
                else
                    stateMachine.Dispatch((int)Event.MoveEvent);
            }
          else if (owner.playerInputHandler.IsCrouching)
                stateMachine.Dispatch((int)Event.CrouchEvent);

        }

        protected override void FixedUpdate()
        {
            owner.simpleMovement.Moving();
        }

        protected override void OnExit(State nextstate)
        {
            owner.playerState.ChengeState(PLayerState.Nomal);
            Debug.Log("ガード解除");
        }
    }
    private class Attack : State
    {
        private bool attackStarted = false;

        protected override void OnEnter(State prevstate)
        {

            Debug.Log("Attacksita");
            owner.simpleMovement.Is_Attaking = true;
            owner.playerInputHandler.IsSmashing = false;
            owner.simpleMovement.OffMove();
            attackStarted = false;
            owner.Is_Attack = true;
            owner.simpleMovement.RotationPlayer();
        }

        protected override void OnUpdata()
        {
            if (owner.IsHit)
            {
                owner.Is_Attack = false;
                stateMachine.Dispatch((int)Event.IsHitEvent);
                return;
            }
            if (!attackStarted)
            {
                if (owner.controller.Attacked)
                {
                    attackStarted = true;
                }
            }
            else
            {
                if (!owner.controller.Attacked)
                {
                    owner.Is_Attack =false;
                    stateMachine.Dispatch((int)Event.MoveEvent);
                }
            }
        }

        protected override void OnExit(State nextstate)
        {
            if (owner.playerState.PState == PLayerState.Jump) owner.playerState.ChengeState(PLayerState.Flying);
            if (!owner.playerInputHandler.IsCrouching) owner.controller.OffCrouch();
            Debug.Log("Attackowari");
            attackStarted = false;
            owner.simpleMovement.Is_Attaking = false;
            owner.simpleMovement.OnMove();
        }
    }
    private class Step: State
    {
        protected override void OnEnter(State prevstate)
        {
            owner.controller.StepAnime(owner.stepInfo.IsBack);
            owner.colliderManagers.ThrowActive(false);
        }
        protected override void OnUpdata()
        {
            if (!owner.stepInfo.IsSteping) owner.stateMachine.Dispatch((int)Event.MoveEvent);
        }
        protected override void OnExit(State nextstate)
        {
            owner.colliderManagers.ThrowActive(true);
        }
    }
    private class Die : State
    {
        protected override void OnEnter(State prevstate)
        {
            owner.playerState.OffHpbarChenge();
            Debug.Log("sinnda");
            owner.controller.Die();
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
    private class Hitting : State
    {
        private bool IsFirst = false;
        private bool IsGuard = false;
        private bool IsHit = false;
        protected override void OnEnter(State prevstate)
        {
            Debug.Log($"Hit now{owner.IsHit}/{owner.IsGuarding}");
            IsFirst = false;
            IsGuard = false;
            IsHit = false;
            if (owner.IsHit) IsHit = true;
            if (owner.IsGuarding)
            {
                IsGuard = true;
                if (owner.playerInputHandler.IsCrouching)
                {
                    owner.controller.OnCrouch();
                }
            }
        }
        protected override void OnUpdata()
        {
            owner.simpleMovement.OffMove();
            if (!owner.IsHit && !IsFirst&&IsHit)
            {
                Debug.Log("Hit End");
                IsFirst = true;
                if (owner.playerInputHandler.IsGuarding)
                {
                    owner.controller.OffCrouch();
                    stateMachine.Dispatch(((int)Event.GuardEvent));return;
                }
                if (owner.playerInputHandler.IsCrouching)
                {
                    stateMachine.Dispatch(((int)Event.CrouchEvent));return;
                }
                owner.controller.OffCrouch();
                stateMachine.Dispatch(((int)Event.MoveEvent));
            }
            if (!owner.IsGuarding && !IsFirst&&IsGuard)
            {
                Debug.Log("Guard End");
                IsFirst = true;
                if (owner.playerInputHandler.IsGuarding)
                {
                    owner.controller.OffCrouch();
                    stateMachine.Dispatch(((int)Event.GuardEvent)); return;
                }
                if (owner.playerInputHandler.IsCrouching)
                {
                    stateMachine.Dispatch(((int)Event.CrouchEvent));
                }
                else
                {
                    owner.controller.OffCrouch();
                    stateMachine.Dispatch(((int)Event.MoveEvent));
                }
            }
            
        }
        protected override void OnExit(State nextstate)
        {
            Debug.Log("Hited");
            owner.IsHitFlag = false;
            owner.Is_Attack = false;
            owner.IsGuardingFlag = false;
            owner.simpleMovement.OnMove();
        }
    }
    private class Win : State
    {
        private float timer = 0.7f;
        private float eplased = 0;
        private bool IsWinFlag = false;
        protected override void OnEnter(State prevstate)
        {
            owner.playerState.ChengeState(PLayerState.Win);
            IsWinFlag = false;
        }
        protected override void OnUpdata()
        {
            eplased += Time.deltaTime;
            if(eplased > timer&&!IsWinFlag)
            {
                eplased = 0;
                IsWinFlag = true;
                owner.controller.WinAnime();
            }
        }
        protected override void OnExit(State nextstate)
        {
            base.OnExit(nextstate);
        }
    }
}

[Serializable] 
public class StepInfo
{
    public float StepDistance = 2;
    public float StepDuration = 0.2f;
    public float doubleTapTime = 0.25f;
    public bool IsSteping => isStep;
    public bool IsBack => isBack;
    private int lastdir = 0;
    private float Lasttap =0;
    private float timer = 0;
    private bool isStep = false;
    private bool isBack = false;
    private Vector3 StartStepPos;
    private Vector3 TargetStepPos;
    private float BackStepDistance;

    public bool CheckStep(int dir,Vector3 CurrentPos,bool Dir)
    {
        float BackStepDistances= 0.5f;
        float FowerdStepDistance = 0.8f;
        float now = Time.time;
        if (dir != lastdir)lastdir = 0;
        if(dir == lastdir && now - Lasttap < doubleTapTime)
        {
            isBack = Dir ? true : false;
           BackStepDistance = Dir ? BackStepDistances:FowerdStepDistance;
            StartStep(dir,CurrentPos,BackStepDistance);
            return true;
        }
        Lasttap = now;
        lastdir = dir;
        return false;
    }
    public Vector3 UpdataStep(Vector3 Current)
    {
        if (!isStep) return Vector3.zero;
        timer += Time.deltaTime;
        var t = timer / StepDuration;
        var newPos = Vector3.Lerp(StartStepPos, TargetStepPos, t);
        if (t >= 1) isStep = false;
        return newPos - Current;
    }
    private void StartStep(int dir,Vector3 CurrentPos,float Pldir)
    {
        isStep = true;
        timer = 0;
        StartStepPos = CurrentPos;
        TargetStepPos = CurrentPos + new Vector3(dir * StepDistance*Pldir, 0, 0);
    }
    public void StopStep()
    {
       if(isStep) isStep = false;
    }
}
