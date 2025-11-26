using NUnit.Framework;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TatuCollderManager : MonoBehaviour
{
    [SerializeField] private Transform Player;
    public TatuCollderManager Opponent;
  //  [SerializeField] private Animator animator;
    [SerializeField] private ColliderManager colliderManager;

    public List<Colliders> CollManager = new List<Colliders>();

    private Dictionary<string, GameObject> MyBox = new Dictionary<string, GameObject>();

    private Dictionary<string, GameObject> HitBox = new Dictionary<string, GameObject>();
    private Dictionary<string, GameObject> HurtBox = new Dictionary<string, GameObject>();
    private Dictionary<string, GameObject> ThrouBox = new Dictionary<string, GameObject>();

    private TatsuAnimationController controller;
    private TatsuPlayerController playerController;
    private KnockBackMotion backMotion;
    private Coroutine coroutine;
    public PlayerState playerState;
    private void Awake()
    {
        foreach (var coll in CollManager)
        {
            if (!MyBox.ContainsKey(coll.ColliderName))
            {
                coll.G_Collider.SetActive(false);
                MyBox.Add(coll.ColliderName, coll.G_Collider);
            }
            foreach (var Acoll in coll.attackColliders)
            {
                if (Acoll.HitBox != null) HitBox[Acoll.HitBox.name] = Acoll.HitBox;
                if (Acoll.HurtBox != null) HurtBox[Acoll.HurtBox.name] = Acoll.HurtBox;
                if (Acoll.ThrouBox != null) ThrouBox[Acoll.ThrouBox.name] = Acoll.ThrouBox;
            }
        }
    }
    void Start()
    {
        playerState = GetComponent<PlayerState>();
        backMotion = GetComponent<KnockBackMotion>();
        controller = GetComponent<TatsuAnimationController>();
        playerController = GetComponent<TatsuPlayerController>();
    }

    // Update is called once per frame
    void Update()
    {
        transform.position = new Vector3(Player.position.x, Player.position.y, Player.position.z);
       if(Opponent != null) colliderManager.CheckPushBox(Opponent.colliderManager);
    }
    public void UpdateCollider(string state)
    {
        foreach (var coll in MyBox.Values)
        {
            coll.SetActive(false);
        }
        if (MyBox.TryGetValue(state, out GameObject obj))
        {
            obj.SetActive(true);
        }
        else
        {
            Debug.Log("Œ©‚Â‚©‚ç‚È‚¢");
        }
    }
    public void SetAttack(TatuAttackData data)
    {
        if (coroutine != null) StopCoroutine(coroutine);
        coroutine = StartCoroutine(PlayAttack(data));
    }
    private Dictionary<int, FrameData> frameLookup;

    private void PrepareFrameLookup(TatuAttackData data)
    {
        frameLookup = new Dictionary<int, FrameData>();
        foreach (var f in data.frameData)
        {
            frameLookup[f.FrameNumber] = f;
        }
    }
    public void StopCollider() 
    {
        DisableAllColliders();
        if (coroutine != null) StopCoroutine(coroutine);
        controller.AttackOff();
        coroutine = null;
    }
    public TatsuAnimationController GetOpponent() { return Opponent.controller; }
    public TatuCollderManager GetNormaOpponent() { return Opponent; }
    IEnumerator PlayAttack(TatuAttackData data)
    {
      //  colliderManager.ResetHitHistory();
        PrepareFrameLookup(data);
        int TotalFrameTime = data.Stiffness;

        for (int currentFrame = 1; currentFrame <= TotalFrameTime; currentFrame++)
        {
            if (playerController.IsHit) break;

            if (frameLookup.TryGetValue(currentFrame, out FrameData frame))
            {
                ApplyFrame(frame);
            }

            // Time.timeScale == 0 ‚ÌŠÔ‚ÍŽ~‚Ü‚é
            while (Time.timeScale == 0f)
            {
                yield return null;
            }

            yield return null;
        }
        Debug.Log("resetFlag");
            UpdateCollider("Idle");
            DisableAllColliders();
          if(playerState.PState != PLayerState.Flying)  controller.AttackOff();
            coroutine = null;
        
    }

    private void ApplyFrame(FrameData frame)
    {
        foreach (var name in frame.enableHitBoxes)
            if (HitBox.TryGetValue(name, out var obj)) obj.SetActive(true);

        foreach (var name in frame.disableHitBoxes)
            if (HitBox.TryGetValue(name, out var obj)) obj.SetActive(false);

        foreach (var name in frame.enableHurtBoxes)
            if (HurtBox.TryGetValue(name, out var obj)) obj.SetActive(true);

        foreach (var name in frame.disableHurtBoxes)
            if (HurtBox.TryGetValue(name, out var obj)) obj.SetActive(false);

        foreach (var name in frame.enableThrowBoxes)
            if (ThrouBox.TryGetValue(name, out var obj)) obj.SetActive(true);

        foreach (var name in frame.disableThrowBoxes)
            if (ThrouBox.TryGetValue(name, out var obj)) obj.SetActive(false);
        if (frame.IsCheckHit && Opponent != null && Opponent.colliderManager != null)
        {
            colliderManager.ThrowActive(false);
            colliderManager.CheckCollider(Opponent.colliderManager);
        }
        else
        {
            colliderManager.ThrowActive(true);
            colliderManager.ResetHitHistory();
        }
        if(frame.IsGraping && Opponent != null&&Opponent.colliderManager != null)
        {
            colliderManager.CheckThrowBox(Opponent.colliderManager);
        }
        else
        {
            Opponent.controller.OffThrowing();
            controller.OffThrowing();
        }
        if (frame.IsPanishCounter)playerState.ChengeState(PLayerState.PanishCounter);
    }
    private void DisableAllColliders()
    {
        foreach (var obj in HitBox.Values)
            obj.SetActive(false);

        foreach (var obj in HurtBox.Values)
            obj.SetActive(false);

        foreach (var obj in ThrouBox.Values)
            obj.SetActive(false);
    }

    //------------AnimationEvent-------------------//

    public void ResetAttackOff()
    {
        DisableAllColliders();
        if (coroutine != null) StopCoroutine(coroutine);
        coroutine = null;
    }
    public void KnockThrown()
    {
        Debug.Log("s");
        playerController.IsThrown = true;
        UpdateCollider("");
        var dir = Opponent.transform.position - transform.position;
        backMotion.FowerdOnMove(-dir, 1f, 2f, playerController);
    }
    public void Lunching()
    {
        var dir = Opponent.transform.position - transform.position;
        backMotion.Launch(-dir, 2, 1.5f, 1);
    }
    public void KnockThrowing()
    {
        Debug.Log("ss");
        var dir = Opponent.transform.position - transform.position;
        backMotion.FowerdOnMove(dir, 0.7f, 0.2f, playerController);
    }
}
[Serializable]
public class Colliders
{
    public string ColliderName;
    public GameObject G_Collider;
    public List<AttackCollider> attackColliders;
}
[Serializable]
public class AttackCollider
{
    public GameObject HitBox;
    public GameObject ThrouBox;
    public GameObject HurtBox;
}
