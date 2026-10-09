#include "BaseGameObjectFactory.h"

#include <unordered_map>

#include "ObjectBuilders.h"

//役割(ObjectType)ごとのコンポーネント構築処理をまとめた関数テーブル
//新しい役割を追加する場合は、ObjectBuildersにBuild関数を作り、ここに登録するだけでよい
using ObjectBuilder = void(*)(GameObject&, const ObjectData&, const std::shared_ptr<ModelDatas>&);

static const std::unordered_map<ObjectType, ObjectBuilder> _objectBuilders =
{
    { ObjectType::Prop,        &ObjectBuilders::BuildProp },
    { ObjectType::Player,      &ObjectBuilders::BuildPlayer },
    { ObjectType::Camera,      &ObjectBuilders::BuildCamera },
    { ObjectType::Light,       &ObjectBuilders::BuildLight },
    { ObjectType::Cannon,      &ObjectBuilders::BuildCannon },
    { ObjectType::ScoreObject, &ObjectBuilders::BuildScoreObject }
};

//ObjectDataからGameObjectを1つ生成する
std::unique_ptr<GameObject> GameObjectFactory::CreateObject(const ObjectData& data)
{
    auto obj = std::make_unique<GameObject>();
    std::shared_ptr<ModelDatas> model;

    //モデルを必要とする属性の場合のみ読み込む
    if (data.attribute != Attribute::Other)
    {
        model = _modelProvider.Prepare(data);

        //モデルが無いまま組み立てると各Build関数でクラッシュするため、ここで中断
        if (model == nullptr)
        {
            OutputDebugStringA("CreateObject failed: model load error\n");
            return nullptr;
        }
    }

    //typeNameに対応する構築関数をテーブルから探して実行
    auto found = _objectBuilders.find(data.typeName);
    if (found != _objectBuilders.end())
    {
        found->second(*obj, data, model);
    }

    obj->GetTransform() = data.transform;
    return obj;
}
