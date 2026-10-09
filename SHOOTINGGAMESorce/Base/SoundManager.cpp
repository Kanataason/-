#include "SoundManager.h"

void SoundManager::Initialize(const std::vector<SoundData>& list)
{
	_handles.fill(-1);//読み込んでいない音は-1にして、鳴らすときに無視する
	for (const auto& data : list)
	{
		int handle = LoadSoundMem(data.path.c_str(), data.bufferNum);
		if (handle == -1) continue;//素材が無くてもゲームは止めない
		ChangeVolumeSoundMem(data.volume, handle);
		_handles[static_cast<size_t>(data.id)] = handle;
	}
}
void SoundManager::PlaySe(SoundId id) const
{
	int handle = GetHandle(id);
	if (handle != -1) PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}
void SoundManager::PlayBgm(SoundId id)
{
	int handle = GetHandle(id);
	if (handle == _currentBgm) return;   //同じ曲なら最初から流し直さない

	StopBgm();
	_currentBgm = handle;
	if (_currentBgm != -1) PlaySoundMem(_currentBgm, DX_PLAYTYPE_LOOP);
}