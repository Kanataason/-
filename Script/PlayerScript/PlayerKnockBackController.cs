using System;
using System.Collections;
using UnityEngine;

public class PlayerKnockBackController : MonoBehaviour
{
    const float GROUND_Y = -4.8f;

    public float gravity = -9.8f;     // 重力（マイナス値）

    private Camera cam;
    private Coroutine LaunchCoroutine;
    private Coroutine ForwardCoroutine;

    //場所を入れる変数
    private Vector3 moveStartPos;
    private Vector3 moveEndPos;

    //地面についたことを通知するためのイベント
    public event Action OnLanded;

    private int currentMoveFrame = 0;
    private void Start()
    {
         cam = Camera.main;
    }


    /// <summary>
    /// 引数のキャラクターが自分が画面はしかを見ている
    /// </summary>
    /// <param name="character"></param>
    /// <returns></returns>
    public bool CheckCanKnockBack(Vector3 character)
    {
        float margin = 0.5f;
        //カメラの奥行きを取得
        float zDist = character.z - cam.transform.position.z;

        (float left, float right) = GetCamerawidth(zDist, margin);

        float charX = character.x;

        //キャラが画面はしかどうか確認
        if (charX <= left)
        {
            Debug.Log("キャラが左端！");
            return true;
        }
        else if (charX >= right)
        {
            Debug.Log("キャラが右端！");
            return true;
        }
        return false;

    }
    public bool IsGoingOutOfBounds(float margin =1f)
    {
        //カメラの奥行きを取得
        float zDist = transform.position.z - cam.transform.position.z;

        (float left,float right) = GetCamerawidth(zDist, margin);

        //キャラがどっちを向いている)か確認
        float dot = Vector3.Dot(
            transform.forward.normalized,
            cam.transform.right.normalized);

        //左端に近くて左を向ていたらtrue
        if (transform.position.x <= left && dot < 0)
            return true;

        //右端に近くて右を向いていたらtrue
        if (transform.position.x >= right && dot > 0)
            return true;

        return false;
    }
    private (float left, float right) GetCamerawidth(float zDist,float margin)
    {
        //画面端の座標を取得
        Vector3 bottomLeft = cam.ViewportToWorldPoint(new Vector3(0, 0, zDist));
        Vector3 topRight = cam.ViewportToWorldPoint(new Vector3(1, 1, zDist));

        //マージン分端からの距離を開ける
        float left = bottomLeft.x + margin;
        float right = topRight.x - margin;

        return (left, right);
    }
    public void Launch(Vector3 direction, float power,float Height,float Duration)
    {
        //再生中のコルーチンがあれば止める
        if (LaunchCoroutine != null) StopCoroutine(LaunchCoroutine);
      LaunchCoroutine = StartCoroutine(FlyRoutine(direction, power,Height,Duration,GROUND_Y));
    }

    private IEnumerator FlyRoutine(Vector3 direction, float power, float height, float duration, float groundY)
    {
        direction.Normalize();//単位ベクトルにする
        Vector3 pos = transform.position;

        //縦方向のベクトルは他の変数で管理するから０に設定 
        Vector3 horizontalVelocity = new Vector3(direction.x, 0f, direction.z) * (power / duration);
        float verticalVelocity = height;

        float elapsed = 0f;

        while (elapsed < duration)
        {
            elapsed += Time.deltaTime;

            // 水平方向
            pos += horizontalVelocity * Time.deltaTime;

            // 垂直方向
            verticalVelocity += gravity * Time.deltaTime;

            //最初は勢い良く飛んで重力で落ちる
            pos.y += verticalVelocity * Time.deltaTime;

            // 地面に着いたら終了
            if (pos.y <= groundY)
            {
                pos.y = groundY;
                transform.position = pos;
                OnLanding(); // 空中コンボ終了
                break;
            }

            transform.position = pos;
            yield return null;
        }

        transform.position = new Vector3(pos.x, groundY, pos.z);
        LaunchCoroutine = null;
    }
    private void OnLanding()//ここの処理は終わったという通知にしてあっちで処理をさせる
    {
        OnLanded?.Invoke();//起き上がりモーションを再生
        Debug.Log("着地");
    }
    public void ForwardMove(Vector3 dir,float Duration,float Power)
    {
        //再生中のコルーチンがあれば止める
        if (ForwardCoroutine != null) StopCoroutine(ForwardCoroutine);
       ForwardCoroutine = StartCoroutine(KnockBack(dir,Duration,Power));
    }
    IEnumerator KnockBack(Vector3 d, float Duration, float Power)
    {
        //ベクトルを単位ベクトルに変換
        d.Normalize();

        Vector3 StartPos = transform.position;
        var startY = GROUND_Y;

        // 地面の高さに揃えてノックバック方向に飛ばす
        Vector3 EndPos = new Vector3(StartPos.x, startY, StartPos.z) + d * Power;

        float elapsed = 0f;
        float OffsetY = 0.01f;

        while (elapsed < Duration)
        {
            elapsed += Time.deltaTime;
            float t = elapsed / Duration;

            // 最初速く、最後遅くするイーズアウトを採用
            t = 1f - (1f - t) * (1f - t) * (1f - t);

            //位置を滑らかに更新
            transform.position = Vector3.Lerp(StartPos, EndPos, t);
            yield return null;
        }

        // 最終位置を微調整
        transform.position = new Vector3(EndPos.x, EndPos.y + OffsetY, EndPos.z);

        ForwardCoroutine = null;
    }

    /// <summary>
    /// 最初の地点と最後の地点を設定
    /// </summary>
    /// <param name="direction"></param>
    /// <param name="power"></param>
    public void SetMoveInfo(Transform opponent,Vector3 direction, float power)
    {
        moveStartPos = transform.position;

        //単位ベクトルをパワー分動かす
        moveEndPos = moveStartPos + direction.normalized * power;
    }
    public void ResetMoveInfo()//初期化処理
    {
        moveEndPos = Vector3.zero;
        moveStartPos = transform.position;
        currentMoveFrame = 0;
    }

    /// <summary>
    /// 技モーション中の移動方法イーズアウトを使って出を早くしている
    /// </summary>
    /// <param name="totalFrame"></param>
    /// <returns></returns>
    public Vector3 OnStraightMove(int totalFrame)
    {
        var t = (float)currentMoveFrame / totalFrame;

        if (t >= 1f)
            return moveEndPos;

        // イーズアウト
        t = 1f - Mathf.Pow(1f - t, 3);

        Vector3 pos = transform.position;

        // 画面端でなければ移動
        if (!IsGoingOutOfBounds())
        {
            Debug.Log("ugoku");
            pos = Vector3.Lerp(moveStartPos, moveEndPos, t);
        }

        // フレームは必ず進める
        currentMoveFrame++;

        return pos;
    }

    public void StopFowardCoroutine() { StopAllCoroutines(); }


    //投げの際に相手の位置を補正
    public void SnapOpponentInFront(Transform targetRoot)
    {
        if (targetRoot == null) return;
        Debug.Log("補正中");
        float posOffset = 0.8f;

        //相手の座標を自分の前まで持ってくる
        var pos = transform.position + transform.forward * posOffset;
        targetRoot.position = pos;


        // 自分の方を向かせる（Y軸回転だけ）
        Vector3 dir = (transform.position - targetRoot.position).normalized;
        dir.y = 0;

        if (dir != Vector3.zero)
            targetRoot.rotation = Quaternion.LookRotation(dir);
    }

}
