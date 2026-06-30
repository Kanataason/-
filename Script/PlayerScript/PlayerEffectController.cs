using System;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.VFX;

public class PlayerEffectController : MonoBehaviour
{

    [SerializeField] private AddresableObjectPool<EffectType> efectList;

    private bool isLoaded = false;
    async void Start()
    {
        //エフェクトを生成
       await efectList.Init(transform);

        isLoaded = true;
    }

    private void OnDestroy()
    {
        efectList.Release();
    }


    //演出用のエフェクトのタイプを確認
    public EffectType CanPlayPresentationEffect(TatsuAnimationController.PresentationState presentationState)
    {
        EffectType type = presentationState switch
        {
            TatsuAnimationController.PresentationState.Charge => EffectType.Charge,
            _ => EffectType.None
        };

        return type;
    }

    public void PlayEffect(EffectType type,Vector3 pos,Quaternion rotation,int returnFrame)
    {
        if (!isLoaded) return;
        Debug.Log("エフェクトを出した");
        var effect = efectList.Get(type);

        if (effect == null) return;

        //場所と回転をセット
        SetPosAndRotation(effect, pos, rotation);

        //エフェクトを消すコルーチン
        Delay.WaitFrame(this, returnFrame,null,null,() => { ReturnEffect(type, effect); });

    }
    public void ReturnEffect(EffectType type,GameObject obj)
    {
        if (!isLoaded) return;

        //エフェクトを戻す
        efectList.Return(type, obj);
    }
    public void PlayVfxEffectAndGetVfxEffect(EffectType effectType,Transform root,
        Quaternion rotation,Action<GameObject> receiver)
    {
        var effect = efectList.Get(effectType);
        //場所や回転を設定
        effect.transform.position = root.position;
        effect.transform.rotation = rotation;

        var vis = effect.GetComponent<VisualEffect>();

        //設定が終わったことを通知
        receiver?.Invoke(effect);
    }
    /// <summary>
    /// 位置と回転を設定
    /// </summary>
    /// <param name="effect"></param>
    /// <param name="pos"></param>
    /// <param name="rotation"></param>
    private void SetPosAndRotation(GameObject effect,Vector3 pos,Quaternion rotation)
    {
        effect.transform.position = pos;
        effect.transform.rotation = rotation;
    }

}
