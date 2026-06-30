using System.Collections.Generic;
using UnityEngine;

public class ColliderManager : MonoBehaviour
{
 public enum ColliderTypeName { HitBox,HurtBox,ThrowBox,ThrowHurtBox}
    public BoxCollider PushBox;
    public BoxCollider ThrowHurtBox;

    //ボックスのデータ
    [System.Serializable]
    public class HitInfo
    {
        public Vector3 hitPos;
        public ColliderManager targetManager;
        public PlayerHitDirector hitDirector;
        public TatuAttackData hitBoxData;
    }

    private PlayerState state;
    [SerializeField] private float SetPushPower = 2.2f;
    [SerializeField] private int throwTechInputFrames = 16;
 
    //参照
    private PlayerController playerController;
    private BaseActionController actionController;
    private PlayerHitDirector hitDirector;

    private Vector3 PushMoveDelta;
    private bool isThrowing = false;

    private void Start()
    {
        TryGetComponents();
        SubscribeEvents();
    }
    /// <summary>
    /// 投げ判定のOn,Offの変更通知アクションを登録
    /// </summary>
    private void SubscribeEvents()
    {
        if (playerController != null)
            playerController.OnActiveCollisionAction += ActiveThrow;

        if (actionController != null)
            actionController.OnActiveCollisionAction += ActiveThrow;
    }
    /// <summary>
    /// 投げ判定のOn,Offの変更通知アクションを解除
    /// </summary>
    private void UnSubscribeEvents()
    {
        if (playerController != null)
            playerController.OnActiveCollisionAction -= ActiveThrow;

        if (actionController != null)
            actionController.OnActiveCollisionAction -= ActiveThrow;

    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }

    private void TryGetComponents()
    {
        actionController = GetComponent<BaseActionController>();
        hitDirector = GetComponent<PlayerHitDirector>();
        state = GetComponent<PlayerState>();
        playerController = GetComponent<PlayerController>();
    }
    /// <summary>
    /// 投げ判定のOn Off
    /// </summary>
    private void ActiveThrow(string name,bool IsThrow)
    {
        GameObject obj = ThrowHurtBox.gameObject;

        if (obj.activeSelf != IsThrow)
        {
            obj.SetActive(IsThrow);
        }
    }

    /// <summary>
    /// 相手にヒットしたかガードしたかを知らせる
    /// </summary>
    /// <param name="hitInfo"></param>
    /// <param name="hitState"></param>
    public void AttackHit(HitInfo hitInfo,
      PlayerHitDirector.HitResult hitState = PlayerHitDirector.HitResult.Guard)
    {

        if (hitState is PlayerHitDirector.HitResult.Hit)
        {
            hitDirector.ReceiveHit(hitInfo);
        }
        else
        {
            hitDirector.ReceiveGuard(hitInfo);
        }
    }

    /// <summary>
    /// 通常技専用の判定
    /// </summary>
    public bool CheckColliderHit(ColliderManager targetManager,List<ColliderType> activeList,
        List<ColliderType> targetHurtList, TatuAttackData attackData)//当たり判定 相手のstate
    {
        if (GetThrowingFlag()) return false;

            foreach (var hit in activeList)
            {
            var hitBounds = hit.MyCollider.bounds;
            foreach(var hurt in targetHurtList)
            {
                var hurtBounds = hurt.MyCollider.bounds;
                //コライダー同士が重なっていなかったら
                if (!hurtBounds.Intersects(hitBounds)) continue;

                float offset = 0.5f;
                Vector3 hitPos = (hurtBounds.ClosestPoint(hitBounds.center) +
                                hitBounds.ClosestPoint(hurtBounds.center)) * offset;

                //最初の攻撃かを見る
                bool isHitStop = targetManager.state.ReceiveHitCount > 0 ? false : true;

                //ガードしているか確認
                if (targetManager.state.CanGuard(attackData.attackType))
                {
                    //ガード成功
                    //ガードされた側
                    hitDirector.OnAttackGuard(new HitInfo()
                    {
                        hitBoxData = attackData,
                        targetManager = targetManager
                    });

                    //ガードした側
                    targetManager.AttackHit(new HitInfo()
                    {
                        hitBoxData = attackData,
                        hitPos = hitPos,
                        targetManager = this

                    });

                    //どちらもヒットストップをオンにする
                    playerController.SetHitStopFlag(isHitStop);
                    targetManager.playerController.SetHitStopFlag(isHitStop);
                    return true;
                }
                //攻撃側
                hitDirector.OnAttackHit(new HitInfo()
                {
                    hitBoxData = attackData,
                    targetManager = targetManager
                    
                });

                //攻撃を受けた側
                targetManager.AttackHit(new HitInfo()
                {
                    hitBoxData = attackData,
                    hitPos = hitPos,
                    targetManager = this

                },PlayerHitDirector.HitResult.Hit);

                //どちらもヒットストップをオンにする
                playerController.SetHitStopFlag(isHitStop);
                targetManager.playerController.SetHitStopFlag(isHitStop);
                return true;
            }
            //ここで送られてきたコライダーを取得して当たり判定をする
        }
        return false;
    }

    /// <summary>
    /// 投げ専用の判定
    /// </summary>
    public void CheckThrowHit(ColliderManager targetManager,List<ColliderType> throwBoxList
        ,TatuAttackData data,TatsuAnimationController animationController)
    {
        if (GetThrowingFlag()) return;

        //アクティブじゃなかったら返す
        if (!targetManager.ThrowHurtBox.gameObject.activeInHierarchy) return;

        foreach (var throwBox in throwBoxList)
        {
            //コライダーの当たり判定
            var throwbox = throwBox.MyCollider.bounds;
            var targetThrowBox = targetManager.ThrowHurtBox.bounds;


            //当たり判定と、やられ判定が重なっているか確認
            if (throwbox.Intersects(targetThrowBox))
            {

                //投げた側の処理
                hitDirector.OnThrowHit(animationController,throwTechInputFrames,new HitInfo()
                {
                    hitBoxData = data,
                    targetManager = targetManager,
                    hitDirector = targetManager.hitDirector
                });

                //投げられた側の処理
                targetManager.hitDirector.ReceiveThrow(new HitInfo()
                {
                    hitBoxData = data,
                    targetManager = targetManager
                });

                SetThrowingFlag(true);
                targetManager.SetThrowingFlag(true);
                return;
            }
        }
    }
    public void CheckPushBox(ColliderManager target)
    {
        float MaxDistance = 3f;
        //ベクトルを取得
        Vector3 dir = transform.position - target.transform.position;

        //特定の範囲に入ったら処理開始
        if (dir.sqrMagnitude <= MaxDistance * MaxDistance)
        {
            //相手と自分の当たり判定を出す
            var mybounds = PushBox.bounds;
            var otherbounds = target.PushBox.bounds;

            //当たり判定が重なっていたら判定
            if (mybounds.Intersects(otherbounds))
            {
                Vector3 centerDiff = mybounds.center - otherbounds.center;

                //重なり具合を見る
                float overlap =
                (mybounds.extents.x + otherbounds.extents.x)
                - Mathf.Abs(centerDiff.x);

                if (overlap > 0)
                {
                    //押し出し方向
                    float sign = transform.position.x < target.transform.position.x ? -1 : 1;

                    float offset = 0.5f;
                    
                    float push = overlap * offset / SetPushPower;

                    Vector3 pushVector = Vector3.right * push * sign;

                    SetMoveDelta(pushVector);
                }
            }
        }
    }

    /// <summary>
    /// 投げ判定のOn、Offを切り替える
    /// </summary>
    /// <param name="IsActive"></param>
    public void ThrowActive(bool IsActive)
    {
        ThrowHurtBox.gameObject.SetActive(IsActive); 
    }


    //押された移動量を取得、セットする
    public void SetMoveDelta(Vector3 delta) => PushMoveDelta = delta;
    public Vector3 ConsumePushMove()
    {
        var movedelta = PushMoveDelta;
        PushMoveDelta = Vector3.zero;
        return movedelta;
    }
    
    //投げのフラグや投げを取得、セットする
    public void SetThrowingFlag(bool isThrow = false) => isThrowing = isThrow;
    public bool GetThrowingFlag() => isThrowing;
}
