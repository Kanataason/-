using UnityEngine;

public class RobotActionController : BaseActionController
{
    private ColliderManager colliderManager;
    private TatuCollderManager tatuCollderManager;

    private ColliderManager targetColliderManager;

    [SerializeField] private float bulletVelocity = 8;
    private void Start()
    {
        TryGetComponents();
    
        Initialize(controller, state, effectController, animationController);
    }
    private void TryGetComponents()
    {
        effectController = GetComponent<PlayerEffectController>();
        animationController = GetComponent<TatsuAnimationController>();
        colliderManager = GetComponent<ColliderManager>();
        tatuCollderManager = GetComponent<TatuCollderManager>();
    }
    //初期化
    protected override void Initialize(PlayerController controller, PlayerState state, PlayerEffectController effectController, TatsuAnimationController animationController)
    {
        base.Initialize(controller, state, effectController, animationController);

        actionDictionary.Add(TatsuAnimationController.AnimationId.Smash, OnSmashAttack);
    }
    //ステートにアクションが紐づいているか確認する
    public override void OnCheckAction(TatuAttackData data)
    {
        if (actionDictionary.TryGetValue(data.ActionName, out var action))
        {
            action?.Invoke(data);
            return;
        }
    }

    //攻撃初め
    public override void OnAttackStart(TatuAttackData data)
    {
        InvokeThrowSuccess("", false);

        OnCheckAction(data);
    }
    //攻撃終わり
    public override void OnAttackEnd()
    {
        base.OnAttackEnd();
    }

    //スマッシュ攻撃
    protected override void OnSmashAttack(TatuAttackData data)
    {
        //スポーン場所をゲット
        var root = animationController.GetAnchor(TatsuAnimationController.EffectAnchor.Hand);

        //エフェクト再生
        effectController.PlayVfxEffectAndGetVfxEffect
            (EffectType.Smash, root, Quaternion.identity, effect => { SetEffectInfo(effect, data); });

        //Se再生
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.RobotSA);
    }
    /// <summary>
    /// エフェクトに情報をセット
    /// </summary>
    /// <param name="effect"></param>
    private void SetEffectInfo(GameObject effect,TatuAttackData data)
    {
        //弾のスクリプトを取得
        var script = effect.GetComponent<BulletMovement>();

        //前方向に弾を出す
        effect.transform.position += transform.forward;

        Vector3 vector = transform.forward;//向き

        //相手を取得
        var opponent = tatuCollderManager.GetNormalOpponent();

        if (targetColliderManager == null)
        {
            targetColliderManager = opponent.GetComponent<ColliderManager>();
        }

        //弾に飛ぶ方向と,速さを与える
        script.SetBulletParameter(bulletVelocity, vector);

        //参照を渡す
        script.SetReference(targetColliderManager,opponent,colliderManager,data);
        
        float returnTime = 2;
        //エフェクト返却処理
        Delay.WaitTime(this, returnTime, () =>
        { effectController.ReturnEffect(EffectType.Smash, effect); });
    }
}
