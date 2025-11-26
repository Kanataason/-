using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.Playables;
using UnityEngine.VFX;

public class ColliderManager : MonoBehaviour
{
 public enum ColliderTypeName { HitBox,HurtBox,ThrowBox,ThrowHurtBox}
    public BoxCollider PushBox;
    public BoxCollider ThrowHurtBox;
    public Transform MyHead;
    public Transform MyRoot;
    [System.Serializable]
    public class ManagedCollider
    {
        public Collider collider;
        public ColliderTypeName Collidertype;
        public HitBoxData hitBoxData;
    }

    [SerializeField] private List<ManagedCollider> collidersList = new();

    public PlayerState state;
    [SerializeField] private float SetPushPower = 2.2f;
    private TatsuAnimationController controller;
    private TatsuPlayerController playerController;
    private KnockBackMotion backMotion;

    private HashSet<string> HistoryList = new HashSet<string>();
    private HashSet<int> ints = new HashSet<int>();

    private Coroutine HitFramecoroutine;


    //private int ThrowWindow = 20;
    //private bool IsCantech = false;
    private Coroutine GrapingTime;
    private bool cancelGrap = false;
    public bool Graped = false;
    public bool IsThrowing = false;
    private ColliderManager BufferCollMana;
  void init()
    {
        state = GetComponent<PlayerState>();
        collidersList.Clear();
        foreach (var coll in GetComponentsInChildren<Collider>(true))//enableも取る
        {
            var script = coll.GetComponent<ColliderType>();
            if (script == null) continue;

            ColliderTypeName type;

            switch (script.types)
            {
                case ColliderTypes.HitBox:
                    type = ColliderTypeName.HitBox;
                    break;
                case ColliderTypes.ThrowBox:
                    type = ColliderTypeName.ThrowBox;
                    break;
                default:
                    type = ColliderTypeName.HurtBox;
                    break;
            }
            HitBoxData data = coll.GetComponentInParent<HitBoxData>();
            collidersList.Add(new ManagedCollider()
            {
                collider = coll,
                Collidertype = type,
                hitBoxData = data

            });
        }
        Debug.Log(collidersList.Count);
    }
    private void Start()
    {
        init();
        backMotion = GetComponent<KnockBackMotion>();
        playerController = GetComponent<TatsuPlayerController>();
        controller = GetComponent<TatsuAnimationController>();
    }
    //state My Other Enemy
    public void CheckCollider(ColliderManager manager)//当たり判定 相手のstate
    {
        float MaxDistance = 3f;
        Vector3 dir = transform.position - manager.transform.position;
        if (dir.sqrMagnitude <= MaxDistance * MaxDistance)
        {
            foreach (var MyColl in collidersList)
            {
                if (MyColl.Collidertype != ColliderTypeName.HitBox) continue;
                if (!MyColl.collider.enabled || !MyColl.collider.gameObject.activeInHierarchy) continue;
                foreach (var otherColl in manager.collidersList)
                {
                    if (otherColl.Collidertype != ColliderTypeName.HurtBox) continue;
                    if (!otherColl.collider.enabled || !otherColl.collider.gameObject.activeInHierarchy) continue;
                    var myBounds = MyColl.collider.bounds; // ここで一回
                    var otherBounds = otherColl.collider.bounds;
                    if (myBounds.Intersects(otherBounds))//一回キャッシュ
                    {
                        string pairId = MyColl.collider.GetInstanceID() + "-" + otherColl.collider.GetInstanceID();

                        if (HistoryList.Contains(pairId)) continue;

                        HistoryList.Add(pairId);
                        var datas = MyColl.hitBoxData;
                        if (datas == null) continue;
                        if (ints.Contains(datas.AttackId)) continue;
                        ints.Add(datas.AttackId);
                        if (manager.IsThrowing) return;
                        var data = datas.tatuAttackData;
                        Debug.Log(otherColl.collider.name);
                        // --- ここでガード判定 ---
                        bool isGuarding = manager.state.PState == PLayerState.Guard || manager.state.PState == PLayerState.StandGuard;

                        if (isGuarding)
                        {
                            if (CanGuard(manager.state.PState, data.attackType))//ネスト深すぎ注意
                            {
                                manager.backMotion.StopFowardCoroutine();
                                backMotion.StopFowardCoroutine();
                                manager.state.AddGauge(data.AddGuardGauge);
                                state.AddGauge(data.AddGuardGauge *0.9f);
                                //  ガード成功
                                manager.playerController.IsGuarding = true;
                                manager.controller.Hit(otherColl.collider.ClosestPoint(MyColl.collider.transform.position), "Guard");

                                if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
                                HitFramecoroutine = StartCoroutine(Stiffnesing(data.BlockStunTime, manager.state, manager));
                                var dirs = manager.transform.position - transform.position;
                                if (manager.backMotion.CheckCanKnockBack(transform))
                                {
                                    manager.backMotion.FowerdOnMove(dirs, data.GuardKnockBackTime, data.GuardKnockBackDistance, playerController);
                                }
                                else
                                {
                                    backMotion.FowerdOnMove(-dirs, data.GuardKnockBackTime, data.GuardKnockBackDistance, playerController);
                                }
                                return; // ガード成立なので処理終了
                            }
                            // ガードできない → ヒット扱い
                        }
                        if(data.AtAttri != AttackAttribute.SmashAttack)
                        {
                            if (manager.state.FlyComboCount > 2) return;
                        }
                        if (manager.state.PState == PLayerState.PanishCounter) Debug.Log("pani");
                        else if (manager.state.PState == PLayerState.Counter) Debug.Log("Counter");
                        else Debug.Log("Normal");
                       // if (manager.controller.IsCancel&&manager.state.IsPowerUp) manager.controller.IsCancel = false;
                        // --- ヒット処理 ---
                        Debug.Log($"この{manager.state.PState}/から来ました");
                        backMotion.StopFowardCoroutine();
                        manager.backMotion.StopFowardCoroutine();
                        manager.playerController.IsHit = true;
                        // manager.state.ChengeState(PLayerState.Hitting);
                        float damage = data.Damage;
                        damage = state.IsPowerUp ? damage * 1.5f :damage*1;
                        damage = OnDamage(damage, data.damageReductionPerHit, manager.state);
                        damage *= Mathf.Max(0f, 1f - manager.state.correction);
                        manager.state.TakeDamage(damage, data.frameData[^1].FrameNumber, data.AddHitGauge);
                        state.AddGauge(data.AddHitGauge);
                        var ClossPos = otherColl.collider.ClosestPoint(MyColl.collider.transform.position);
                        CheckAttackAttribute(data.AtAttri, data, ClossPos, manager);
                        if (manager.state.PState == PLayerState.Flying || manager.state.PState == PLayerState.Jump)
                        {
                            if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
                            HitFramecoroutine = StartCoroutine(Stiffnesing(120, manager.state, manager, true));
                        }
                        else
                        {
                            if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
                            HitFramecoroutine = StartCoroutine(Stiffnesing(data.HitStunTime, manager.state, manager));
                        }

                        #region
                        //if (manager.state.PState != PLayerState.Guard&&manager.state.PState != PLayerState.StandGuard)
                        //{
                        //    var data = datas.tatuAttackData;
                        //    if (CanGuard(manager.state.PState, data.attackType)) return;
                        //    manager.playerController.IsHit = true;
                        //    var damage = data.Damage;
                        //    damage = OnDamage(damage,data.damageReductionPerHit, manager.state);
                        //    damage *= Mathf.Max(0f, 1f - manager.state.correction);
                        //    manager.state.TakeDamage(damage, data.frameData[data.frameData.Count - 1].FrameNumber);
                        //    if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
                        //    HitFramecoroutine = StartCoroutine(Stiffnesing(data.HitStunTime, manager.state,manager));
                        //   manager.controller.Hit(otherColl.collider.ClosestPoint(MyColl.collider.transform.position),"Hit");
                        //}
                        //else
                        //{
                        //    manager.playerController.IsGuarding = true;
                        //    var data = datas.tatuAttackData;
                        //    manager.controller.Hit(otherColl.collider.ClosestPoint(MyColl.collider.transform.position),"Guard");
                        //    if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
                        //    HitFramecoroutine = StartCoroutine(Stiffnesing(data.BlockStunTime, manager.state, manager));
                        //}
                        #endregion
                    }
                }
            }
        }
    }
    public bool CheckAttackAttribute(AttackAttribute attackAttribute, TatuAttackData data, Vector3 ClossPos,ColliderManager manager)
    {
        var dir = manager.transform.position - transform.position;
        bool isFlying = manager.state.PState == PLayerState.Flying||manager.state.PState == PLayerState.Jump;
        Debug.Log(isFlying);
        switch (attackAttribute)
        {
            case AttackAttribute.SmashAttack://ふっとばし必殺技
                Debug.Log("Smash");
                manager.state.ChengeState(PLayerState.Flying);
                manager.controller.Hit(ClossPos, "Hit",this);
                manager.controller.LunchAnimes("Lunch");
                manager.backMotion.Launch(dir, data.HitKnockbackDistance, data.knockInfos[0].Height-1,data.HitKnockbackTime);
                return true;
            case AttackAttribute.DownAttack://ダウン攻撃　アニメーション再生
                if (manager.state.PState == PLayerState.Down) return false;
                manager.state.ComboCount++;
                manager.controller.Hit(ClossPos, "Hit",this);
                manager.controller.DownAnimation();
                manager.ThrowActive(false);//投げの当たり判定をなくすため
                return true;
            case AttackAttribute.LaunchAtack://打ち上げ攻撃 ヒットスタンを長めに設定する
                DoLaunch(dir,ClossPos,manager,data);
                manager.state.ComboCount++;
                return true;
            case AttackAttribute.Normal://通常
                manager.state.ComboCount++;
                if (isFlying)
                    return DoLaunch(dir,ClossPos, manager,data);
                if (!manager.backMotion.CheckCanKnockBack(transform)&& state.PState != PLayerState.Jump)
                {
                    backMotion.FowerdOnMove(-dir, data.HitKnockbackTime, data.HitKnockbackDistance, playerController);
                }
                else if (state.PState != PLayerState.Jump)
                {
                    manager.backMotion.FowerdOnMove(dir, data.HitKnockbackTime, data.HitKnockbackDistance, playerController);
                }
                manager.controller.Hit(ClossPos, "Hit",this);
                return false;
        }
        return false;
    }
    private bool DoLaunch(Vector3 dir,Vector3 ClossPos, ColliderManager manager,TatuAttackData data)
    {
        Debug.Log("sssssss");
      manager.state.FlyComboCount++;
        var KnockInfo = data.knockInfos[0];
        float height = state.ComboCount > 0f ? KnockInfo.Height -1.5f : KnockInfo.Height +1;
        manager.backMotion.Launch(dir, KnockInfo.Power, height, KnockInfo.Duration);
        manager.state.ChengeState(PLayerState.Flying);
        manager.controller.Hit(ClossPos, "Hit", this);
        manager.controller.LunchAnimes("Lunch");
        return true;
    }
    //private void Update()
    //{
    //    if (Input.GetKeyDown(KeyCode.Tab))
    //    {
    //        Debug.Log($"{state.PState}/{Graped}/{controller.Attacked}/{controller.Attacking}");
    //    }
    //}

    public void CheckThrowBox(ColliderManager manager)
    {
       // if()
        foreach(var MyColl in collidersList)
        {
          if (MyColl.Collidertype != ColliderTypeName.ThrowBox) continue;
            if (!MyColl.collider.enabled || !MyColl.collider.gameObject.activeInHierarchy) continue;
            if (!manager.ThrowHurtBox.enabled || !manager.ThrowHurtBox.gameObject.activeInHierarchy)
                return;
            var myBounds = MyColl.collider.bounds;
            var OtherBounds = manager.ThrowHurtBox.bounds;
            if (myBounds.Intersects(OtherBounds))
            {
                var datas = MyColl.hitBoxData;
                if (datas == null) continue;
                var data = datas.tatuAttackData;
                if (ints.Contains(datas.AttackId)) continue;
               manager.state.ChengeState(PLayerState.Graping);//投げれられている
                ints.Add(datas.AttackId);
                if (GrapingTime != null) GrapingTime = null;
               GrapingTime = StartCoroutine(GrapingRimit(manager,data));
                Debug.Log("投げ");
                //Grap 属性にする
            }
        }
    }
    public void CheckPushBox(ColliderManager manager)
    {
        if (BufferCollMana == null) BufferCollMana = manager;
        float MaxDistance = 3f;
        Vector3 dir = transform.position - manager.transform.position;
        if (dir.sqrMagnitude <= MaxDistance * MaxDistance)
        {
            var mybounds = PushBox.bounds;
            var otherbounds = manager.PushBox.bounds;
            if (mybounds.Intersects(otherbounds))
            {
                Vector3 direction = transform.position - manager.transform.position;
                var overlap = (mybounds.extents.x + otherbounds.extents.x) - Mathf.Abs(direction.x);
                if (overlap > 0)
                {
                    float push = overlap / (2f * SetPushPower);
                    transform.position += new Vector3(push * Mathf.Sign(direction.x), 0, 0);
                    manager.transform.position -= new Vector3(push * Mathf.Sign(direction.x), 0, 0);
                }
            }
        }
    }
    IEnumerator GrapingRimit(ColliderManager colliderManager,TatuAttackData data)
    {
        cancelGrap = false;
        float totalFrame = data.frameData[2].FrameNumber;


        for (int i = 0; i < totalFrame; i++)
        {
            if (i > totalFrame - 2) IsThrowing = true;
            if (playerController.IsHit) yield break;

                if (Graped|| state.PState == PLayerState.Graping) // 相手が投げ入力
                {

                    cancelGrap = true;
                    Graped = false;
                    SuccessGrap(colliderManager); // 投げ抜け成立処理
                    yield break;
                }
            yield return null;
        }
        //------------------test---------------//
        //cancelGrap = false;
        //float totalFrame = data.frameData[2].FrameNumber;

        //colliderManager.ThrowWindow = data.frameData[2].FrameNumber;
        //colliderManager.IsCantech = true;
        //for (int i = 0; i < totalFrame; i++)
        //{
        //    if (controller.IsCheckHitting(state)) yield break;
        //    if (colliderManager.IsCantech && colliderManager.ThrowWindow > 0)
        //    {
        //        colliderManager.ThrowWindow--;

        //        if (colliderManager.Graped) // 相手が投げ入力
        //        {

        //            cancelGrap = true;
        //            colliderManager.Graped = false;
        //            SuccessGrap(colliderManager); // 投げ抜け成立処理
        //            yield break;
        //        }
        //    }
        //    if (colliderManager.Graped)
        //    {
        //      colliderManager.Graped = false;
        //        cancelGrap = true;
        //        SuccessGrap(colliderManager);
        //       yield break;
        //    }
        //    yield return null;
        //}
        if (!cancelGrap)
        {
            colliderManager.controller.ThrowAnime("Thrown", colliderManager);
            controller.ThrowAnime("Throwing", colliderManager);
            Debug.Log("Grap　時間終了");
            Graped = false;
            colliderManager.backMotion.StopFowardCoroutine();
            if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
            HitFramecoroutine = StartCoroutine(Stiffnesing(data.HitStunTime, colliderManager.state, colliderManager,true));
            colliderManager.controller.StartGraping(MyHead,MyRoot);
            controller.StartGraping(colliderManager.MyHead,colliderManager.MyRoot);
            colliderManager.playerController.IsHit = true;
            colliderManager.ThrowActive(false);
            IsThrowing = false;
          if (GrapingTime != null)  colliderManager.StopCoroutine(GrapingTime);
        }

        GrapingTime = null;
    }
   public void SuccessGrap(ColliderManager manager)//グラップ成功
    {
        var dir = manager.transform.position - transform.position;
        ApplyGrapEffect(manager, dir,true,1f,0.7f);     // 相手
        ApplyGrapEffect(this, -dir,true,1f,0.7f); // 自分（this.manager が自分の ColliderManager）
        Debug.Log("aaaa");
    }
    void ApplyGrapEffect(ColliderManager target, Vector3 moveDir,bool IsGrap,float Duration,float Power)
    {
        target.playerController.IsHit = true;
        target.backMotion.FowerdOnMove(moveDir, Duration, Power,target.playerController);
        target.state.ChengeState(PLayerState.Hitting);
        target.Graped = false;
       if(IsGrap)target.controller.ThrowAnime("Grap",target);
    }
 public void Stiffnes(ColliderManager manager) 
    {
        if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
        HitFramecoroutine = StartCoroutine(Stiffnesing(30, manager.state, manager));
    }
    IEnumerator Stiffnesing(int Stiffnes,PlayerState state,ColliderManager collider,bool IsThrow = false)//硬直
    {
        for (int i = 0; i < Stiffnes; i++)
        {
            if (state.PState == PLayerState.Hitting) break;
            yield return null;
        }
        //flag
      if(!IsThrow) collider.playerController.IsHit = false;
        collider.playerController.IsGuarding = false;
        state.OffHpbarChenge();
        state.ComboCount =0;
        HitFramecoroutine = null;
        if (IsThrow)
        {
            collider.controller.StopGraping();
            controller.StopGraping();
        }
    }
    float OnDamage(float damage,float damageReductionPerHit,PlayerState state)
    {
        float newDamage = Mathf.Pow(damageReductionPerHit, state.ComboCount - 1);
        float finalDamage = damage * newDamage;
        return finalDamage;
    }
    bool CanGuard(PLayerState guardState, AttackType attackType)
    {
        return (guardState == PLayerState.StandGuard && attackType != AttackType.LowerRow)
            || (guardState == PLayerState.Guard && (attackType == AttackType.LowerRow || attackType == AttackType.UpperRow));
    }
    public void ThrowActive(bool IsActive) {if(ThrowHurtBox.gameObject.activeInHierarchy != IsActive) ThrowHurtBox.gameObject.SetActive(IsActive); }
    public void ContinuousDamage(TatuAttackData attackData) 
    {
        if (state.NowHp <= 0) return;
        UIAudioSound.Instance.HitSe();
        state.AddGauge(attackData.AddHitGauge);
        float damage = attackData.ContinuousDamage;
        damage = state.IsPowerUp ? damage * 2.5f : damage * 1f;
        BufferCollMana.state.TakeDamage(damage, attackData.frameData[attackData.frameData.Count-1].FrameNumber,attackData.AddHitGauge);
        if (HitFramecoroutine != null) StopCoroutine(HitFramecoroutine);
        HitFramecoroutine = StartCoroutine(Stiffnesing(attackData.HitStunTime, BufferCollMana.state, BufferCollMana, true));
    }
    public void ResetHitHistory()
    {
       ints.Clear();
       HistoryList.Clear();
        Debug.Log(HistoryList.Count);
        Debug.Log(ints.Count);
    }
}
