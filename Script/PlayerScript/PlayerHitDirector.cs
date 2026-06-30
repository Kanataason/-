using System;
using System.Collections.Generic;
using UnityEngine;
using static TatsuAnimationController;
using static ColliderManager;
public class PlayerHitDirector : MonoBehaviour
{
    public enum HitResult
    {
        Hit,
        Guard,
        Counter,
    }

    //参照先
    private PlayerState state;
    private PlayerController playerController;
    private PlayerKnockBackController knockBackManager;
    private TatsuAnimationController animationController;
    private PlayerEffectController effectController;
    private ColliderManager colliderManager;
    private BaseActionController actionController;

    private PlayerState targetState;

    private TatuAttackData currentAttackdata;

    private Dictionary<AttackAttribute, Action<ColliderManager.HitInfo,Vector3>> actionList;
    void Start()
    {
        TryGetComponents();
        InitList();
    }
    private void InitList()
    {
        actionList = new()
        {
            { AttackAttribute.DownAttack,ApplyDownAttack},
            {AttackAttribute.SmashAttack,ApplySmashAttack },
            {AttackAttribute.LaunchAtack,ApplyLaunchAttack },
            {AttackAttribute.NormalAttack,ApplyNormalAttack}
        };
    }
    private void TryGetComponents()
    {
        colliderManager = GetComponent<ColliderManager>();
        actionController = GetComponent<BaseActionController>();
        effectController = GetComponent<PlayerEffectController>();
        knockBackManager = GetComponent<PlayerKnockBackController>();
        state = GetComponent<PlayerState>();
        playerController = GetComponent<PlayerController>();
        animationController = GetComponent<TatsuAnimationController>();
    }
    //受けた攻撃の情報を渡す
    public TatuAttackData GetAttackData()=> currentAttackdata;


    //攻撃をした側の処理
    public void OnAttackGuard(HitInfo hitInfo)
    {
        //nullなら相手のステータスをゲット
        if (targetState == null)
            targetState = hitInfo.targetManager.GetComponent<PlayerState>();

        var data = hitInfo.hitBoxData;
        var dir = hitInfo.targetManager.transform.position - transform.position;

        playerController.SetTargetTranstionState(targetState.currentTranstionState);
        knockBackManager.StopFowardCoroutine();

        float correction = 0.7f;//補正
        state.AddGauge(data.AddGuardGauge * correction);

        //自分が壁側に居るかを確認 もし居なかったらノックバック
        if (!knockBackManager.CheckCanKnockBack(hitInfo.targetManager.transform.position)) return;
        //ガードバック 反対方向に飛ぶためdirを反対に
        knockBackManager.ForwardMove(-dir, data.GuardKnockBackTime, data.GuardKnockBackDistance);
    }
    public void OnAttackHit(HitInfo hitInfo)
    {
        //nullなら相手のステータスをゲット
        if (targetState == null)
            targetState = hitInfo.targetManager.GetComponent<PlayerState>();

        //４回までしか攻撃ができないようにする
        if (targetState.FlyComboCount >= 4) return;

        var data = hitInfo.hitBoxData;
        var dir = hitInfo.targetManager.transform.position - transform.position;
        dir.y = 0;

        playerController.SetTargetTranstionState(targetState.currentTranstionState);
        knockBackManager.StopFowardCoroutine();

        state.AddGauge(data.AddHitGauge);

        if (hitInfo.hitBoxData.AtAttri != AttackAttribute.NormalAttack) return;

        if (playerController.PlayerActionState is PlayerAction.Jump) return;

        //自分が壁側に居るかを確認 もし居なかったらノックバック
        if (!knockBackManager.CheckCanKnockBack(hitInfo.targetManager.transform.position)) return;
        //ガードバック 反対方向に飛ぶためdirを反対に
        knockBackManager.ForwardMove(-dir, data.GuardKnockBackTime, data.HitKnockbackDistance);
    }
    //投げた側の処理
    public void OnThrowHit(TatsuAnimationController targetAnimationController,int grapFrames,HitInfo hitInfo)
    {
        //nullなら相手のステータスをゲット
        if (targetState == null)
            targetState = hitInfo.targetManager.GetComponent<PlayerState>();

        //グラップ入力受付時間を決めてタイマースタート
        Delay.WaitFrame(this, grapFrames, 
           () => targetAnimationController.GetThrowingFlag(),
           () => //投げ抜け成立
           {
               //投げ抜け処理
               ApplyGrap(hitInfo);

               //相手に投げ抜けを通知
               hitInfo.hitDirector.ApplyGrap(hitInfo); 
           },
           () => //投げ成立
           {
               //投げ処理
               OnApplyThrowing(targetAnimationController);

               //相手に投げ成立を通知
               hitInfo.hitDirector.ApplyThrow(hitInfo); 
           });
    }
    //投げが成立したら
    private void OnApplyThrowing(TatsuAnimationController targetAnimationController)
    {
        //アニメーション再生
        animationController.PlayAnimation(AnimationId.Throwing);

        //投げられフラグを直す
        colliderManager.SetThrowingFlag();
        animationController.SetThrowingFlag();

        //ステートを変更
        playerController.HasInoperable();

        //相手の位置を補正させる
        var targetRoot = targetAnimationController.GetAnchor(EffectAnchor.Root);
        knockBackManager.SnapOpponentInFront(targetRoot);
    }
    //--------------------アニメーションイベントから呼ばれる

    //投げた側の処理
    public void OnApplyThrowingDamage(TatuAttackData data)
    {
        //ゲージを増やす
        state.AddGauge(data.AddHitGauge);
    }





    /// <summary>
    /// ここから下はアタックされた方の処理
    /// <param name="hitInfo"></param>







    //ガード時の処理
    public void ReceiveGuard(HitInfo hitInfo)
    {
        //受けた攻撃のデータ
        var data = hitInfo.hitBoxData;
        currentAttackdata = data;

        //敵方向のベクトルを獲得
        var dir = hitInfo.targetManager.transform.position - transform.position;
        dir.y = 0;

        knockBackManager.StopFowardCoroutine();

        //ゲージを増やす
        state.AddGauge(data.AddGuardGauge);

        //nullなら相手のステータスをゲット
        if (targetState == null)
            targetState = hitInfo.targetManager.GetComponent<PlayerState>();

        //エフェクトを再生
        int returnTime = 30;
        effectController.PlayEffect(EffectType.Guard, hitInfo.hitPos, Quaternion.identity, returnTime);

        //アニメーション再生
        animationController.PlayCheckHitOrGuard(AnimationId.Guarding);
        PlayAnimation(HitResult.Guard);

        //自分が壁側に居るかを確認 もしいたらノックバックさせない
        if (knockBackManager.CheckCanKnockBack(transform.position)) return;
        //ガードバック
        knockBackManager.ForwardMove(-dir, data.GuardKnockBackTime, data.GuardKnockBackDistance);

        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Guard);//Se再生
    }
    //ヒット時の処理
    public void ReceiveHit(HitInfo hitInfo)
    {
        //空中コンボ中は４回までしか攻撃が入らないようにする 必殺技は入るようにする
        if (state.FlyComboCount >= 4&&
            hitInfo.hitBoxData.ActionName is not AnimationId.Smash) return;

        var data = hitInfo.hitBoxData;
        currentAttackdata = hitInfo.hitBoxData;

        knockBackManager.StopFowardCoroutine();

        //nullなら相手のステータスをゲット
        if (targetState == null)
            targetState = hitInfo.targetManager.GetComponent<PlayerState>();
        
        //ダメージを与える処理
        state.TakeDamage(ApplyDamage(data,targetState),data.AddHitGauge);

        
        CheckAttackAttribute(hitInfo);
    }
    //投げられた時の処理
    public void ReceiveThrow(HitInfo hitInfo)
    {
        //投げられ状態にする
        state.AddStatusFlag(StatusFlags.IsThrowing);

        currentAttackdata = hitInfo.hitBoxData;

        //nullなら相手のステータスをゲット
        if (targetState == null)
            targetState = hitInfo.targetManager.GetComponent<PlayerState>();

    }

    private void CheckCancelState()//特定の状態か見る
    {
        //パワーアップの演出の最中ならキャンセルする
        if (state.HasStatusFlag(StatusFlags.IsPowerUpeffect))
        {
            //パワーアップ状態&パワーアップ演出中を解除
            state.RemoveStatusFlag(StatusFlags.IsPowerUp);
            state.RemoveStatusFlag(StatusFlags.IsPowerUpeffect);

            var data = animationController.GetCurrentAttackData();
            if (data == null) return;

            //登録しているアクションがあるか確認
            actionController.OnCheckAction(data);
            return;
        }
        return;
    }
    //受けた攻撃がどんな属性を持っているか
    private void CheckAttackAttribute(HitInfo hitInfo)
    {
        var data = hitInfo.hitBoxData;
        var attributeState = data.AtAttri;
        var dir = hitInfo.targetManager.transform.position - transform.position;//ベクトルを取得
        dir.y = 0;

        //パワーアップ状態だったらエフェクトを変える
        var effectState = targetState.currentState.HasFlag(StatusFlags.IsPowerUp)
          ? EffectType.PowerUp : EffectType.Hit;

        //エフェクトを再生
        int returnTime = 30;
        effectController.PlayEffect(effectState, hitInfo.hitPos, Quaternion.identity, returnTime);

        //ジャンプ状態じゃなければ
        if (state.currentTranstionState is not (PlayerTranstionState.Jump or PlayerTranstionState.Flying))
        {
            //特別な状態か確認
            CheckCancelState();
        }

        //プレイヤーが現在飛んでいるか、確認飛んでいたら属性を変更
        var resultState = CheckPlayerFlying(attributeState);

        Debug.Log(resultState);
        //アクションが登録されているかを確認
        if(actionList.TryGetValue(resultState,out var action))
        {
            action?.Invoke(hitInfo,dir);
        }
        return;

    }
    private AttackAttribute CheckPlayerFlying(AttackAttribute attackState)
    {
        //特別の攻撃だったら属性を変更しない
        if (attackState is AttackAttribute.SmashAttack) 
            return attackState;

        //自分が飛ばされているもしくはジャンプをしていたら
        bool isFly = state.currentTranstionState is PlayerTranstionState.Flying or PlayerTranstionState.Jump;

        //ふっとび属性に変更
        return isFly ? AttackAttribute.LaunchAtack : attackState;
    }

    //アニメーションを再生してプレイヤーの行動ステートを変える
    private void PlayAnimation(HitResult hitResult)
    {
        playerController.SetHitOrGuardFlag(hitResult);
        playerController.ProcessHit();
    }
    //ダメージ計算
    private float ApplyDamage(TatuAttackData data,PlayerState state)
    {
        float powerUpOffset = 1.5f;
        float normalOffset = 1;
        float damage = data.Damage;

        bool isPowerUp = state.currentState is StatusFlags.IsPowerUp;

        damage = isPowerUp ? damage * powerUpOffset : damage * normalOffset;
        damage = OnDamage(damage, data.damageReductionPerHit);
        damage *= Mathf.Max(0f, 1f - state.Correction);//範囲を超えないようにするため補間
        Debug.Log(damage);
        return damage;
    }
    float OnDamage(float damage, float damageReductionPerHit)
    {
        //800damage * Pow(0.6^2....3...4)= 0.36 .. 0.216..0.1296
        //こういう漢字で補正を入れて計算をしていく
        float newDamage = Mathf.Pow(damageReductionPerHit,state.ReceiveHitCount -1);
        float finalDamage = damage * newDamage;
        return finalDamage;
    }


    //技の属性に合った処理をする
    private void ApplySmashAttack(HitInfo hitInfo,Vector3 dir)//特別な技（必殺技じゃない）
    {
        var data = hitInfo.hitBoxData;

        state.ChangeState(PlayerTranstionState.Flying);
        animationController.PlayLaunchAnimation(AnimationId.Launch);
        //アニメーションを再生
        PlayAnimation(HitResult.Hit);

        //ノックバック処理
        var knockInfo = data.knockInfos[0];
        knockBackManager.Launch(-dir, knockInfo.Power, knockInfo.Height, knockInfo.Duration);

        //Se再生
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Hit);
    }
    private void ApplyDownAttack(HitInfo hitInfo,Vector3 dir)//ダウン攻撃
    {
        state.ReceiveHitCount++;

        var data = hitInfo.hitBoxData;
        //アニメーションを再生
        animationController.PlayLaunchAnimation(AnimationId.Down);

        //ステートを変更
        playerController.HasInoperable();

        //Se再生
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Hit);
    }
    private void ApplyLaunchAttack(HitInfo hitInfo,Vector3 dir)//ふっとばし攻撃
    {
        state.ReceiveHitCount++;//攻撃を何かい当てたかのコンボカウント補正に使われる

        state.FlyComboCount++;//空中コンボカウント

        var data = hitInfo.hitBoxData;

        //アニメーションやステートを更新
        state.ChangeState(PlayerTranstionState.Flying);
        animationController.PlayLaunchAnimation(AnimationId.Launch);

        //アニメーションを再生
        PlayAnimation(HitResult.Hit);

        //ノックバックの処理をする
        var KnockInfo = data.knockInfos[0];
        float height = state.ReceiveHitCount > 0f ? KnockInfo.Height * 1.5f : KnockInfo.Height + 1;
        knockBackManager.Launch(-dir, KnockInfo.Power, height, KnockInfo.Duration);

        //Se再生
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Hit);
    }
    private void ApplyNormalAttack(HitInfo hitInfo,Vector3 dir)//通常攻撃の処理
    {

        state.ReceiveHitCount++;//攻撃を何回当てたかのコンボカウント補正に使われる

        var data = hitInfo.hitBoxData;
        state.ChangeState(PlayerTranstionState.Hitting);

        //アニメーションを再生
        animationController.PlayCheckHitOrGuard(AnimationId.Hitting);
        PlayAnimation(HitResult.Hit);

        //Se再生
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Hit);

        //自分が壁側に居るかを確認 もしいたらノックバックさせない
        if (knockBackManager.CheckCanKnockBack(transform.position)) return;
        //自分をバック
        knockBackManager.ForwardMove(-dir, data.GuardKnockBackTime, data.HitKnockbackDistance);

    }

    //投げ成立
    private void ApplyThrow(HitInfo hitInfo)
    {
        state.RemoveStatusFlag(StatusFlags.IsThrowing);
        //アニメーション再生
        state.ChangeState(PlayerTranstionState.Hitting);
        animationController.ThrowAnime(AnimationId.Thrown);

        //投げられフラグを直す
        colliderManager.SetThrowingFlag();
        animationController.SetThrowingFlag();

        colliderManager.ThrowActive(false);
        playerController.HasInoperable();


        knockBackManager.StopFowardCoroutine();
    }

    //投げ抜け成立
    private void ApplyGrap(HitInfo hitInfo)
    {
        //状態を解除
        state.RemoveStatusFlag(StatusFlags.IsThrowing);

        //相手の方向ベクトルを取得
        var dir = hitInfo.targetManager.transform.position - transform.position;
        var data = hitInfo.hitBoxData.knockInfos[0];

        //投げられフラグを直す
        Delay.OneFrame(this,()=> colliderManager.SetThrowingFlag());
        animationController.SetThrowingFlag();

        //アニメーション再生
        state.ChangeState(PlayerTranstionState.Hitting);
        animationController.ThrowAnime(AnimationId.Grabbing);

        //ステートを変更
        playerController.HasInoperable();
        colliderManager.ThrowActive(false);

        //ノックバック
        knockBackManager.ForwardMove(-dir,data.Duration,data.Power);
    }



    ///-----------ここからはアニメーションイベントから呼ばれる



    public void ReceiveThrowDamage()//投げられている最中に呼ばれる
    {
        if (state.CurrentHp <= 0) return;

        var data = currentAttackdata;
        float correction = 0.6f;//カット率

        //Se再生
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.Hit);

        //ダメージを反映
        float addGauge = data.AddGuardGauge * correction;
        state.TakeDamage(ApplyDamage(data, targetState), addGauge);
    }
}
