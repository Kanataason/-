#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <DirectXMath.h>

//画面の「どこに、何を置くか」の定義。描く仕組み(UiRenderer)とは分けてある。
//・座標はすべて 1280x720 の仮想解像度
//・表の実体は UiLayout.cpp にある(ここには宣言だけ)。画面を増やすときは、.cpp の表に足す

enum class TextType
{
    Start,
    Setting,
    Select,
    End,
    MouseControll,
    Score,
    ShotCount,
    StageSelectTitle,
    StageSelectHint,
    CharacterSelectTitle,
    CharacterSelectHint,
    None
};
enum class ButtonType
{
    Box,
    Setting,
    End,
    Panel,
    MouseControll,
    Score,
    None
};
enum class PanelType
{
    Setting,
    StageSelect,
    CharacterSelect,
    Score,
    ShotCount,
    Result,
    Pause,
    CannonGauge,
    None
};

//上から下への並び順。W/Sで選ぶ順番になる
struct MenuEntry
{
    TextType text;
    ButtonType button;
};

struct ButtonData
{
    //ボタンの大きさ
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float thickness = 2.0f;//枠の太さ

    DirectX::XMFLOAT4 frameColor = { 1, 1, 1, 1 }; //枠の色
};

struct TextData
{
    std::wstring text;

    float x = 0.0f;
    float y = 0.0f;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float scale = 1.0f;

    DirectX::XMFLOAT4 mainColor = { 1, 1, 1, 1 };
    DirectX::XMFLOAT4 outlineColor = { 0, 0, 0, 1 };

    float outLineStiffnes = 2.0f;
};

struct PanelData
{
    ButtonData frame; //パネルの位置・大きさ・枠の太さ・枠の色
    DirectX::XMFLOAT4 fillColor = { 0, 0, 0, 0.6f }; //塗りの色(アルファで透け具合)
    std::vector<MenuEntry> items; //ボタン+文字の組
};

namespace UiInfo
{
    //実体は UiLayout.cpp(ヘッダーに実体を書くと、includeした数だけ作られてしまうため)
    extern const std::unordered_map<TextType, TextData> textList;
    extern const std::unordered_map<ButtonType, ButtonData> buttonList;
    extern const std::unordered_map<PanelType, PanelData> panelList;

    //タイトルのメニュー。constexprなので、ヘッダーに置く(MenuCountを定数として使うため)
    inline constexpr std::array<MenuEntry, 3> MenuEntries =
    { {
        { TextType::Start,   ButtonType::Box},
        { TextType::Setting, ButtonType::Setting},
        { TextType::End,     ButtonType::End}
    } };

    inline constexpr int MenuCount = static_cast<int>(MenuEntries.size());
}
