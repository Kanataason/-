#include "GameAudio.h"

#include "Ragdoll.h"
#include "ScoreManager.h"
#include "RoundManager.h"
#include "SoundManager.h"
#include "CharacterRoster.h"

void GameAudio::Initialize(ServiceLocator& locator)
{
	_sound = locator.sound;
	_characters = locator.characters;

	if (_sound)
		_sound->PlayBgm("stage"); //同じ曲が鳴っていれば、続ける(「もういちど」で、頭に戻らない)
}

void GameAudio::Bind(Ragdoll* ragdoll, ScoreManager& score, RoundManager& round)
{
	//発射したあと、地面に着くたびに、ヒット音を鳴らす
	if (ragdoll)
		ragdoll->OnGroundHit.Subscribe([this]() { PlayCharacterSe("hit"); });

	score.OnScoreGained.Subscribe([this](ScoreGain gain)
		{
			PlayTargetHitSe();

			if (gain.isHead)
				PlayCharacterSe("head");
		});

	score.OnBonusGained.Subscribe([this](float /*points*/) { PlayCharacterSe("clear"); });

	round.OnShotFired.Subscribe([this]() { PlayCharacterSe("shot"); });

	//玉を撃ち終えて、結果が出るまでのドラムロール
	round.OnFinished.Subscribe([this]()
		{
			if (_sound)
				_sound->PlayBgm("drumRoll");
		});
}

void GameAudio::Update(float deltaTime)
{
	_currentHitSeTime -= deltaTime;
}

void GameAudio::PlayTargetHitSe()
{
	if (_currentHitSeTime > 0.0f)
		return;

	_currentHitSeTime = _hitSeCoolDown;
	PlayCharacterSe("hit");
}

void GameAudio::PlayCharacterSe(const std::string& eventName)
{
	if (!_sound)
		return;

	std::string soundName = _characters ? _characters->ResolveSoundName(eventName) : eventName;
	if (!_sound->HasSe(soundName))
		soundName = eventName;

	_sound->PlaySe(soundName);
}
