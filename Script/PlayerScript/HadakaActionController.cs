using UnityEngine;
using UnityEngine.VFX;

public class HadakaActionController : BaseActionController
{
    private SkinnedMeshRenderer hadakaBody;
    private GameObject prevEffect;
    private TatuAttackData currentAttackData;

    [Header("パワーアップ情報")]
    [SerializeField] private float playerSpeed = 2.5f;
    [SerializeField] private float playerCorection = 0.4f;//ダメージカット率
    [SerializeField] private float powerUpTime = 15;
    private void Start()
    {
        TryGetComponents();

        Initialize(controller, state, effectController,animationController);
    }
    private void TryGetComponents()
    {
        controller = GetComponent<PlayerController>();
        state = GetComponent<PlayerState>();
        effectController = GetComponent<PlayerEffectController>();
        animationController = GetComponent<TatsuAnimationController>();
    }

    protected override void Initialize(PlayerController controller, PlayerState state,
        PlayerEffectController effectController,TatsuAnimationController animationController)
    {
        base.Initialize(controller, state, effectController,animationController);

        InitList();
    }
    protected override void InitList()
    {
        base.InitList();

        actionDictionary.Add(TatsuAnimationController.AnimationId.Smash, OnSmashAttack);
    }
    public override void OnCheckAction(TatuAttackData data)
    {
        if (actionDictionary.TryGetValue(data.ActionName, out var action))
        {
            action?.Invoke(data);
            return;
        }
    }
    public override void OnAttackStart(TatuAttackData data)
    {
        currentAttackData = data;
        //特別な技か確認
        ApplyPowerUpState(data.ActionName);

        //当たり判定を復活させる
        InvokeThrowSuccess("Idle",true);

        //アクションがあるか確認
        OnCheckAction(data);

    }
    public override void OnAttackEnd()//技終了
    {
        ApplyPowerUpState(currentAttackData.ActionName);

        base.OnAttackEnd();
    }
    private void ApplyPowerUpState(TatsuAnimationController.AnimationId animationId)
    {
        if (animationId is TatsuAnimationController.AnimationId.Smash)
        {
            if (state.currentState.HasFlag(StatusFlags.IsPowerUpeffect))
            {
                state.RemoveStatusFlag(StatusFlags.IsPowerUpeffect);

                //パワーアップ状態にする
                state.OnStartPowerUp(true, powerUpTime);
                return;
            }
            //状態を追加
            state.AddStatusFlag(StatusFlags.IsPowerUp);
            state.AddStatusFlag(StatusFlags.IsPowerUpeffect);
        }
    }

    protected override void OnSmashAttack(TatuAttackData data)
    {
        //生成場所を取得
        var root = animationController.GetAnchor(TatsuAnimationController.EffectAnchor.Root);

        //パワーアップ状態かを確認
        bool isPowerUp = state.HasStatusFlag(StatusFlags.IsPowerUp);

        //パラメータを宣言
        const float normalCorrection = 0.2f;
        const float normalSpeed = 2f;

        float correction = isPowerUp ? playerCorection : normalCorrection;
        float speed =isPowerUp? playerSpeed:normalSpeed;

        //プレイヤーのパラメータにセット
        state.SetMoveParameters(correction,speed);

        //エフェクト生成
        if (prevEffect != null)
        {
            SetEffectInfo(prevEffect, isPowerUp);

            effectController.ReturnEffect(EffectType.Smash, prevEffect);
            prevEffect = null;
            return;
        }

        //エフェクト再生
        effectController.PlayVfxEffectAndGetVfxEffect
            (EffectType.Smash, root, Quaternion.identity, effect=>SetEffectInfo(effect,isPowerUp) );

        //Se再生
        if(isPowerUp)
        UIAudioSound.Instance.PlaySe(UIAudioSound.SeState.HadakaSA);

    }
    private void SetEffectInfo(GameObject effect,bool isPowerUp)
    {
        //メッシュが無ければ取得
        if (hadakaBody == null)
            hadakaBody = animationController.GetAnchor(TatsuAnimationController.EffectAnchor.Body)
                .GetComponent<SkinnedMeshRenderer>();

        //エフェクトの燃え具合を設定
        float normal = 0;
        float powerUp = 0.2f;
        float offset = isPowerUp?powerUp:normal;

        var vfxEfe = effect.GetComponent<VisualEffect>();
        if (vfxEfe == null) return;

        //エフェクトに情報をセット
        vfxEfe.SetBool("IsDead",isPowerUp);
        vfxEfe.SetSkinnedMeshRenderer("Charactor", hadakaBody);

        //マテリアルを更新
        foreach (var mate in hadakaBody.materials)
            mate.SetFloat("_offset", offset);

        prevEffect = effect;
    }
}
