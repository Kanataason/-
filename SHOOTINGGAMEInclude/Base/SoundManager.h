#pragma once
#include <array>
#include <vector>
#include "Datas.h"

class SoundManager
{
public:
	void Initialize(const std::vector<SoundData>& list);
	void PlaySe(SoundId id) const;
	//今のBGMを止めてから、ループで鳴らす
	void PlayBgm(SoundId id);
	void StopBgm() 
	{
		if (_currentBgm != -1) StopSoundMem(_currentBgm);
		_currentBgm = -1;
	};
private:
	int GetHandle(SoundId id) const { return _handles[static_cast<size_t>(id)]; }

	std::array<int, static_cast<size_t>(SoundId::Count)> _handles;//Initializeで全部-1にする
	int _currentBgm = -1;
};