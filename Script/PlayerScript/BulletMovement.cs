using System.Collections.Generic;
using UnityEngine;

public class BulletMovement : MonoBehaviour
{
    public float Velocity = 10;//速度
    public Vector3 Direction = Vector3.forward;//進行方向

    [SerializeField] private float Distance = 2;//進行距離

    [SerializeField] private int MaxCount = 1;//ヒット回数
    private bool isMove = false;

    private ColliderManager ownerManager;//攻撃をした人
    private ColliderManager targetManager;//攻撃される人
    private TatuCollderManager targetCollderManager;//攻撃される人のやられ判定を取る用

    private List<ColliderType> hitBoxList = new();//攻撃のヒットボックス

    private TatuAttackData currentAttackData;

    private int currentHitCount = 0;//現在のヒット回数
    /// <summary>
    /// 誰が攻撃して誰を狙っているのかをセットする関数
    /// </summary>
    /// <param name="target"></param>
    /// <param name="targetCollderManager"></param>
    /// <param name="ownerColliderManager"></param>
    /// <param name="data"></param>
    public void SetReference(ColliderManager target, TatuCollderManager targetCollderManager,
        ColliderManager ownerColliderManager,TatuAttackData data)
    {
        currentAttackData = data;//攻撃データ
        ownerManager = ownerColliderManager;//攻撃をした人
        targetManager = target;//攻撃対象
        this.targetCollderManager = targetCollderManager;//
    }
    /// <summary>
    /// 弾自体のパラメーターをセットする関数
    /// </summary>
    /// <param name="velocity"></param>
    /// <param name="dir"></param>
    public void SetBulletParameter(float velocity,Vector3 dir)
    {
        Velocity = velocity;//速度
        Direction = dir.normalized;//進む方向

        currentHitCount = MaxCount;//ヒットカウントを復活させる
        isMove = true;
    }
    private void OnEnable()
    {
        if (hitBoxList.Count >= 1) return;

        //ヒットボックスをリストに保存
        var hitBox = GetComponent<ColliderType>();
        hitBoxList.Add(hitBox);
    }
    private void OnDisable()
    {
        InitReference();

        currentHitCount = MaxCount;
    }
    private void InitReference()
    {
        currentAttackData = null;
        ownerManager = null;
        targetManager = null;
        targetCollderManager = null;
    }
    void Update()
    {
        //direction方向に進行
        transform.position +=  Direction * Velocity * Time.deltaTime;
    }
    private void FixedUpdate()
    {
        if (!isMove) return;

        //相手との方向ベクトルを取る
        var dir = targetManager.transform.position - transform.position;

        //長さを出してその長さが当たり判定に入っていたら処理開始
        if (dir.sqrMagnitude < Distance * Distance)
        {
            //ヒットカウントが０になったら判定しないようにする
            if (currentHitCount <= 0) return;

            var hurtList = targetCollderManager.GetCurrentHurtBoxesList();

            //当たっていたらヒットカウントを減らす
          if (ownerManager.CheckColliderHit(targetManager,
                hitBoxList, hurtList, currentAttackData))
            {
                currentHitCount--;
            }

        }
    }
    private void OnTriggerEnter(Collider other)
    {
      //  if (ColliderManager == null) return;
      //  var state = other.GetComponentInParent<PlayerState>();
      //  if (state == null) return; // プレイヤー以外は無視

      //  var num = state.PlayerNumber;
      //  if (num == Number) return;        // 自分自身は無視
      //  if (ints.Contains(num)) return; // 既に当たってるなら無視

      //  if (state.currentTranstionState == PlayerTranstionState.PlayerDown) return;
      //  ints.Add(num);
      //  if (state.currentTranstionState == PlayerTranstionState.CrouchGuard || state.currentTranstionState == PlayerTranstionState.StandGuard)
      //  {
      //      var colls = other.GetComponentInParent<ColliderManager>();
      //       colls.Stiffnes(colls);
      //    //  other.GetComponentInParent<TatsuAnimationController>().
      //     //     PlayCheckHitOrGuard(other.ClosestPoint(transform.position), TatsuAnimationController.AnimationId.Guarding);
      //      return;
      //  }
      //  state.ChangeState(PlayerTranstionState.Flying);
      //  other.GetComponentInParent<TatsuPlayerController>().IsHit = true;
      //  var s = other.GetComponentInParent<TatsuAnimationController>();
      //  var coll =  other.GetComponentInParent<ColliderManager>();
      //  // coll.Stiffnes(coll);
      //  var data = s.GetAttackData(TatsuAnimationController.AnimationId.Smash);
      ////  coll.CheckAttackAttribute(AttackAttribute.LaunchAtack,data, other.ClosestPoint(transform.position),coll);
      //  //  other.GetComponentInParent<TatuCollderManager>().Lunching();
      //  // s.Hit(other.ClosestPoint(transform.position), "Hit"); 
      //  //  Debug.Log($"Hit Player {num}: {other.gameObject.name}");
      //  var damage = OnDamage(3000, data.damageReductionPerHit, state);
      //  damage *= Mathf.Max(0f, 1f - state.Correction);
      //  state.TakeDamage(damage, 0);

    }

}
