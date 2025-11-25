using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Playables;
[RequireComponent(typeof(Animator))]
public class MakePlayable : MonoBehaviour
{
    public Animator animator;

    private PlayableGraph _graph;
    private AnimationLayerMixerPlayable _layerMixer; // ★変更
    private AnimationClipPlayable _clip;

   public AvatarMask mask; // ★AvatarMaskを指定
   public AvatarMask Normal;
    [SerializeField] private float fadeDuration = 0.08f;

    private Coroutine _manualRoutine;
    private Coroutine _fadeCoroutine;
    private TatsuPlayerController playerController;
    private Coroutine _transitionCoroutine;
    private PlayerState playerState;
    public bool IsThrown = false;

    private void OnValidate()
    {
        if (!animator) animator = GetComponent<Animator>();
    }

    void Start()
    {
        StartCoroutine(Init());
    }

    IEnumerator Init()
    {
        yield return null;
        playerController = GetComponent<TatsuPlayerController>();
        playerState = GetComponent<PlayerState>();
        // PlayableGraph 初期化
        _graph = PlayableGraph.Create($"{gameObject.name}-graph");
        var output = AnimationPlayableOutput.Create(_graph, "AnimationOutput", animator);

        // ★ AnimationLayerMixerPlayable を使う（2入力）
        _layerMixer = AnimationLayerMixerPlayable.Create(_graph, 3);
        output.SetSourcePlayable(_layerMixer);

        // AnimatorController を Input0 に接続
        var controllerPlayable = AnimatorControllerPlayable.Create(_graph, animator.runtimeAnimatorController);
        _layerMixer.ConnectInput(0, controllerPlayable, 0);
        _layerMixer.SetInputWeight(0, 1f);

        // AnimatorController を切り離し
        animator.runtimeAnimatorController = null;

        // ★ Mask を Input1 に設定しておく（初期は空）
        _layerMixer.SetLayerMaskFromAvatarMask(1, mask);

        _graph.Play();
    }
    public void PlayAtNormalizedTime(float t, AnimationClip clip)
    {
        if (!_clip.IsValid())
        {
            Debug.Log("ssss");
            _clip = AnimationClipPlayable.Create(_graph, clip);
            _clip.SetDuration(clip.length);
            _clip.Pause();

            // Mixer に接続（Input1に入れる）
            _layerMixer.ConnectInput(1, _clip, 0);
            _layerMixer.SetInputWeight(0, 0f); // ベースモーション常に有効
            _layerMixer.SetInputWeight(1, 1f);
            _layerMixer.SetLayerMaskFromAvatarMask(1, Normal);
        }

        float duration = clip.length;
        float currentTime = Mathf.Clamp01(t) * duration;
        _clip.SetTime(currentTime);

        // AnimationEvent 再現（tに応じたものだけ発火）
        var events = clip.events;
        for (int i = 0; i < events.Length; i++)
        {
            if (events[i].time <= currentTime)
            {
                ExecuteAnimationEvent(events[i]);
            }
        }
        if (currentTime >= duration)
        {
            Debug.Log("sssssss");
            EndManual();
        }
    }
    /// <summary>
    /// 攻撃アニメーションを手動再生（途中キャンセル対応）
    /// </summary>
 public void TransitionTo(AnimationClip nextClip, int totalFrames, float fadeTime, AvatarMask mask = null)
    {
        if (_manualRoutine != null)
        {
            StopCoroutine(_manualRoutine);
            _manualRoutine = null;
        }
        if (mask == null) mask = Normal;

    var nextPlayable = AnimationClipPlayable.Create(_graph, nextClip);
    nextPlayable.SetDuration(nextClip.length);
    nextPlayable.Pause();

    int nextIndex = 2; // Input2を使用
    _layerMixer.ConnectInput(nextIndex, nextPlayable, 0);
    _layerMixer.SetInputWeight(nextIndex, 0f);
    _layerMixer.SetLayerMaskFromAvatarMask((uint) nextIndex, mask);

    if (_transitionCoroutine != null) StopCoroutine(_transitionCoroutine);
    _transitionCoroutine = StartCoroutine(TransitionRoutine(nextPlayable, nextIndex, nextClip, totalFrames, fadeTime));
}

private IEnumerator TransitionRoutine(AnimationClipPlayable nextPlayable, int nextIndex, AnimationClip clip, int totalFrames, float fadeTime)
{
    float timer = 0f;

    // 新しいアニメを手動再生開始
    _manualRoutine = StartCoroutine(ManualPlayRoutine(totalFrames, clip, nextPlayable));

    // フェード処理
    while (timer < fadeTime)
    {
        timer += Time.deltaTime;
        float t = timer / fadeTime;

        _layerMixer.SetInputWeight(1, 1f - t); // 古いアニメを下げる
        _layerMixer.SetInputWeight(nextIndex, t); // 新しいアニメを上げる

        yield return null;
    }

    // 古いアニメを切断して新しい方だけ残す
    _layerMixer.SetInputWeight(1, 0f);
        _layerMixer.SetInputWeight(0,1f);
        _layerMixer.DisconnectInput(1);
    _clip = nextPlayable;
}
public void Play(AnimationClip clip, int totalFrames,AvatarMask mask=null)
    {
        StopManual();
        if (mask == null) mask = Normal;
        if (_fadeCoroutine != null)
        {
            StopCoroutine(_fadeCoroutine);
            _fadeCoroutine = null;
            _layerMixer.SetInputWeight(1, 0f);
            _layerMixer.SetInputWeight(0, 1f);
        }

        _clip = AnimationClipPlayable.Create(_graph, clip);
        _clip.SetDuration(clip.length);
        _clip.Pause();

        // Mixer に接続（Input1に入れる）
        _layerMixer.ConnectInput(1, _clip, 0);
        _layerMixer.SetInputWeight(0, 1f); // ベースモーション常に有効
        _layerMixer.SetInputWeight(1, 1f);
        
            _layerMixer.SetLayerMaskFromAvatarMask(1, mask);


        // 手動再生開始
        _manualRoutine = StartCoroutine(ManualPlayRoutine(totalFrames, clip));
    }
    private IEnumerator ManualPlayRoutine(int totalFrames, AnimationClip clip, AnimationClipPlayable playable = default)
    {
        if (!playable.IsValid()) playable = _clip;

        float duration = clip.length;
        float frameTime = duration / totalFrames;

        var events = clip.events;
        int eventIndex = 0;

        for (int i = 0; i < totalFrames; i++)
        {
            if (!playable.IsValid()) yield break;

            float currentTime = i * frameTime;
            playable.SetTime(currentTime);

            while (eventIndex < events.Length && events[eventIndex].time <= currentTime)
            {
                ExecuteAnimationEvent(events[eventIndex]);
                eventIndex++;
            }

            yield return new WaitForSeconds(1f / 60f);
        }

            EndManual();
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
    public float GetClipTime()
    {
        return (float)_clip.GetTime();
    }
    public void StopClip() { if (_clip.IsValid()) _clip.Pause(); }
    public void EndManual()
    {

        if (_manualRoutine != null)
        {
            StopCoroutine(_manualRoutine);
            _manualRoutine = null;
        }
        if (_fadeCoroutine != null) StopCoroutine(_fadeCoroutine);
        _fadeCoroutine = StartCoroutine(FadeBackToBase());
    }


    private IEnumerator FadeBackToBase()
    {
        float elapsed = 0f;
        float startWeight = _layerMixer.GetInputWeight(1);
        if (IsThrown)
        {
            IsThrown = false;
            playerController.AttackReset();
            playerController.IsThrown = false;
            playerController.IsHit = false;
        }
        while (elapsed < fadeDuration)
        {
            elapsed += Time.deltaTime;
            float t = elapsed / fadeDuration;

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
        if (_graph.IsValid())
        {
            _graph.Destroy();
        }
    }
}
