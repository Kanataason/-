using System;
using System.Collections.Generic;
using UnityEngine;

public class TatuCollderManager : MonoBehaviour
{
    private TatuCollderManager Opponent;//相手

    private ColliderManager colliderManager;

    public List<Colliders> CollManager = new List<Colliders>();

    //攻撃、当たり判定のコライダーを収納するクラス
    [Serializable]
    public class ColliderInfo
    {
        public GameObject ParentCollider;
        public ColliderType[] ChildrenCollider;
    }

    //各やられ判定、ヒット判定、投げ判定を収納する
    private Dictionary<string, ColliderInfo> hurtBoxGroups = new Dictionary<string, ColliderInfo>();

    private Dictionary<string, ColliderInfo> HitBox = new Dictionary<string, ColliderInfo>();
    private Dictionary<string, ColliderInfo> HurtBox = new Dictionary<string, ColliderInfo>();
    private Dictionary<string, ColliderInfo> ThrowBox = new Dictionary<string, ColliderInfo>();

    private TatsuAnimationController controller;
    private PlayerController playerController;
    private PlayerKnockBackController backMotion;
    private PlayerState playerState;
    private BaseActionController actionController;

    private Dictionary<int, FrameData> frameLookup;//フレームの変わり目入れるリスト
    
    //現在のやられ判定
    private List<ColliderType> currentActiveHurtBox = new();
    private List<ColliderType> currentActiveHitBox = new();
    private List<ColliderType> currentActiveThrowBox = new();

    private bool isPlayed = false;//コライダー更新時一回だけ処理をするためのフラグ
    private bool isPlayerKnockBack = false;//trueの時だけ動くようにするフラグ
    private bool isCanClearList = false;

    private int totalFrame;
    private void Awake()
    {
        InitList();
    }
    private void InitList()
    {
        //リストからアタックコライダーを分けてリストに保存
        foreach (var coll in CollManager)
        {
            if (!hurtBoxGroups.ContainsKey(coll.ColliderName))
            {
                if (coll.G_Collider == null) continue;

                coll.G_Collider.SetActive(false);
                hurtBoxGroups.Add(coll.ColliderName, new ColliderInfo()
                {
                    ParentCollider = coll.G_Collider,
                    ChildrenCollider = coll.G_Collider.GetComponentsInChildren<ColliderType>(),
                });
            }
            foreach (var Acoll in coll.attackColliders)
            {
                //コライダーには子が確定でついているからnullチェックは不要
                if (Acoll.HitBox != null)
                {
                    HitBox[Acoll.HitBox.name] = new ColliderInfo()
                    {
                        ChildrenCollider = Acoll.HitBox.GetComponentsInChildren<ColliderType>(),
                        ParentCollider = Acoll.HitBox
                    };
                }

                if (Acoll.HurtBox != null)
                {
                    HurtBox[Acoll.HurtBox.name] = new ColliderInfo()
                    {
                        ChildrenCollider = Acoll.HurtBox.GetComponentsInChildren<ColliderType>(),
                        ParentCollider = Acoll.HurtBox
                    };
                }

                if (Acoll.ThrouBox != null)
                {
                    ThrowBox[Acoll.ThrouBox.name] = new ColliderInfo()
                    {
                        ChildrenCollider = Acoll.ThrouBox.GetComponentsInChildren<ColliderType>(),
                        ParentCollider = Acoll.ThrouBox
                    };
                }
            }
        }
    }
    void Start()
    {
        TryGetComponents();
        SubscribeEvents();
    }
    private void TryGetComponents()
    {
        colliderManager = GetComponent<ColliderManager>();
        actionController = GetComponent<BaseActionController>();
        playerState = GetComponent<PlayerState>();
        backMotion = GetComponent<PlayerKnockBackController>();
        controller = GetComponent<TatsuAnimationController>();
        playerController = GetComponent<PlayerController>();
    }
    private void SubscribeEvents()//イベント登録
    {
        if (playerController == null) return;

        playerController.OnStopAction += ResetColliderInfo;
        playerController.OnActiveCollisionAction += UpdateCollision;

        actionController.OnActiveCollisionAction += UpdateCollision;
    }
    private void UnSubscribeEvents()//イベント解除
    {
        if (playerController == null) return;

        playerController.OnStopAction -= ResetColliderInfo;
        playerController.OnActiveCollisionAction -= UpdateCollision;

        actionController.OnActiveCollisionAction -= UpdateCollision;
    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }

    //試合が始まったらセットされる
    public void SetOpponentAndSubscribeEvent(TatuCollderManager opponent)
    {
        Opponent = opponent;
    }
    //コライダーを捜して更新
    private void UpdateCollision(string colliderName, bool IsThrow = false)
    {
        ActiveHurtCollider(colliderName);
    }
    // Update is called once per frame
    void Update()
    {
        if (Opponent == null) return;

       colliderManager.CheckPushBox(Opponent.colliderManager);

        //押し出し確認
        if (CanPushPlayer()) return;

        transform.position += colliderManager.ConsumePushMove();
    }
    private bool CanPushPlayer()
    {
        //相手が攻撃をしていて、自分が倒れていたら押し出しをしない
        return Opponent.playerController.PlayerActionState is PlayerAction.Attack &&
            playerController.PlayerActionState is PlayerAction.Inoperable;
    }
    public void ActiveHurtCollider(string state)
    {
        //引数で渡されたコライダーを捜してアクティブにする
        foreach (var coll in hurtBoxGroups.Values)
        {
            foreach (var chilcoll in coll.ChildrenCollider)
            {
                //現在のやられ判定を消す
                if (currentActiveHurtBox.Contains(chilcoll))
                {
                    currentActiveHurtBox.Remove(chilcoll);
                }
            }
            coll.ParentCollider.gameObject.SetActive(false);
        }
        if (hurtBoxGroups.TryGetValue(state, out var obj))
        {
            obj.ParentCollider.gameObject.SetActive(true);
            //やあられ判定を追加
            foreach (var chillcoll in obj.ChildrenCollider)
                currentActiveHurtBox.Add(chillcoll);
        }
        else
        {
            Debug.Log("見つからない");
        }
    }

    private void PrepareFrameLookup(TatuAttackData data)//データに設定したフレームデータをリストにセット
    {
        int firstFrame = 1;

        frameLookup ??= new Dictionary<int, FrameData>();

        frameLookup.Clear();

        frameLookup[firstFrame] = data.frameData[0];

        foreach (var f in data.frameData)
        {
            frameLookup[f.FrameNumber] = f;
        }
    }
    //毎フレーム管理する
    public void InitColliderInfo(TatuAttackData data)
    {
        isPlayed = false;
        SetKnockBackFlag();

        PrepareFrameLookup(data);
    }

    //当たり判定をフレームごとに処理する関数
    public void UpdateColliders(TatuAttackData data,int currentIndex)
    {
        if (Time.timeScale == 0||isPlayed) return;

        if (playerController.IsHit) return;


        int totalFrameTime = data.frameData[^1].FrameNumber;

        int currentFrame = currentIndex;

        //もし特定のフレームに動ける攻撃なら動かす
        if (totalFrame != 0&&!GetknockBackFlag())
        {
            transform.position = backMotion.OnStraightMove(totalFrame);
        }

        if (frameLookup.TryGetValue(currentFrame, out FrameData frame))
        { 
            ApplyFrame(frame,data);
        }
        if (currentFrame >= totalFrameTime)
        {
            frameLookup.Clear();
            isPlayed = true;
            InitPlayerState();
            return;
        }
    }
    public List<ColliderType> GetCurrentHurtBoxesList()//現在のやられ判定リストを渡す
    {
        return currentActiveHurtBox;
    }

    public void StopCollider() 
    {
        //ヒットボックスを消す
        DisableBoxColliders(currentActiveHitBox);

    }
    public void ClearThrowBox()
    {
        //投げの処理をしている時は消さない
        if (!isCanClearList)
        DisableBoxColliders(currentActiveThrowBox);
    }
    public void ClearHurtBox()
    {
        //子を消さずにアクティブにする
        DisableBoxColliders(currentActiveHurtBox);
    }
    public void SetKnockBackFlag(bool isKnockBack = false) => isPlayerKnockBack = isKnockBack;
    public bool GetknockBackFlag() => isPlayerKnockBack;

    //相手を取る関数
    public TatsuAnimationController GetOpponent() { return Opponent.controller; }
    public TatuCollderManager GetNormalOpponent() { return Opponent; }


    private void InitPlayerState()//初期化処理
    {
        backMotion.ResetMoveInfo();
        SetKnockBackFlag();
        totalFrame = 0;

        playerController.ChangeCancelActionState(AcceptInput.Normal);

        //当たり判定を消す
        DisableBoxColliders(currentActiveHitBox);
        DisableBoxColliders(currentActiveThrowBox);
        DisableBoxColliders(currentActiveHurtBox);
    }

    private void ApplyFrame(FrameData frame,TatuAttackData data)
    {
        //技がキャンセル可能な状態か確認
        CheckAndChangeAttackState(frame);

        //現在のフレーム中にコライダーを出すか確認

        CheckActiveCollider(HitBox, frame.enableHitBoxes, true);
        CheckActiveCollider(HitBox, frame.disableHitBoxes);

        CheckActiveCollider(HurtBox, frame.enableHurtBoxes, true);
        CheckActiveCollider(HurtBox, frame.disableHurtBoxes);

        CheckActiveCollider(ThrowBox, frame.enableThrowBoxes, true);
        CheckActiveCollider(ThrowBox, frame.disableThrowBoxes);

        //ここで動くかどうかを見る 毎フレーム動かないから相手が動かさないといけない

        UpdateAttackMovement(frame);

        bool isOpponent = Opponent != null && Opponent.colliderManager != null;

        CanHit(frame, isOpponent, data);

        CanThrow(frame,isOpponent,data);

        if (frame.flags.HasFlag(ActionFlags.PanishCounter))
            playerState.ChangeState(PlayerTranstionState.PanishCounter);
    }
    private void CanHit(FrameData frame,bool isOpponent,TatuAttackData data)
    {
        //ヒットしたか判定をする
        if (frame.flags.HasFlag(ActionFlags.Hit) && isOpponent)
        {
            //敵の参照と現在アクティブコライダーを渡す。
            var opponentHurtList = Opponent.GetCurrentHurtBoxesList();

            bool isAttackApply = colliderManager.CheckColliderHit(Opponent.colliderManager,
                 currentActiveHitBox, opponentHurtList, data);

            if (isAttackApply)
            {
                SetKnockBackFlag(true);
            }
        }
    }

    private void CanThrow(FrameData frame,bool isOpponent,TatuAttackData data)
    {
        //投げの判定をする
        if (frame.flags.HasFlag(ActionFlags.Throw) && isOpponent)
        {
            //投げフラグをオンにする
            isCanClearList = true;
            colliderManager.CheckThrowHit
                (Opponent.colliderManager, currentActiveThrowBox, data, GetOpponent());
        }
        else
        {
            isCanClearList = false;
        }
        if (frame.flags.HasFlag(ActionFlags.HitBoxActive))
        {
            ActiveHurtCollider("Idle");
        }
    }
    /// <summary>
    /// 
    /// </summary>
    /// <param name="frame"></param>
    private void UpdateAttackMovement(FrameData frame)//技を出している時に動けるかを確認
    {
        if (frame.MoveFrameData.IsStartMoveing)
        {
            //もしうしろ向きなら-1を渡す
            var isback = frame.MoveFrameData.IsBack?-1:1;
            //どちらに敵が居るかを知るだけだから内積を使わずに向きを取得
            var dir = Opponent.transform.position.x > transform.position.x ? 1 : -1;
            //合わせる
            var resultDir = Vector3.right * (dir * isback);

            totalFrame = frame.MoveFrameData.MoveFrame;
            
            //方向と力を渡して移動のスタート地点と終点を決める
            backMotion.SetMoveInfo(Opponent.transform,resultDir, frame.MoveFrameData.Movetorque);
        }
        if (frame.MoveFrameData.IsEndMoveing)
        {
            SetKnockBackFlag(true);
        }
    }
    private void CheckActiveCollider(Dictionary<string,ColliderInfo> colliderList,
        List<string> colliderData, bool isActive = false)
    {
        //データベースから設定した名前を取ってリストに存在するか確認
        foreach (var name in colliderData)
        {
            if (!colliderList.TryGetValue(name, out var obj))
            {
                continue;
            }

            if (isActive)
            {
               
                foreach (var coll in obj.ChildrenCollider)
                {
                    //タイプにあったリストに当たり判定が入っていなかったらリストに追加
      
                    if (coll.Types is ColliderTypes.HitBox && !currentActiveHitBox.Contains(coll))
                        currentActiveHitBox.Add(coll);

                    if (coll.Types is ColliderTypes.HurtBox && !currentActiveHurtBox.Contains(coll))
                        currentActiveHurtBox.Add(coll);

                    if (coll.Types is ColliderTypes.ThrowBox && !currentActiveThrowBox.Contains(coll))
                        currentActiveThrowBox.Add(coll);
                }

            }
            else
            {
                //リストに存在していたら消す
                foreach (var coll in obj.ChildrenCollider)
                {
                    currentActiveHurtBox.Remove(coll);
                    currentActiveHitBox.Remove(coll);
                    currentActiveThrowBox.Remove(coll);
                }
            }
            obj.ParentCollider.SetActive(isActive);
        }
    }
    private void CheckAndChangeAttackState(FrameData data)
    {
        switch (data.InputState)
        {
            //行動不可
            case AcceptInput.Disable:
                playerController.ChangeCancelActionState(AcceptInput.Disable); break;
            //攻撃のキャンセル可能
            case AcceptInput.AttackCancellable:
                playerController.ChangeCancelActionState(AcceptInput.AttackCancellable); break;
            //必殺技のキャンセル可能
            case AcceptInput.SmashCancellable:
                playerController.ChangeCancelActionState(AcceptInput.SmashCancellable); break;
            default: break;
        }
    }

    /// <summary>
    ///渡されたリストの中身をクリアにしてアクティブにするかしないかを処理する関数
    /// </summary>
    /// <param name="boxlist"></param>
    private void DisableBoxColliders(List<ColliderType> boxlist)
    {
        boxlist.Clear();
    }
    

    //------------AnimationEvent-------------------//

    public void ResetColliderInfo()
    {
        InitPlayerState();
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
