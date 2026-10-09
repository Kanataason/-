#include "SoundManager.h"

#include <fstream>
#include <exception>
#include <random>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <ThirdParty/nlohmann/json.hpp>

#include "StringUtility.h"

using json = nlohmann::json;
using StringUtility::ToWide;

void SoundManager::Initialize(ServiceLocator& /*locator*/)
{
	DirectX::AUDIO_ENGINE_FLAGS flags = DirectX::AudioEngine_Default;
#ifdef _DEBUG
	flags |= DirectX::AudioEngine_Debug; //デバッグ時に、詳しいエラーが出る
#endif

	//音の出力装置が使えない環境でも、ゲームは動くように、失敗したら無音にする
	try
	{
		_engine = std::make_unique<DirectX::AudioEngine>(flags);
	}
	catch (const std::exception&)
	{
		OutputDebugStringA("SoundManager: AudioEngine init failed (no sound)\n");
		_engine.reset();
		return;
	}

	LoadFromJson("Asset/Data/Sound.json");
}

void SoundManager::Shutdown()
{
	if (_engine)
		_engine->Suspend(); //音のスレッドを止める。これ以降、鳴っている音の途中経過を、スレッドが触ることはない

	//SoundEffectを消す前に、そこから作ったインスタンスを消す。エンジンは、いちばん最後
	if (_bgm)
		_bgm->Stop(true);
	_bgm.reset();
	_bgmClips.clear();
	_sounds.clear();
	_engine.reset();
}

void SoundManager::LoadFromJson(const std::string& jsonPath)
{
	std::ifstream file(jsonPath);
	if (!file.is_open())
	{
		OutputDebugStringA("SoundManager: Sound.json not found\n");
		return;
	}

	//書き方を間違えていても、例外を投げずに空で返す
	const json root = json::parse(file, nullptr, false);
	if (root.is_discarded() || !root.is_object())
	{
		OutputDebugStringA("SoundManager: Sound.json is invalid\n");
		return;
	}

	//"path"(1つ)も、"paths"(複数のグループ)も、書ける。同じ名前の行を、何行か書いても、同じグループに足される
	for (const char* group : { "SE", "BGM" })
	{
		if (!root.contains(group) || !root[group].is_array())
			continue;

		for (const auto& entry : root[group])
		{
			if (!entry.is_object())
				continue;

			const std::string name = entry.value("name", "");
			if (name.empty())
				continue;

			//このエントリーの、ファイルのパスを、すべて集める
			std::vector<std::string> paths;
			if (entry.contains("path") && entry["path"].is_string())
				paths.push_back(entry["path"].get<std::string>());
			if (entry.contains("paths") && entry["paths"].is_array())
			{
				for (const auto& item : entry["paths"])
				{
					if (item.is_string())
						paths.push_back(item.get<std::string>());
				}
			}

			for (const std::string& path : paths)
			{
				const std::wstring widePath = ToWide(path);
				if (widePath.empty())
					continue;

				if (std::string(group) == "SE")
					LoadSe(name, widePath);
				else
					LoadBgm(name, widePath);
			}
		}
	}
}

void SoundManager::LoadSe(const std::string& name, const std::wstring& path)
{
	if (!_engine)
		return;

	//ファイルがない・WAV以外のときは、例外が投げられる。ゲームは止めずに、その音だけ、使えないことにする
	try
	{
		//同じ名前のグループに、音を足す(読み込めたものだけが、グループに入る)
		_sounds[name].clips.push_back(std::make_unique<DirectX::SoundEffect>(_engine.get(), path.c_str()));
	}
	catch (const std::exception&)
	{
		OutputDebugStringW((L"SoundManager: SE load failed: " + path + L"\n").c_str());
	}
}

void SoundManager::LoadBgm(const std::string& name, const std::wstring& path)
{
	if (!_engine)
		return;

	try
	{
		_bgmClips[name] = std::make_unique<DirectX::SoundEffect>(_engine.get(), path.c_str());
	}
	catch (const std::exception&)
	{
		OutputDebugStringW((L"SoundManager: BGM load failed: " + path + L"\n").c_str());
	}
}

void SoundManager::PlaySe(const std::string& name, float volume)
{
	auto it = _sounds.find(name);
	if (it == _sounds.end() || it->second.clips.empty())
		return;

	SeGroup& group = it->second;
	const int count = static_cast<int>(group.clips.size());

	//2つ以上あるときは、ランダムに選ぶ。直前と同じ音は避ける(同じ音が、続くと、単調に聞こえるため)
	int index = 0;
	if (count > 1)
	{
		static std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<int> pick(0, count - 2); //直前の音を除いた、count-1個の中から選ぶ
		index = pick(rng);
		if (group.lastIndex >= 0 && index >= group.lastIndex)
			++index;
	}
	group.lastIndex = index;

	group.clips[static_cast<size_t>(index)]->Play(volume, 0.0f, 0.0f);
}

void SoundManager::PlayBgm(const std::string& name, float volume, bool loop)
{
	auto it = _bgmClips.find(name);
	if (it == _bgmClips.end())
		return;

	//同じ曲が、すでに鳴っていれば、そのまま続ける
	if (_bgm && _currentBgmName == name && _bgm->GetState() == DirectX::PLAYING)
		return;

	if (_bgm)
		_bgm->Stop();

	//元のSoundEffect(it->second)は、_bgmClipsが持ち続けるので、インスタンスは、安全に使える
	_bgm = it->second->CreateInstance();
	_bgm->SetVolume(volume);
	_bgm->Play(loop);
	_currentBgmName = name;
}

void SoundManager::StopBgm()
{
	if (_bgm)
		_bgm->Stop();
	_currentBgmName.clear();
}
