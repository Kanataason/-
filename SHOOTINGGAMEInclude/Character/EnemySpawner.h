#pragma once
#include <vector>
#include <DxLib.h>
#include "Datas.h"
#include "ServiceLocator.h"
#include "GameObjectfactory.h"
#include "GameWindow.h"
#include "BaseScene.h"

//読み込んだ出現データと、出現させた数をまとめたもの
struct SpawnEntry
{
	SpawnData data;
	int spawnedCount = 0;
	bool CanSpawn() const { return data.maxCount < 0 || spawnedCount < data.maxCount; }
};

//雑魚敵のランダム出現を担当する
//一定間隔ごとに、出現できるテンプレートから1つ選んでシーンに追加する
class EnemySpawner
{
public:
	void Initialize(const ServiceLocator& locator, std::vector<SpawnData> spawnData)
	{
		_factory = locator.factory;
		for (auto& data : spawnData)
			_entries.push_back({ std::move(data) });
		_screenWidth = static_cast<int>(locator.gameWindow->GetBounds().width);
		ResetTimer();
	}

	void Update(float deltaTime, BaseScene& scene)
	{
		if (!_isActive || _entries.empty()) return;

		_timer -= deltaTime;
		if (_timer > 0.0f) return;
		ResetTimer();

		//出現間隔がばらつかないように、上限に達したテンプレートを候補から外してから選ぶ
		std::vector<SpawnEntry*> candidates;
		candidates.reserve(_entries.size());
		for (auto& entry : _entries)
			if (entry.CanSpawn()) candidates.push_back(&entry);
		if (candidates.empty()) return;

		SpawnEntry& entry = *candidates[GetRand(static_cast<int>(candidates.size()) - 1)];
		++entry.spawnedCount;

		ObjectData objectData = entry.data.objectData;
		if (entry.data.isRandomX)
			objectData.transform.Position.x = static_cast<float>(40 + GetRand(_screenWidth - 80));

		scene.Spawn(_factory->CreateObject(objectData));
	}

	//ボス出現時などに出現を止める
	void SetActive(bool isActive) { _isActive = isActive; }

private:
	void ResetTimer()
	{
		_timer = _minInterval + GetRand(static_cast<int>((_maxInterval - _minInterval) * 100)) / 100.0f;
	}

	GameObjectFactory* _factory = nullptr;
	std::vector<SpawnEntry> _entries;
	float _timer = 0.0f;
	float _minInterval = 2.0f;
	float _maxInterval = 3.5f;
	int _screenWidth = 640;
	bool _isActive = true;
};