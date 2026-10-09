#include "UiLayout.h"

namespace UiInfo
{
    const std::unordered_map<TextType, TextData> textList =
    {
        {TextType::Start,{.text = L"ステージセレクト",.centerX = 640.0f,.centerY = 430.0f,.scale = 0.36f
        ,.mainColor = { 1, 1, 1, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::Setting,{.text = L"キャラクターセレクト",.centerX = 640.0f,.centerY = 510.0f,.scale = 0.29f
        ,.mainColor = { 1, 1, 1, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::End,{.text = L"エンド",.centerX = 640.0f,.centerY = 590.0f,.scale = 0.7f
        ,.mainColor = { 1, 1, 1, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::MouseControll,{.text = L"マウスのかんど",.centerX = 350.0f,.centerY = 200.0f,.scale = 0.7f
        ,.mainColor = { 1, 1, 1, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::Score,{.text = L"スコア",.centerX = 90.0f,.centerY = 44.0f,.scale = 0.3f
        ,.mainColor = { 0.7f, 0.8f, 1.0f, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::ShotCount,{.text = L"のこり",.centerX = 640.0f,.centerY = 44.0f,.scale = 0.3f
        ,.mainColor = { 0.7f, 0.8f, 1.0f, 1 },.outlineColor = { 0, 0, 0, 1 }}},

        //ステージ選択(ボタンの中心に、文字を置く)
        {TextType::StageSelectTitle,{.text = L"ステージをえらぶ",.centerX = 640.0f,.centerY = 185.0f,.scale = 0.65f
        ,.mainColor = { 1, 1, 1, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::StageSelectHint,{.text = L"W/S: えらぶ    スペース: けってい    Enter: もどる",.centerX = 640.0f,.centerY = 520.0f,.scale = 0.28f
        ,.mainColor = { 0.7f, 0.8f, 1.0f, 1 },.outlineColor = { 0, 0, 0, 1 }}},

        //キャラクター選択(名前の行は、キャラクターの数だけ、Title.cppが並べる)
        {TextType::CharacterSelectTitle,{.text = L"キャラクターをえらぶ",.centerX = 640.0f,.centerY = 185.0f,.scale = 0.55f
        ,.mainColor = { 1, 1, 1, 1 },.outlineColor = { 0, 0, 0, 1 }}},
        {TextType::CharacterSelectHint,{.text = L"W/S: えらぶ    スペース: けってい",.centerX = 640.0f,.centerY = 535.0f,.scale = 0.28f
        ,.mainColor = { 0.7f, 0.8f, 1.0f, 1 },.outlineColor = { 0, 0, 0, 1 }}}
    };

    const std::unordered_map<ButtonType, ButtonData> buttonList =
    {
        {ButtonType::Box,{.left = 540.0f,.top = 400.0f,.width = 200.0f,.height = 60.0f}},
        {ButtonType::Setting,{.left = 540.0f,.top = 480.0f,.width = 200.0f,.height = 60.0f}},
        {ButtonType::End,{.left = 540.0f,.top = 560.0f,.width = 200.0f,.height = 60.0f}},
        {ButtonType::Panel,{.left = 350.0f,.top = 100.0f,.width = 500.0f,.height = 500.0f}},
        {ButtonType::MouseControll,{.left = 350.0f,.top = 100.0f,.width = 200.0f,.height = 200.0f}},
    };

    const std::unordered_map<PanelType, PanelData> panelList =
    {
        { PanelType::Setting, {
            .frame = {.left = 350.0f, .top = 100.0f, .width = 500.0f, .height = 500.0f,
                       .thickness = 2.0f, .frameColor = { 1, 1, 1, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.6f },
            .items = {{TextType::MouseControll, ButtonType::MouseControll},
            } }
        },
        //ステージ選択(画面の中央)。選ぶ行は、ステージの一覧(Stages.json)から作るので、itemsは空
        { PanelType::StageSelect, {
            .frame = {.left = 340.0f, .top = 130.0f, .width = 600.0f, .height = 440.0f,
                       .thickness = 2.0f, .frameColor = { 1, 1, 1, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.65f },
            .items = {} }
        },
        //キャラクター選択(画面の中央)。選ぶ行は、キャラクターの一覧から作るので、itemsは空
        { PanelType::CharacterSelect, {
            .frame = {.left = 340.0f, .top = 130.0f, .width = 600.0f, .height = 440.0f,
                       .thickness = 2.0f, .frameColor = { 1, 1, 1, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.65f },
            .items = {} }
        },
        {PanelType::Score, {
            .frame = {.left = 20.0f, .top = 20.0f, .width = 320.0f, .height = 120.0f,
                       .thickness = 2.0f, .frameColor = { 1, 1, 1, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.6f },
            .items = {{TextType::Score, ButtonType::None}
            }}
        },
        //画面の上の中央(1280の中央 640 を、パネルの中心にする)
        {PanelType::ShotCount, {
            .frame = {.left = 520.0f, .top = 20.0f, .width = 240.0f, .height = 120.0f,
                       .thickness = 2.0f, .frameColor = { 1, 1, 1, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.6f },
            .items = {{TextType::ShotCount, ButtonType::None}
            }}
        },
        {PanelType::CannonGauge, {
            .frame = {.left = 80.0f, .top = 200.0f, .width = 30.0f, .height = 200.0f,
                       .thickness = 2.0f, .frameColor = { 0, 1, 0, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.6f },
            .items = {{TextType::None, ButtonType::None}
            }}
        },
         {PanelType::Result, {
            //画面の中央(1280x720の中心 640,360 が、パネルの中心)
            .frame = {.left = 340.0f, .top = 80.0f, .width = 600.0f, .height = 560.0f,
                       .thickness = 3.0f, .frameColor = { 1.0f, 0.85f, 0.3f, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.6f },
            .items = {{TextType::None, ButtonType::None}
            }}
        },
        {PanelType::Pause, {
            //画面の中央(パネルの中心が 640,360)
            .frame = {.left = 440.0f, .top = 170.0f, .width = 400.0f, .height = 380.0f,
                       .thickness = 3.0f, .frameColor = { 1.0f, 1.0f, 1.0f, 1 } },
            .fillColor = { 0.0f, 0.0f, 0.0f, 0.6f },
            .items = {{TextType::None, ButtonType::None}
            }}
        }
    };
}
