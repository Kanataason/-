#pragma once
#define NOMINMAX

#include <string>
#include <vector>
#include "PhysicsBody.h"


class RagdollSystem
{
public:
    //ジョイントは子から親へ荷重を伝えないので、そのままだと胴体全体が倒れない
    //ルートから見たその位置のずれで、全体が倒れるトルクを与える
    void ApplyRootGravityTorque(std::vector<PhysicsBody>& physicsBodies, float subDeltaTime);


    //ragdollが倒れて静止したら、全ボーンをスリープさせて凍結する
    //ragdollが居ないときは、休止の状態をリセットする

    //凍結の条件は2つ(どちらかを満たせばよい):
    //・完全静止がkRestTime続いた
    //・ほぼ静止がkForceRestTime続いた(小さな揺れが残って完全静止にならない場合の保険)
    void UpdateRest(std::vector<PhysicsBody>& physicsBodies, float deltaTime);


    void ResetRest()
    {
        _settleTimer = 0.0f;
        _restTimer = 0.0f;
        _downLatched = false;
    }

    //休止(静止したら止める処理)を使うかを切り替える。
    //吊り下げているあいだ(体を持ち上げて、ぶらぶらさせているあいだ)は、止めないように false にする
    void SetRestEnabled(bool enabled) { _restEnabled = enabled; }

private:
    //全ボーンの動きの要約(休止判定の材料)
    struct MotionSummary
    {
        bool hasRagdoll = false;
        bool hasRoot = false; //ルート(腰)が見つかったか(rootY が有効か)
        bool upperBodyGrounded = false; //胸・頭が地面に触れている
        bool anyBoneGrounded = false; //足を含めどれかが地面に触れている
        bool allStill = true; //全ボーンが静止の基準を下回っている
        float maxLinearSpeed = 0.0f;
        float maxAngularSpeed = 0.0f;
        float rootY = 0.0f;
    };


    //ragdoll全体の動きの要約を測る
    MotionSummary MeasureMotion(std::vector<PhysicsBody>& physicsBodies, float deltaTime);

    //ragdollが倒れた状態かどうかを判定して返す
    //[倒れたとみなす条件]
    //・上半身(胸・頭)が地面に触れている
    //・ルート(腰)が、最初に地面に触れたときの高さより kRestRootDrop 以上下がった
    //(膝立ち・座りのように、足だけが地面に触れて上半身が触れない場合の対策
    //地面はY=0とは限らないので、絶対の高さではなく「下がった量」で見る)
    bool UpdateDownState(const MotionSummary& motion);

    //ほぼ静止しているragdollの速度・角速度を減衰させて、小さな揺れを消す
    //ほぼ静止が続くほど減衰を強くする
    void ApplySettleDamping(std::vector<PhysicsBody>& physicsBodies);

    //ragdollの全ボーンをスリープさせる
    //restHasLastを戻しておくのは、起こされた後に古い前回の位置と比べて,誤った速度を測らないため
    void FreezeAllBones(std::vector<PhysicsBody>& physicsBodies);

    //ragdollのボーンかどうか
    static bool IsRagdollBody(const PhysicsBody& body);

    //上半身のボーンかどうか
    //倒れたの判定には上半身だけを使う
    static bool IsUpperBodyBone(const std::string& boneName);

    //2つの回転の差から、角速度を求める
    static float ComputeAngularSpeed(const Quaternion& current, const Quaternion& previous, float deltaTime);

    //----休止判定の状態
    bool  _hasInitialRootY = false; //最初に地面に触れたときのルートの高さを記録済みか
    float _initialRootY = 0.0f; //その高さ
    bool  _downLatched = false; //「倒れた」を保持しているか
    float _settleTimer = 0.0f; //ほぼ静止している時間
    float _restTimer = 0.0f; //完全静止している時間
    bool  _restEnabled = true; //休止(静止したら止める)を使うか。吊り下げ中は false にする
};
