using UnityEngine;
using UnityEngine.InputSystem;
using System.Collections.Generic;
using UnityEngine.EventSystems;
using UnityEngine.UI;
public class MainManager : MonoBehaviour
{

    //すべてのプレイヤーを保存している
    private List<UiInputManager> playerInputManagerList = new();
    private void OnDisable()
    {
        UnSubscribeEvents();
        UnSucscribeUiEvent();
    }
    private void UnSucscribeUiEvent()
    {
        //保存されているプレイヤーのイベント解除
        foreach (var input in playerInputManagerList)
        {
            input.OnCancelAction -= OnCancel;
            input.OnSubmitAction -= OnSubmit;
        }
        playerInputManagerList.Clear();
    }
    /// <summary>
    /// 各マネージャーのイベントを解除
    /// </summary>
    private void UnSubscribeEvents(UiselectedMenuChractor selectManager = null)
    {
        if (InputUserMakeManager.Instance != null)
        {
            InputUserMakeManager.Instance.OnCreateCharacter -= SubscribeUiEvents;
        }
        if (PlayerInitializeManager.Instance != null)
            PlayerInitializeManager.Instance.OnEnableUiPlayerInputAction -= SubscribeUiEvents;

        if (BattleStateManager.Instance != null)
            BattleStateManager.Instance.OnNextRoundAction -= UpdateFadeByUiState;

        if (selectManager == null) return;

        selectManager.OnCancelInputAction -= UnSubscribeEvents;
        Debug.Log("すべてのイベントの解除に成功");
    }

    //このスクリプトが生成された時に参照を渡される
    public void SubscribeEvents(UiselectedMenuChractor selectManager)//イベント登録
    {
        selectManager.OnInputAction += SubscribeUiEvents;
        selectManager.OnCancelInputAction += UnSubscribeEvents;
    }

    /// <summary>
    /// 各マネージャーのイベントを登録する
    /// </summary>
    private void SubscribeEvents()
    {
        if (InputUserMakeManager.Instance != null)
            InputUserMakeManager.Instance.OnCreateCharacter += SubscribeUiEvents;

        if (PlayerInitializeManager.Instance != null)
            PlayerInitializeManager.Instance.OnEnableUiPlayerInputAction += SubscribeUiEvents;

        if (BattleStateManager.Instance != null)
            BattleStateManager.Instance.OnNextRoundAction += UpdateFadeByUiState;
    }

    /// <summary>
    /// プレイヤーの入力イベントを登録
    /// </summary>
    /// <param name="input"></param>
    private void SubscribeUiEvents(PlayerInput input)
    {
        var uiManager = input.GetComponent<UiInputManager>();
        //一回解除してから設定
        uiManager.OnCancelAction -= OnCancel;
        uiManager.OnSubmitAction -= OnSubmit;

        uiManager.OnCancelAction += OnCancel;
        uiManager.OnSubmitAction += OnSubmit;

        playerInputManagerList.Add(uiManager);
    }




    private void Start()
    {

        SubscribeEvents();

        float waitLoadTime = 0.3f;
        //引数の数字だけ待つ
        Delay.WaitTime(this, waitLoadTime, () => { UpdateFadeByUiState(); });

    }

    /// <summary>
    /// 現在のUI状態を確認して必要なフェード演出を開始する
    /// </summary>
    private void UpdateFadeByUiState()
    {
        float durationTime = 0;
        float startAlpha = 1;
        float endAlpha = 0;
        //現在のUiステータスをみてフェードさせる
        switch (TatuGameManager.Instance.CurrentUiState)
        {

            case UiState.SelectCharactor:
                durationTime = 0.5f;
                EventManager.Instance.Startfade(durationTime,startAlpha,endAlpha);
                break;

            case UiState.Battle:
                durationTime = 1f;
                EventManager.Instance.Startfade(durationTime,startAlpha,endAlpha);
                break;
            default:break;
        }
    }
    private void OnSubmit()//プレイヤーが決定を押したら流れる
    {
        //バトル中だけ使用
        if (TatuGameManager.Instance.CurrentUiState == UiState.Pose||
           TatuGameManager.Instance.CurrentUiState == UiState.Battle)
        {
            var currentSelectButton = EventSystem.current.currentSelectedGameObject;
            //現在選んでいるボタンがあれば流す
            if(currentSelectButton != null)
            {
                var button = currentSelectButton.GetComponent<Button>();
                if (button == null) return;

                button.onClick.Invoke();//選んでいるボタンを実行
            }
        }
    }
    private void OnCancel()//プレイヤーがキャンセルを押したら流れる
    {
        Debug.Log("キャンセルされた");
        EventManager.Instance.EscCancel();
    }

}
