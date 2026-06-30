using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Playables;
[RequireComponent(typeof(Animator))]
public class MakePlayable : MonoBehaviour
{
    public Animator animator;

    //アニメーション再生にひつようなGraphを宣言
    private PlayableGraph _graph;
    private AnimationLayerMixerPlayable _layerMixer;
    private AnimationClipPlayable _clip;

   private AvatarMask crouchMask;
   private AvatarMask normalMask;
    [SerializeField] private float fadeDuration = 0.08f;

    private Coroutine _manualRoutine;
    private Coroutine _fadeCoroutine;

    private int eventIndexBuffer;
    private bool isPlayed = false;
    private void OnValidate()
    {
        if (!animator) animator = GetComponent<Animator>();
    }

    void Start()
    {
        SubscribeEvents();
    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }

    //クリップの再生、停止の通知イベントを登録
    private void SubscribeEvents()
    {
        BattleStateManager.OnStartAnimation += StartClip;
        BattleStateManager.OnStopAnimation += StopClip;
    }

    //クリップの再生、停止の通知イベントを解除
    private void UnSubscribeEvents()
    {
        BattleStateManager.OnStartAnimation -= StartClip;
        BattleStateManager.OnStopAnimation -= StopClip;
    }
    
    void InitGraph()
    {
        //PlayableGraph 初期化
        _graph = PlayableGraph.Create($"{gameObject.name}-graph");
        var output = AnimationPlayableOutput.Create(_graph, "AnimationOutput", animator);

        //AnimationLayerMixerPlayableを使う
        _layerMixer = AnimationLayerMixerPlayable.Create(_graph, 3);
        output.SetSourcePlayable(_layerMixer);

        //AnimatorControllerをInput0に接続
        var controllerPlayable = AnimatorControllerPlayable.Create(_graph, animator.runtimeAnimatorController);
        _layerMixer.ConnectInput(0, controllerPlayable, 0);
        _layerMixer.SetInputWeight(0, 1f);

        //AnimatorControllerを切り離し
        animator.runtimeAnimatorController = null;

        //MaskをInput1に設定しておく
        _layerMixer.SetLayerMaskFromAvatarMask(1, crouchMask);

        _graph.Play();
    }
    //アニメーションコントローラーから呼ばれてアバターマスクをセットしてグラフを初期化
    public void SetAvatarMask(AvatarMask normal,AvatarMask crouch)
    {
        crouchMask = crouch;
        normalMask = normal;

        InitGraph();
    }

    public void Play(AnimationClip clip,AvatarMask mask=null)
    {
        StopManual();
        if (mask == null) mask = normalMask;
        if (_fadeCoroutine != null)
        {
            StopCoroutine(_fadeCoroutine);
            _fadeCoroutine = null;
            _layerMixer.SetInputWeight(1, 0f);
            _layerMixer.SetInputWeight(0, 1f);
        }

        //アニメーションplayableを生成
        _clip = AnimationClipPlayable.Create(_graph, clip);
        _clip.SetDuration(clip.length);
        _clip.Pause();

        // Mixer に接続（Input1に入れる）
        _layerMixer.ConnectInput(1, _clip, 0);
        _layerMixer.SetInputWeight(0, 1f); // ベースモーション常に有効
        _layerMixer.SetInputWeight(1, 1f);
        
        //レイヤーにアバターマスクを設定
        _layerMixer.SetLayerMaskFromAvatarMask(1, mask);

    }

    //位置フレームずつ自分でアニメーションを再生
    public void InitAnimaInfo(int totalFrames, AnimationClip clip, AnimationClipPlayable playable = default)
    {
        eventIndexBuffer = 0;
        isPlayed = false;

        if (!playable.IsValid())
            playable = _clip;

        PlayAnimation(0,totalFrames, clip,playable);
    }
    public void PlayAnimation(int currentIndex,int totalFrames, AnimationClip clip,AnimationClipPlayable playable = default)
    {
        if (isPlayed) return;

        if (!playable.IsValid())
            playable = _clip;

        var events = clip.events;

        int eventIndex = eventIndexBuffer;

        if (currentIndex >= totalFrames)
        {
            EndPlayAnima();
            EndManual();
            return;
        }

        if (!playable.IsValid()) return;

        //現在の時間がclipのどのくらいまで再生しているのかを出している
        float normalizedTime = (float)currentIndex / totalFrames;

        float currentTime = normalizedTime * clip.length;

        //playableに現在の時間を入れる
        playable.SetTime(currentTime);

        //現在のフレームにアニメーションイベントがあるかを確認
        if (eventIndex < events.Length &&
                    events[eventIndex].time <= currentTime)
        {
            ExecuteAnimationEvent(events[eventIndex]);
            eventIndex++;
        }

        eventIndexBuffer = eventIndex;
    }
    public void EndPlayAnima()
    {
        if (!_clip.IsValid())
        {
            Debug.LogWarning("_clip is invalid");
            return;
        }
        var clip = _clip.GetAnimationClip();

        //最後の少し前のアニメーションを再生する
        _clip.SetTime(clip.length - 0.001f);
        _graph.Evaluate();

        isPlayed = true;
        eventIndexBuffer = 0;

    }
    private void ExecuteAnimationEvent(AnimationEvent animEvent)
    {
        // AnimationEvent の functionName を対象のGameObjectに送信
        SendMessage(animEvent.functionName, animEvent.objectReferenceParameter, SendMessageOptions.DontRequireReceiver);
    }
    public void StopManual()
    {
        Debug.Log("StopPlayableAnime");
        if (_manualRoutine != null)
        {
            StopCoroutine(_manualRoutine);
            _manualRoutine = null;
        }

        if (_clip.IsValid())
        {
            _clip.Destroy();
        }

        if (_layerMixer.GetInputCount() > 1 && _layerMixer.GetInput(1).IsValid())
            _layerMixer.SetInputWeight(1, 0f);

        if (_layerMixer.GetInputCount() > 0 && _layerMixer.GetInput(0).IsValid())
            _layerMixer.SetInputWeight(0, 1f);
    }
    public void StopClip() 
    {
        if (_clip.IsValid()) _clip.Pause();
        if (_graph.IsValid()) _graph.Stop();
    }
    public void StartClip()
    {
        if (_clip.IsValid()) _clip.Play();
        if (_graph.IsValid()) _graph.Play();

    }
    public void EndManual()//playableアニメーション終了処理
    {
        if (_manualRoutine != null)
        {
            StopCoroutine(_manualRoutine);
            _manualRoutine = null;
        }
        if (_fadeCoroutine != null) StopCoroutine(_fadeCoroutine);
        _fadeCoroutine = StartCoroutine(FadeBackToBase());
    }


    private IEnumerator FadeBackToBase()//なめらかにノーマルアニメーションに戻す
    {
        float elapsed = 0f;
        float startWeight = _layerMixer.GetInputWeight(1);

        while (elapsed < fadeDuration)
        {
            //レイヤーミキサーが稼働していなかったら流さない
            if (!_layerMixer.IsValid())
                yield break;

            elapsed += Time.deltaTime;
            float t = elapsed / fadeDuration;

            //なめらかにノーマルアニメーションに戻す
            _layerMixer.SetInputWeight(1, Mathf.Lerp(startWeight, 0f, t));
            //   _layerMixer.SetInputWeight(0, Mathf.Lerp(1f - startWeight, 1f, t));

            yield return null;
        }

        _layerMixer.SetInputWeight(1, 0f);
        _layerMixer.SetInputWeight(0, 1f);

        if (_clip.IsValid())
        {
            _clip.Destroy();
            _clip = default;
        }
        _fadeCoroutine = null;

    }

    private void OnDestroy()
    {
        //稼働していたら捨てる
        if (_graph.IsValid())
        {
            _graph.Destroy();
        }
    }
}
