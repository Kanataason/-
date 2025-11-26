using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.VFX;
public class TatsuAnimationController : MonoBehaviour
{
    private static readonly int Is_Attack = Animator.StringToHash("IsAttack");
    private static readonly int Is_Dead = Animator.StringToHash("Dead");
    private static readonly int Is_Damage = Animator.StringToHash("Hit");
    private static readonly int Is_Jumping = Animator.StringToHash("Jumping");
    private static readonly int F_MoveInput = Animator.StringToHash("MoveInput");
    private static readonly int I_AttackType = Animator.StringToHash("AttackType");
    private static readonly int Is_Crouch = Animator.StringToHash("IsCrouch");

    private Animator animator;
    private TatuCollderManager manager;
    public MakePlayable playable;
    private ColliderManager colliderManager;
    private TatsuPlayerController controller;
    private PlayerObjctPool playerObjctPool;
    private KnockBackMotion knockBack;
    private SimpleMovement simple;
    public PlayerInputHandler playerInputHandler;
    PlayerState playerState;
    public bool Attacking = false;
    public bool Throwing = false;
    public bool IsGraping = false;
    public bool Attacked = false;
    public bool CanNextCombo = false;
    public bool IsOnPushing = false;
    public bool IsDown = false;
    public bool IsSmash = false;
    public bool IsCancel = false;

    public Transform targetHead; // 相手の頭（Headボーン）
    public Transform handAttachPoint; // 自分の右手の掴み位置（空のオブジェクトでOK）
    public Transform LeftLeg;
    public Transform RightLeg;
    public Transform TargetRoot;
    public SkinnedMeshRenderer Body; 
    private bool isGraping = false;

    private GameObject BufferEfect;

    public event System.Action<Transform> OnCameraZoom;

    [System.Serializable]
    public class AttackInfo
    {
        public string StateName;    
    }
    [SerializeField] private List<AttackInfo> attackinfo = new List<AttackInfo>();

    public List<TatuAttackData> DataAttack = new List<TatuAttackData>();
    public List<AnimationClip> ClipList = new List<AnimationClip>();

    void Start()
    {
        simple = GetComponent<SimpleMovement>();
        knockBack = GetComponent<KnockBackMotion>();
        playerObjctPool = GetComponent<PlayerObjctPool>();
        manager = GetComponent<TatuCollderManager>();
        playerState = GetComponent<PlayerState>();
        animator = GetComponent<Animator>();
        playerInputHandler = GetComponent<PlayerInputHandler>();
        controller = GetComponent<TatsuPlayerController>();
        colliderManager = GetComponent<ColliderManager>();
    }

    public void MoveInput(float MoIn)
    {
        animator.SetFloat(F_MoveInput, MoIn, 0.03f, Time.deltaTime);
    }
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.Backspace)) playerState.AddGauge(30000f);
        if (playerInputHandler == null||animator == null||controller.stepInfo.IsSteping||!controller.IsSpown||
            controller.IsHit||controller.IsGuarding||IsGraping|| playerState.PState == PLayerState.Hitting|| CameraFollowTwoTargets.IsZooming) return;
        if (playerState.PState == PLayerState.Jump)
        {
            if (playerInputHandler.CheckAttacking())
            {
                if (!Attacking)
                {

                    var data = GetAttackData("JumpAttack");
                    manager.SetAttack(data);
                    playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber,data.Mask);
                    Attacking = true;
                    Attacked = true;
                }
            }
            return;
        }
        ///自分が攻撃してない、自分に攻撃が当たっていない
        /// 自分が投げられてない、自分がまだ投げていない
        ///投げ　成立
        ///

        if (playerInputHandler.IsGraping)
        {
            if (playerState.PState == PLayerState.Graping)
            {
                var opponent = manager.GetOpponent();
                opponent.colliderManager.Graped = true;
                return;
            }
            if (!Throwing&&!controller.IsHit&&playerState.PState != PLayerState.Graping && !Attacking)
            {
                Attacking = true;
                Attacked = true;
                Throwing = true;
                var data = GetAttackData("Throw");
                playerState.ChengeState(PLayerState.Counter);
                manager.SetAttack(data);
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
            }
                ///test
            //if (playerState.PState == PLayerState.Graping||playerState.PState == PLayerState.Hitting)
            //{
            //    Attacking = true;
            //    colliderManager.Graped = true;
            //    var opponent = manager.GetOpponent();
            //    opponent.colliderManager.Graped = true;
            //    return;
            //}
            //if (Attacking) return;
            //// 投げ開始条件（掴んでいない、相手も掴んでいない、相手も投げていない）
            //if (!Throwing&&playerState.PState != PLayerState.Graping)
            //{
            //    Throwing = true;
            //    Debug.Log("Throwingg");
            //    var data = GetAttackData("Throw");
            //    manager.SetAttack(data);
            //    playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
            //    Attacked = true;
            //    Attacking = true;
            //}
        }
        if (playerInputHandler.Is_WeakAttack)
        {
            if (!Attacking)
            {
                var data = GetAttackData("Panch");
                manager.SetAttack(data);
                playable.Play(data.clip, data.frameData[data.frameData.Count -1].FrameNumber);
                playerState.PState = PLayerState.Counter;
                Attacking = true;
                Attacked = true;
            }
            else if(CanNextCombo)
            {
                playerState.PState = PLayerState.BegingAttacked;
                CanNextCombo = false;
            }
        }
        if (playerInputHandler.Is_MidlleAttack)
        {
            if (!Attacking)
            {
                Debug.Log("Midlle");
                var data = GetAttackData("MidlleAttack");
                manager.SetAttack(data);
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                playerState.PState = PLayerState.Counter;
                Attacking = true;
                Attacked = true;
            }
            else if (CanNextCombo)
            {
                //manager.UpdateCollider("");
                //var data = GetAttackData("StrongAttack");
                //manager.SetAttack(data);
                //playable.TransitionTo(data.clip, data.frameData[^1].FrameNumber, 1f);
                //playerState.PState = PLayerState.Counter;
                CanNextCombo = false;
            }
        }
        if (playerInputHandler.Is_StrongAttack)
        {
            if (!Attacking)
            {
                if (playerInputHandler.IsCrouching)
                {
                    var data = GetAttackData("CrouchKick");
                    manager.SetAttack(data);
                    playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber, data.Mask);
                }
                else
                {
                    var data = GetAttackData("StrongAttack");
                    manager.SetAttack(data);
                    playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                }
                playerState.PState = PLayerState.Counter;
                Attacking = true;
                Attacked = true;
            }
            else if (CanNextCombo)
            {
                playerState.PState = PLayerState.BegingAttacked;
                CanNextCombo = false;
            }
        }
        if (playerInputHandler.Is_SpecialAttack)
        {
            if (!Attacking)
            {
                if (playerInputHandler.IsCrouching)
                {
                    var data = GetAttackData("CrouchUpper");
                    manager.SetAttack(data);
                    playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber, data.Mask);
                }
                else
                {
                    //manager.UpdateCollider("");
                    var data = GetAttackData("SpecialAttack");
                    manager.SetAttack(data);
                    playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                }
                playerState.PState = PLayerState.Counter;
                Attacking = true;
                Attacked = true;
            }
            else if (CanNextCombo)
            {
                playerState.PState = PLayerState.BegingAttacked;
                CanNextCombo = false;
            }
        }
        if (playerInputHandler.IsTiltAttack)
        {
            if (!Attacking)
            {
                var data = GetAttackData("TiltAttack");
                manager.SetAttack(data);
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                playerState.PState = PLayerState.Counter;
                Attacking = true;
                Attacked = true;
                var info = data.knockInfos[0];
                var opponent = manager.GetOpponent();
                var dir = opponent.transform.position.x > transform.position.x ? 1 : -1;
                knockBack.OnMoveStraight(Vector3.right * dir,info.Power,info.Duration);
            }
            else if (CanNextCombo)
            {
                playerState.PState = PLayerState.BegingAttacked;    
                CanNextCombo = false;
            }
        }
        if (playerInputHandler.IsSmashAttack)
        {
            if (Attacking) return;
            Attacking = true;
            if (!playerState.CheckSmash(25000))
            {
                Attacking = false;
                Debug.Log("足りない");
                playerInputHandler.ResetSmash();
                return;
            }
            playerInputHandler.IsSmashing = true;
            playerState.ChengeState(PLayerState.Down);
            manager.UpdateCollider("");
            colliderManager.ThrowActive(false);
            playerInputHandler.ResetSmash();
            var efe = playerObjctPool.GetEffect(EffectType.Charge);
            efe.transform.position = new Vector3(transform.position.x, transform.position.y + 1, transform.position.z);
            var ps = efe.GetComponentInChildren<ParticleSystem>();
            var main = ps.main;
            main.useUnscaledTime = true;
            StartCoroutine(EfectReturnTimer(efe,0.5f,EffectType.Charge));
            OnCameraZoom?.Invoke(this.transform);
            Debug.Log("足りた");
          
        }
        if (IsSmash)
        {
            playerState.UseGauge(25000);
            IsSmash = false;
            playerInputHandler.ResetSmash();

            Attacked = true;
            var BufferData = GetAttackData("Smash");
                Debug.Log(BufferData);
                manager.SetAttack(BufferData);
                playable.Play(BufferData.clip, BufferData.frameData[^1].FrameNumber, BufferData.Mask);
                // if (BufferData == null) BufferData = GetAttackData("");
                Debug.Log("Smash");
            
        }
    }
    private void LateUpdate()
    {
        playerInputHandler.ResetAttackReset();
    }
    public AnimationClip GetAnimationClip(string Name)
    {
        foreach(var clip in ClipList)
        {
            if(clip.name == Name)
            {
                return clip;
            }
        }
        return null;
    }
    public TatuAttackData GetAttackData(string Name)
    {
        foreach(var data in DataAttack)
        {
            if(data.AttackName == Name)
            {
                return data;
            }
        }
        return null;
    }

    private void RootMotionFlag(bool IsMotion) => animator.applyRootMotion = IsMotion;
    public void OnJump()
    {
        var data = GetAttackData("Jump");
        playable.Play(data.clip,data.frameData[data.frameData.Count -1].FrameNumber);
    }
    public void OnCrouch()
    {
        animator.SetBool(Is_Crouch, true);
    }
    public void OffCrouch()
    {
        animator.SetBool(Is_Crouch, false);
    }
    public void StepAnime(bool IsBack)
    {
        var vfx = playerObjctPool.GetEffect(EffectType.Step);
        vfx.transform.position = LeftLeg.position;
        vfx.transform.localRotation = Quaternion.identity;
        StartCoroutine(EfectReturnTimer(vfx, 0.2f, EffectType.Step));
        var vfxs = playerObjctPool.GetEffect(EffectType.Step);
        vfxs.transform.position = RightLeg.position;
        vfxs.transform.localRotation = Quaternion.identity;
        StartCoroutine(EfectReturnTimer(vfxs, 0.2f, EffectType.Step));
        if (IsBack) return;
        var data = GetAttackData("Step");
        playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
    }
    public void ThrowAnime(string stateName,ColliderManager target)
    {
        foreach(var info in DataAttack)
        {
            if (stateName == "Grap" && info.AttackName == "Grap")
            {
                playerState.ChengeState(PLayerState.Hitting);
                Debug.Log("Grap");
                var data = GetAttackData("Grap");
                manager.StopCollider();
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                return;
            }
             if (stateName == "Throwing" && info.AttackName == "Throwing"&&!IsCheckHitting(playerState))
            {
                Debug.Log("Throwing");
                var data = GetAttackData("Throwing");
                //manager.StopCollider()
                manager.SetAttack(data);
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                return;
            }
             if(stateName == "Thrown" && info.AttackName == "Thrown"&&!IsCheckHitting(target.state))
            {
                manager.StopCollider();
                playerState.ChengeState(PLayerState.Down);
                Debug.Log("Thrown");
                IsDown = true;
                var data = GetAttackData("Thrown");
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                return;
            }
        }
    }
    public void Hit(Vector3 point,string stateName,ColliderManager target =null)
    {
        foreach(var info in attackinfo)
        {
            if (stateName == "Hit" && info.StateName == "Hit")
            {
                UIAudioSound.Instance.HitSe();
                if (target.state.IsPowerUp)
                {
                    Debug.Log("PowerUpEfect");
                    var vfx = playerObjctPool.GetEffect(EffectType.PowerUp);
                    vfx.transform.position = point;
                    vfx.transform.localRotation = Quaternion.identity;
                    StartCoroutine(EfectReturnTimer(vfx, 0.5f, EffectType.PowerUp));
                }
                else
                {
                    Debug.Log("NormalEfect");
                    var vfx = playerObjctPool.GetEffect(EffectType.Hit);
                    vfx.transform.position = point;
                    vfx.transform.localRotation = Quaternion.identity;
                    StartCoroutine(EfectReturnTimer(vfx, 0.5f, EffectType.Hit));
                }
                if (playerState.PState == PLayerState.Flying||playerState.PState == PLayerState.Jump) return;
                Debug.Log("Hit effect");
                var data = GetAttackData("Hit");
                manager.StopCollider();
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                return;
            }
            else if (stateName == "Guard" && info.StateName == "Guard")
            {
                var data = GetAttackData("Guard");
                AvatarMask mask =null;
                Debug.Log("Guard effect");
                UIAudioSound.Instance.GuardSe();
                var vfx = playerObjctPool.GetEffect(EffectType.Guard);
                vfx.transform.position = point;
                vfx.transform.localRotation = Quaternion.identity;
                StartCoroutine(EfectReturnTimer(vfx, 0.5f, EffectType.Guard));
                if (playerInputHandler.IsCrouching) mask = data.Mask; 
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber,mask);
                return;
            }
        }
        playerInputHandler.ResetAttackReset();
    }
    public void LunchAnimes(string stateName)
    {
        foreach (var anime in DataAttack)
        {
            if (stateName == "Lunch" && anime.AttackName == "Lunch")
            {
               if(playerState.PState == PLayerState.Flying) manager.UpdateCollider("Fly");
               else manager.UpdateCollider("");
                colliderManager.ThrowActive(false);
                var data = GetAttackData("Lunch");
              //  manager.StopCollider();
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                return;
            }
            if (stateName == "Lunching" && anime.AttackName == "Lunching")
            {
                playerState.ChengeState(PLayerState.Down);
                manager.UpdateCollider("");
                colliderManager.ThrowActive(false);
                var data = GetAttackData("Lunching");
                //manager.StopCollider();
                playable.StopManual();
                playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
                return;
            }
        }
    }
    public void WinAnime()
    {
        var data = GetAttackData("Win");
        playable.Play(data.clip, data.frameData[^1].FrameNumber);
    }
    public void GunBulletAnimePlay()
    {
        var vfx = playerObjctPool.GetEffect(EffectType.Smash);
        var bul = vfx.GetComponent<BulletMovement>();
        bul.ResetList();
        bul.SetInfo(8, transform.forward, playerState);
        vfx.transform.position = handAttachPoint.position + transform.forward;
        vfx.transform.localRotation = handAttachPoint.rotation;
        UIAudioSound.Instance.RobotSA();
        StartCoroutine(EfectReturnTimer(vfx, 1f, EffectType.Smash));
    }
    public void DownAnimation()
    {
        playerState.ChengeState(PLayerState.Down);
        manager.StopCollider();
        IsDown = true;
        manager.UpdateCollider("");
        var data = GetAttackData("Down");
        playable.StopManual();
        playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
    }

    IEnumerator EfectReturnTimer(GameObject efe,float Time,EffectType type)
    {
        if (IsCancel&&controller.IsHit)
        {
            Debug.Log("Returnnefect");
            IsCancel = false;
          playerObjctPool.ReturnEffect(type, efe);
          playerState.IsPowerUp = false;
            PlayerSmashEfect();
            yield break;
        }
        yield return new WaitForSeconds(Time);
        playerObjctPool.ReturnEffect(type,efe);
    }
    public void Die()
    {
        if (playerState.PState == PLayerState.Flying)
        {
            Debug.Log("Dieaaaa");
            animator.SetInteger("DeadType", 1);
        }
        else animator.SetInteger("DeadType", 0);
        animator.SetBool(Is_Dead, true);
    }
    public void Flying()
    {
        var data = GetAttackData("Fly");
        playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
    }
    //----------------AnimatorEvent Call-------------//
    public void CanNextComboOnFlag()=> CanNextCombo = true;
    public void CanNextComboOffFlag() => CanNextCombo = false;

    public void PlayerSmashEfect()
    {
        playerState.ChengeState(PLayerState.Nomal);
        manager.UpdateCollider("Idle");
        bool IsDead = playerState.IsPowerUp ? true : false;
        if (!IsDead)
        {
            playerState.correction = 0.2f;
            var mates = Body.materials;
            playerObjctPool.ReturnEffect(EffectType.Smash, BufferEfect);
            foreach (var mt in mates)
            {
                Debug.Log("Material Normal");   
                    mt.SetFloat("_offset", 0f);             
            }
            BufferEfect = null;
            return;
        }
        playerState.correction = 0.4f;
        UIAudioSound.Instance.HadakaSA();
        var efe = playerObjctPool.GetEffect(EffectType.Smash);
            efe.transform.position = transform.position;
            var vis = efe.GetComponent<VisualEffect>();
            vis.SetBool("IsDead", IsDead);
            vis.SetSkinnedMeshRenderer("Charactor", Body);
        if (BufferEfect != null) playerObjctPool.ReturnEffect(EffectType.Smash, BufferEfect);
            BufferEfect = efe;
            var mate = Body.materials;
        
        foreach(var mt in mate)
        {
            if (IsDead)
            {
                Debug.Log("Material PowerUp");
                mt.SetFloat("_offset", 0.2f);
            }
            else
            {
                Debug.Log("Material Normal");
                mt.SetFloat("_offset", 0f);
            }
        }
    }
    public void PlayGetUp()
    {
        if (playerState.NowHp <= 0) return;
        playerState.ChengeState(PLayerState.Down);
        var data = GetAttackData("GetUp");
        playable.StopManual();
        playable.Play(data.clip, data.frameData[data.frameData.Count - 1].FrameNumber);
        playable.IsThrown = true;
    }
    public void AttackOff()
    {
       // Debug.Log($"AttackOff/{gameObject.name}");
        Attacking = false;
        //animator.SetInteger(I_AttackType, 0);
        if (playerState.PState != PLayerState.Down) playerState.ChengeState(PLayerState.Nomal);
        Attacked = false;
        Throwing = false;
        IsDown = false;
    }
    public void OffThrowing() => Throwing = false;
    public void MoveOn() => Debug.Log("s");
    public void CrossFadeEvent() => animator.CrossFade("Locomtion", 0);

    public void OffJump() => playerInputHandler.ResetJump();
    public void OnIsCancel(bool IsCancesl) { IsCancel = IsCancesl; }
    public bool IsCheckHitting(PlayerState state) { return state.PState == PLayerState.Hitting || state.PState == PLayerState.Flying; }
    //-----------------anima--------------//
    public void StartGraping(Transform head,Transform root)
    {
        Debug.Log("HeadTelepo");
        targetHead = head;
        TargetRoot = root;
        isGraping = true;
        SnapOpponentInFront();
    }

    public void StopGraping()
    {
        isGraping = false;
    }
    public void StopAnimetion()
    {
        Debug.Log("s");
        StopGraping();
        playable.EndManual();
        manager.ResetAttackOff();
    }

    void OnAnimatorIK(int layerIndex)
    {
        if (animator == null) return;
        if (isGraping && targetHead != null)
        {
            Debug.Log("OnAnimation");
            // 自分の右手を掴み位置に固定
            animator.SetIKPositionWeight(AvatarIKGoal.RightHand, 1f);
            animator.SetIKRotationWeight(AvatarIKGoal.RightHand, 1f);
            animator.SetIKPosition(AvatarIKGoal.RightHand, handAttachPoint.position);
            animator.SetIKRotation(AvatarIKGoal.RightHand, handAttachPoint.rotation);

            // 相手の頭を自分の手の位置に移動
            targetHead.position = handAttachPoint.position;
        }
        else
        {
            // IK解除
            animator.SetIKPositionWeight(AvatarIKGoal.RightHand, 0f);
            animator.SetIKRotationWeight(AvatarIKGoal.RightHand, 0f);
        }
    }
    void SnapOpponentInFront()
    {
        if (TargetRoot == null) return;
        Debug.Log("補正中");
        // 自分の正面にオフセットを付けて配置
        //Vector3 frontPos = transform.position + transform.forward * 0.5f; // 0.5m前
        //TargetRoot.position = frontPos;
        Vector3 frontpos = TargetRoot.position + TargetRoot.forward * 0.8f;
        transform.position = frontpos;

        // 自分の方を向かせる（Y軸回転だけ）
        Vector3 dir = (transform.position - TargetRoot.position).normalized;
        dir.y = 0;
        if (dir != Vector3.zero)
            TargetRoot.rotation = Quaternion.LookRotation(dir);
    }
}
