#pragma once
#include <iostream>
#include "VectorUtility.h"
#include "Ragdoll.h"

struct PhysicsBody
{
    //所有者はGameObject(RigidBody/Colliderのunique_ptr)。ここでは借りるだけ
    RigidBody* rigidBody = nullptr;
    Collider* collider = nullptr;

    RagdollBone ragdollBone;


    //前回の移動量とかを保存する
    Vector3 restLastPos{};
    Quaternion restLastRot{};

    bool restHasLast = false;
};