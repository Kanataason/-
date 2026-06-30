using System;
using System.Collections.Generic;
using UnityEngine;
using static TatsuAnimationController;
/// <summary>
/// プレイヤー現在の状態
/// </summary>
public enum PlayerTranstionState
{
    Nomal,
    BegingAttacked,
    CrouchGuard,
    StandGuard,
    Jump,
    Crouch,
    Hitting,
    Flying,
    PanishCounter,
    Counter,
    PlayerDown,
    Win,
}

/// <summary>
/// プレイヤーの特殊状態
/// </summary>
[Flags]
public enum StatusFlags
{
    None =0,
    IsPowerUp = 1<<0,//パワーアップ状態
    IsPowerUpeffect = 1<<1,//パワーアップの演出中   
    IsThrowing = 1<<2,//捕まれている状態

}
public class PlayerState : MonoBehaviour
{
    //イベント
    public event Action<PlayerState> OnHpbarChange;//Hpの減りをUiに通知するアクション
    public event Action<PlayerState> OnOffHpBarChange;//Hpの白ゲージをけすことを通知するアクション
    public event Action<PlayerState,bool,float> OnGaugeChange;//現在のゲージをUiに通知するアクション
    public event Action<PlayerState, float> OnPowerUptime;//パワーアップの通知をUiに通知するアクション
    public event Action<AnimationId> OnStatusEnded;//プレイヤーの状態が終わった通知するアクション
    public event Action OnPlayerDeadAction;//死んだことを通知するアクション


    //現在の状態や行動状態のステート
    public PlayerTranstionState currentTranstionState = PlayerTranstionState.Nomal;
    public StatusFlags currentState = StatusFlags.None;

    //イベント登録用の参照
    private PlayerController tatsuPlayerController;

    [Header("Player状態")]

    public int PlayerNumber = 0;

    public float MaxHp = 1;
    public float CurrentHp = 0;
    public bool IsDead = false;

    //プレイヤーのゲージ変数
    public float CurrentPlayerGauge;
    public float PlayerMaxGauge =30000;

    ///<summary>ダメージカット率</summary>
    public float Correction = 0.2f;

    ///<summary>プレイヤーの速度</summary>
    public float PlayerSpeed = 3;

    //ヒット時の情報変数

    ///<summary>攻撃した側のコンボカウント</summary>
    public int ReceiveHitCount = 0;

    ///<summary>攻撃された側のコンボカウント</summary>
    public int ComboCount = 0;

    ///<summary>空中でのコンボカウント</summary>
    public int FlyComboCount = 0;
  

    private void Start()
    {
        TryGetComponents();

        //パラメーター初期化
        Init();

        //イベント登録
        SubscribeEvents();
    }
    public void Init()//プレイヤー情報初期化処理
    {
        CurrentHp = MaxHp;
        CurrentPlayerGauge = 0;
        IsDead = false;

        ChangeState(PlayerTranstionState.Nomal);

        //Uiの初期化
        OnHpbarChange?.Invoke(this);
        OffHpbarChenge();
        AddGauge(CurrentPlayerGauge);
    }
    private void TryGetComponents()
    {
        TryGetComponent<PlayerController>(out tatsuPlayerController);
    }
    private void SubscribeEvents()//イベント登録
    {
        if(tatsuPlayerController != null)
        tatsuPlayerController.OnUpdataPlayerStateAction += ChangeState;
    }

    private void UnSubscribeEvents()//イベント解除
    {
        if (tatsuPlayerController != null)
            tatsuPlayerController.OnUpdataPlayerStateAction -= ChangeState;
    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }
    /// <summary>
    /// ダメージを与える関数
    /// </summary>
    /// <param name="damage"></param>
    /// <param name="Gauge"></param>
    public void TakeDamage(float damage,float Gauge)
    {

        //体力が０以下にならないようにする
        CurrentHp = Mathf.Max(0, CurrentHp - damage);

        //ゲージをカットして減らした分追加する
        AddGauge(Gauge*Correction);

        //Hpが減ったことをUiに反映
        OnHpbarChange?.Invoke(this);

        //体力が０かつステートが死んでいなかったら 死ぬ
        if (CurrentHp <= 0 && !IsDead)
            Die();
    }
    /// <summary>
    /// ガードできる攻撃か確認
    /// </summary>
    /// <param name="attackType"></param>
    /// <returns></returns>
    public bool CanGuard(AttackType attackType)
    {
        bool isGuard = false;

        switch (currentTranstionState)
        {
            //立ガードなら下段以外の攻撃はガード可能
            case PlayerTranstionState.StandGuard:

                if (attackType == AttackType.LowerRow) return false;

                isGuard = true; break;

            //しゃがみガードなら中段以外の攻撃はガード可能
            case PlayerTranstionState.CrouchGuard:

                if (attackType is AttackType.MiddleRow) return false;

                isGuard = true; break;
            default: break;
        }
        return isGuard;
    }
    public bool IsCheckHit()
    {
        return currentTranstionState is PlayerTranstionState.Hitting or PlayerTranstionState.Flying;
    }

    //引数で渡されたゲージが使えるか確認
    public bool CanUseGauge(int useGauge)
    {
        return useGauge <= CurrentPlayerGauge;
    }
    //ゲージを使用してUiに反映させる
    public void UseGauge(float Gauge)
    {
        CurrentPlayerGauge -= Gauge;

        //０～マックスの値に収まるように補間
        CurrentPlayerGauge = Mathf.Clamp(CurrentPlayerGauge, 0, PlayerMaxGauge);

        //Uiに反映させる
        OnGaugeChange?.Invoke(this,false,CurrentPlayerGauge);
    }

    //引数で渡された数ゲージを足す
   public void AddGauge(float Gauge)
    {
        CurrentPlayerGauge += Gauge;

        //０～マックスの値に収まるように補間
        CurrentPlayerGauge = Mathf.Clamp(CurrentPlayerGauge, 0, PlayerMaxGauge);

        //Uiに反映させる
        OnGaugeChange?.Invoke(this,true,CurrentPlayerGauge);
    }

    //パラメーターを設定
    public void SetMoveParameters(float correction,float playerSpeed)
    {
        Correction = correction;//ダメージカット率
        PlayerSpeed = playerSpeed;//プレイヤーの移動スピード
    }

    public void Die()//死んだときの処理をする
    {

        IsDead = true;
        //死んだことを通知する
        OnPlayerDeadAction?.Invoke();

    }

    //キャラクターごとにパラメーターを設定する
    public void SetPlayerInfo(CostomCharacterData data)
    {
        //最大Hpを設定
        MaxHp = data.Hp;
        CurrentHp = MaxHp;

        bool typeTank = data.Type is CostomCharacterData.PlayerType.Tank;

        bool typePower = data.Type is not CostomCharacterData.PlayerType.Power;

        //タイプに合った値を設定
        PlayerSpeed = typeTank ? 1.5f : 2;

        Correction = typePower ? 0.2f : 0.1f;

    }

    /// <summary>
    /// UiにHpの減りを反映させる
    /// </summary>
    public void OffHpbarChenge() => OnOffHpBarChange?.Invoke(this);

    //hadakaの必殺技の状態処理
    public void OnStartPowerUp(bool isStartPowerUp = false,float PowerUptime = 0.1f)
    {
        //パワーアップしているか確認
        if (isStartPowerUp)
        {
            OnPowerUptime?.Invoke(this, PowerUptime);
            return;
        }

        //状態を解除
        RemoveStatusFlag(StatusFlags.IsPowerUp);
        RemoveStatusFlag(StatusFlags.IsPowerUpeffect);

        //終わったことを通知してリセット処理をする
        OnStatusEnded?.Invoke(AnimationId.Smash);
    }
    public void OnResetComboCount()
    {
        ComboCount = 0;
        ReceiveHitCount = 0;
        FlyComboCount = 0;
    }

    ///---------------ここからステートを変更する関数--------------

    public void ChangeState(PlayerTranstionState state)//プレイヤーの行動状態を変更
    {
        if (currentTranstionState == state) return; // 同じなら何もしない

        currentTranstionState = state;//ステートを入れ替える
    }
    public void AddStatusFlag(StatusFlags flag)//状態を追加
    {
        currentState |= flag;
    }
    public void RemoveStatusFlag(StatusFlags flag)//状態を解除
    {
        currentState &= ~flag;
    }
    public bool HasStatusFlag(StatusFlags flag)//状態を確認
    {
        return currentState.HasFlag(flag);
    }
}