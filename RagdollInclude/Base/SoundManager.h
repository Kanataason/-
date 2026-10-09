#pragma once
#include <Audio.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ServiceLocator.h"

//効果音(SE)とBGM。DirectXTK Audioを使うので、読み込めるのは WAV だけ
class SoundManager
{
public:
    ~SoundManager() { Shutdown(); }

    void Initialize(ServiceLocator& locator);

    //終了のときの後片付け。先に音のエンジンを止めてから、音のデータを消す
    //何度呼んでも安全
    void Shutdown();

    //毎フレーム呼ぶ(呼ばないと、ヘッドホンの抜き差しなどに対応できない)
    void Update() { if (_engine) _engine->Update(); }

    //効果音を、名前のグループに、1つ足す。同じ名前で、何度か呼ぶと、グループの中の音が増える
    void LoadSe(const std::string& name, const std::wstring& path);
    void LoadBgm(const std::string& name, const std::wstring& path);

    //グループに、複数の音があれば、ランダムに1つ(直前と同じ音は、続けて選ばない)
    void PlaySe(const std::string& name, float volume = 1.0f);

    //その名前の効果音が、1つでも読み込めているか(キャラクター専用の音がないときに、標準の音へ戻すのに使う)
    bool HasSe(const std::string& name) const
    {
        const auto it = _sounds.find(name);
        return it != _sounds.end() && !it->second.clips.empty();
    }

    //すでに同じBGMが鳴っているときは何もしない
    void PlayBgm(const std::string& name, float volume = 0.4f, bool loop = true);
    void StopBgm();

private:
    void LoadFromJson(const std::string& jsonPath);

private:
    std::unique_ptr<DirectX::AudioEngine> _engine;

    //効果音のグループ(1つの名前に、1つ以上の音)
    struct SeGroup
    {
        std::vector<std::unique_ptr<DirectX::SoundEffect>> clips;
        int lastIndex = -1; //直前に鳴らした音の番号(続けて同じ音が鳴らないように)
    };

    //効果音と、BGMの元データ。インスタンスが生きている間は、元のSoundEffectも生きていなければならないので、持ち続ける
    std::unordered_map<std::string, SeGroup> _sounds;
    std::unordered_map<std::string, std::unique_ptr<DirectX::SoundEffect>> _bgmClips;

    std::unique_ptr<DirectX::SoundEffectInstance> _bgm; //いま鳴っているBGM
    std::string _currentBgmName;
};
