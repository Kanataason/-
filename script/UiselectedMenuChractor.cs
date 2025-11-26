using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.UI;
using System.Collections;
using Unity.VisualScripting;
using System;
//using UnityEngine.UIElements;
public class UiselectedMenuChractor : MonoBehaviour
{

    [System.Serializable]
    public class PlayerInfo
    {
        public int I_CurrentButtonNumber;
        public string PlayerName;
        public PlayerInput playerInput;
        public Transform modelSpawnPoint;
        public TextMeshProUGUI nameText;
        public TextMeshProUGUI explanationText;
        [HideInInspector] public GameObject currentModel;
        public bool Is_Selected => ChractorBoolManager.Instance.GetPlayerSelected(PlayerName);
    }
    [Header("参照")]
    public PlayerState player1State;

    [Header("キャラデータ")]
    public ChractorData[] chractordata;
    public UnityEngine.UI.Button[] ChractorButtons;
    [SerializeField] private TextMeshProUGUI SelectedCharaText;

    [Header("プレイヤー管理")]
    public PlayerInfo[] players; // プレイヤー1・2の情報を格納
    public PlayerInput playerInput1KeyBord;
    public PlayerInput playerInputGamePad;
    private int currentPlayerIndex = 0;
    private bool Is_Selecting = false;
    public bool Is_Fadeing = false;
    private bool Is_First = false;
    private GameObject currentSelectedUI;
    private Coroutine ShowCoroutine;
    private bool IsCover = false;
    [SerializeField] private Material P2Color;
    private void Awake()
    {
        if (PlayerDataManager.Instace.Player1.P1 == "1p")
        {
            players[0].playerInput = playerInputGamePad;
            players[1].playerInput = playerInput1KeyBord;
        }
        else if (PlayerDataManager.Instace.Player1.P1 == "2p")
        {
            players[0].playerInput = playerInput1KeyBord;
            players[1].playerInput = playerInputGamePad;
        }
    }
    void init()
    {
   
    }
    private void Start()
    {
        Invoke("init",0.1f);
        UpdateInputState();
    }

    private void LateUpdate()
    {
        if (Is_Fadeing) return;


        if (!IsMyTurn()) return;

        GameObject selected = EventSystem.current.currentSelectedGameObject;
        if (selected != null && selected != currentSelectedUI)
        {
            currentSelectedUI = selected;
            var buttonData = selected.GetComponent<SelectedChractorButtonData>();
            if (buttonData != null)
            {
                SelectCharacter(buttonData.ChractorDatas.ButtonNumber);
            }
        }
    }

    private bool IsMyTurn()
    {
        return currentPlayerIndex < players.Length && players[currentPlayerIndex].Is_Selected == false;
    }

    private void UpdateInputState()
    {
        SeeSelectChara(currentPlayerIndex);
        for (int i = 0; i < players.Length; i++)
        {
            if (i == currentPlayerIndex)
            {
                UiEventManager.Instance.UiModuleEnable(players[i].playerInput);
                UiEventManager.Instance.ResetEnableUiactionMap(players[i].playerInput);
            }
            else
            {
                 //UiEventManager.Instance.UiModuleDisable(players[i].playerInput);
                UiEventManager.Instance.ResetUiActionMap(players[i].playerInput);
            }
        }
        // UiEventManager.Instance.SwitchToPlayer(currentPlayerIndex);
        if (!Is_First)
        {
            UiEventManager.Instance.ResetFirstselectButton();
        }
        else
        {
            ChractorButtons[players[currentPlayerIndex].I_CurrentButtonNumber].Select();
        }
        // ★現在選択されているUIボタンに対応するキャラを表示
        GameObject selected = EventSystem.current.currentSelectedGameObject;
        if (selected != null)
        {
            currentSelectedUI = selected; // 状態更新
            var buttonData = selected.GetComponent<SelectedChractorButtonData>();
            if (buttonData != null)
            {
                SelectCharacter(buttonData.ChractorDatas.ButtonNumber);
            }
        }
    }
    private void SeeSelectChara(int CharaId)
    {
        var id = CharaId+1;
        SelectedCharaText.text = $"Selection Player{id}";
    }
    public void SelectCharacter(int index)
    {
        // 古いコルーチンが動いてたら止める
        if (ShowCoroutine != null)
        {
            StopCoroutine(ShowCoroutine);
            
            ShowCoroutine= null;
        }

        // 新しいコルーチン開始
        ShowCoroutine = StartCoroutine(SelectCharacterRoutine(index));

    }

    public void ConfirmSelection(ChractorData data)//ボタンを押したら呼ばれる
    {
       // players[currentPlayerIndex].I_CurrentButtonNumber = data.ButtonNumber;
        if (currentPlayerIndex >= 2||Is_Selecting) return;
        var player = players[currentPlayerIndex];
        var datas = PlayerDataManager.Instace != null
            ? PlayerDataManager.Instace.GetData(currentPlayerIndex)
            : null;

        if (datas != null && datas.SelectedChractor != null && data != null)
        {
            PlayerDataManager.Instace.IsCover = (data.Chractor == datas.SelectedChractor.Chractor);
        }
        else
        {

            PlayerDataManager.Instace.IsCover = false;
        }
        ChractorBoolManager.Instance.OnSelectedChecker(player.PlayerName,data);
        currentPlayerIndex++;
        if (currentPlayerIndex >= players.Length)
        {
            Debug.Log("全プレイヤー選択完了");
            UiEventManager.Instance.BattleStart();
            //TatuGameManager.Instance.SceneChenge("tatuyaScene");
        }
        else
        {
            Debug.Log($"次は {players[currentPlayerIndex].PlayerName} の番です");
            UpdateInputState();
            //  EventSystem.current.SetSelectedGameObject(null);
        }
    }

    public void CancelSelection()//Escを押したらなる
    {
        if (currentPlayerIndex <= 0)
        {
            TatuGameManager.Instance.SceneChenge("TatuyaGameMenu");
            PlayerDataManager.Instace.CanselPlayerInfo();
            PlayerDataManager.Instace.CancelPLayerPos();
        }
        else if (currentPlayerIndex == 1)
        {
            Is_First = true;
            var player = players[currentPlayerIndex];
            player.nameText.text = "";
            if (player.currentModel != null) Destroy(player.currentModel);
            currentPlayerIndex = 0;
            UpdateInputState();
        }
    }

    private IEnumerator SelectCharacterRoutine(int index)
    { 
        if (Is_First)
        {
            index = players[currentPlayerIndex].I_CurrentButtonNumber;
        }
        var playerss = players[currentPlayerIndex];
        if (playerss.currentModel != null&&!Is_First) Destroy(playerss.currentModel);
        Is_First = false;
        // 1秒待つ
        float F_time = 0f;
        Is_Selecting = true;
        while (F_time < 0.5f)
        {
            F_time += Time.deltaTime;
            yield return null;
        }
        Is_Selecting = false;

        // キャラ表示処理
        if (index < 0 || index >= chractordata.Length)
        {
            Debug.Log("不正なインデックス");
            yield break;
        }
        ChractorData data = null;

        if (chractordata != null && index >= 0 && index < chractordata.Length)
        {
            data = chractordata[index];
        }
        else
        {
            Debug.LogError($"chractordata[{index}] が無効です");
        }

        var datas = PlayerDataManager.Instace != null
            ? PlayerDataManager.Instace.GetData(currentPlayerIndex)
            : null;

        if (datas != null && datas.SelectedChractor != null && data != null)
        {
            IsCover = (data.Chractor == datas.SelectedChractor.Chractor);
        }
        else
        {
            IsCover = false;
        
        }
        var player = players[currentPlayerIndex];

        if (player.currentModel != null)
            Destroy(player.currentModel);

        player.nameText.text = data.S_Name;
        player.explanationText.text = data.S_explanation;
        if (data.Chractor != null)
        {
            var chara = Instantiate(data.Chractor, player.modelSpawnPoint.position, Quaternion.identity);
            if (IsCover)
            {
                Transform s = chara.transform.Find("body");
                if (s != null)
                {
                    var smr = s.GetComponent<SkinnedMeshRenderer>();
                    smr.material.color = Color.red;
                }
            }

            player.currentModel = chara;
        }
        Debug.Log($"キャラ {data.name} を {player.PlayerName} に表示しました");
    }
}
