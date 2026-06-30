using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Users;
using UnityEngine.UI;


public class PlayerInitializeManager : MonoBehaviour
{
    public static PlayerInitializeManager Instance { get; private set; }
    /// <summary>
    /// 生成が終了したと知らせるだけのスクリプト
    /// </summary>
    [SerializeField] private Transform playerPos1;
    [SerializeField] private Transform playerPos2;

    [SerializeField] private Image PlayerImage1;
    [SerializeField] private Image PlayerImage2;

    //-------refarence--------//
    [SerializeField] private TestUi UIHpBarManager;
    [SerializeField] private PlayerDebagTextManager player1TextManager;
    [SerializeField] private PlayerDebagTextManager player2TextManager;

    PlayerInput Player1InputDevice;
    PlayerInput Player2InputDevice;

    private CameraFollowTwoTargets cameraFollowManager;

    private List<InputUser> _inputUsersList = new List<InputUser>();
    private List<InputDevice> _inputDevicesList = new List<InputDevice>();
    private List<PlayerInput> _playerInputList = new List<PlayerInput>();

    public event Action<PlayerInput> OnEnableUiPlayerInputAction;

    private void Awake()
    {
        InputUser.listenForUnpairedDeviceActivity = 0; // 自動ペアリング無効化

        if(Instance == null)
        {
            Instance = this;
        }
        else
        {
            Destroy(gameObject);
        }
    }

    //  private List<(PlayerInputHandler handler, PlayerInput input)> pendingHandlers = new();
    void Start()
    {
        UIAudioSound.Instance.RandomBGM();
        SubscribeEvents();

        TryGetComponents();

        Delay.WaitTime(this, 0.5f, PlayerControllerInitilize);
    }
    private void TryGetComponents()
    {
        cameraFollowManager = Camera.main.GetComponent<CameraFollowTwoTargets>();
    }

    private void OnDestroy()
    {
        ClearList();
        if (Player1InputDevice != null) Destroy(Player1InputDevice.gameObject);
        if (Player2InputDevice != null) Destroy(Player2InputDevice.gameObject);
    }
    private void OnDisable()
    {
        if (Player1InputDevice != null)
        {
            UnsubscribeEvents(Player1InputDevice);
        }
        if (Player2InputDevice != null)
        {
            UnsubscribeEvents(Player2InputDevice);
        }

    }
    private void SubscribeEvents()//イベント登録
    {
        BattleStateManager.Instance.OnUnPairUserAction +=ClearPlayerInputList;
        BattleStateManager.Instance.OnTimeOutGameAction += ClearPlayerInputList;
    }
    private void UnsubscribeEvents(PlayerInput targetinput)//イベント解除
    {
        var handler = targetinput.GetComponent<PlayerInputHandler>();
        var UiInput = targetinput.GetComponent<UiInputManager>();
        var controller = targetinput.GetComponent<PlayerController>();

        if(controller != null)
        controller.OnRemovePos -= RemovePlayerPos;

        if (BattleStateManager.Instance != null)
        {
            BattleStateManager.Instance.OnUnPairUserAction -= ClearPlayerInputList;
            BattleStateManager.Instance.OnTimeOutGameAction -= ClearPlayerInputList;
        }

        if (handler == null) return;

        UnsubscribeAttackEvent(targetinput, handler,UiInput);

        UnsubscribeUiEvent(targetinput, UiInput);
    }
    private void UnsubscribeAttackEvent(PlayerInput input,PlayerInputHandler handler,UiInputManager uiInput)
    {
        input.actions["Move"].performed -= handler.OnMove;
        input.actions["Move"].canceled -= handler.OnMove;
        input.actions["Jump"].canceled -= handler.OnJump;
        input.actions["Jump"].performed -= handler.OnJump;
        input.actions["WeakAttack"].performed -= handler.OnWeakAttack;
        input.actions["WeakAttack"].canceled -= handler.OnWeakAttack;
        input.actions["MidlleAttack"].canceled -= handler.OnMidlleAttack;
        input.actions["StrongAttack"].canceled -= handler.OnStrongAttack;
        input.actions["SpecialAttack"].canceled -= handler.OnSpecialAttack;
        input.actions["MidlleAttack"].performed -= handler.OnMidlleAttack;
        input.actions["StrongAttack"].performed -= handler.OnStrongAttack;
        input.actions["SpecialAttack"].performed -= handler.OnSpecialAttack;
        input.actions["Grap"].performed -= handler.OnThrow;
        input.actions["Grap"].canceled -= handler.OnThrow;
        input.actions["TiltAttack"].performed -= handler.OnTiltAttack;
        input.actions["TiltAttack"].canceled -= handler.OnTiltAttack;
        input.actions["SmashAttack"].performed -= handler.OnSmashAttack;
        input.actions["SmashAttack"].canceled -= handler.OnSmashAttack;
        input.actions["Crouch"].performed -= handler.OnCrouch;
        input.actions["Crouch"].canceled -= handler.OnCrouch;
    }
    private void SubscribeAttackEvent(PlayerInputHandler handler, UiInputManager uiManager, PlayerInput input)//入力を受け取る関数と紐づけ
    {

        input.actions["Move"].performed += handler.OnMove;
        input.actions["WeakAttack"].performed += handler.OnWeakAttack;
        input.actions["MidlleAttack"].performed += handler.OnMidlleAttack;
        input.actions["SpecialAttack"].performed += handler.OnSpecialAttack;
        input.actions["StrongAttack"].performed += handler.OnStrongAttack;
        input.actions["Jump"].performed += handler.OnJump;
        input.actions["Grap"].performed += handler.OnThrow;
        input.actions["TiltAttack"].performed += handler.OnTiltAttack;
        input.actions["SmashAttack"].performed += handler.OnSmashAttack;
        input.actions["Crouch"].performed += handler.OnCrouch;

        input.actions["Move"].canceled += handler.OnMove;
        input.actions["WeakAttack"].canceled += handler.OnWeakAttack;
        input.actions["MidlleAttack"].canceled += handler.OnMidlleAttack;
        input.actions["SpecialAttack"].canceled += handler.OnSpecialAttack;
        input.actions["StrongAttack"].canceled += handler.OnStrongAttack;
        input.actions["Jump"].canceled += handler.OnJump;
        input.actions["Grap"].canceled += handler.OnThrow;
        input.actions["TiltAttack"].canceled += handler.OnTiltAttack;
        input.actions["SmashAttack"].canceled += handler.OnSmashAttack;
        input.actions["Crouch"].canceled += handler.OnCrouch;


        var uiMap = input.actions.FindActionMap("Ui", true);
        uiMap.Enable();

        uiMap["Cancel"].canceled += uiManager.OnCancel;
        uiMap["Navigate"].canceled += uiManager.OnNavigate;
        uiMap["Submit"].canceled += uiManager.OnSubmit;

        uiMap["Cancel"].performed += uiManager.OnCancel;
        uiMap["Navigate"].performed += uiManager.OnNavigate;
        uiMap["Submit"].performed += uiManager.OnSubmit;

    }
    private void UnsubscribeUiEvent(PlayerInput input, UiInputManager handler)
    {
        var uiMap = input.actions.FindActionMap("Ui", true);
        uiMap.Enable();
        uiMap["Cancel"].canceled -= handler.OnCancel;
        uiMap["Navigate"].canceled -= handler.OnNavigate;
        uiMap["Submit"].canceled -= handler.OnSubmit;
        uiMap["Cancel"].performed -= handler.OnCancel;
        uiMap["Navigate"].performed -= handler.OnNavigate;
        uiMap["Submit"].performed -= handler.OnSubmit;
        var playerMap = input.actions.FindActionMap("Player", true);
        uiMap.Enable();
    }
    //プレイヤーを生成
    private void PlayerControllerInitilize()
    {
        PlayerDataManager playerDataManager = PlayerDataManager.Instance;
        RemoveUser();
        var p1Data = playerDataManager.Player1Data;
        var p2Data = playerDataManager.Player2Data;

        Player1InputDevice = CreateDevice(p1Data.CurrentControlScheme, p1Data.CurrentDevice,p1Data.CurrentCharacter);
        Player2InputDevice = CreateDevice(p2Data.CurrentControlScheme, p2Data.CurrentDevice,p2Data.CurrentCharacter);

        Player2InputDevice.transform.rotation = Quaternion.Euler(0, -90, 0);
        Player2InputDevice.transform.position = playerPos2.position;
        Player1InputDevice.transform.position = playerPos1.position;
    }
    //セレクト画面で選択した情報でキャラクターを生成する
    private PlayerInput CreateDevice(string controlSchemeGamepad,InputDevice device,GameObject character)
    {
        PlayerInput input = PlayerInput.Instantiate(character);

        SetPlayerInput(input, controlSchemeGamepad, device);
        return input;

    }
    //入力デバイスをキャラクターと紐づけ
    private void SetPlayerInput(PlayerInput input,string scheme,InputDevice device)
    {
        _playerInputList.Add(input);
        OnEnableUiPlayerInputAction?.Invoke(input);

        if (string.IsNullOrEmpty(scheme) || device == null) return;

        input.enabled = true;
        Debug.Log("入力コントローラー登録");

        input.SwitchCurrentControlScheme(scheme, device);
        input.DeactivateInput();
    }


    private void PlayerInfo(PlayerInput playerInput, int id, Transform transfom)
    {
        //コンポーネント構造体を取得
        var com = GetComponents(playerInput);

        SetupCommon(playerInput, com, transfom);

        //player1,2の情報をセット
        if (id == 0)
            SetupPlayer1(playerInput, com);
        else
            SetupPlayer2(playerInput, com);
    }

    //構造体にplayer情報をセット
    private PlayerComponents GetComponents(PlayerInput input)
    {
        return new PlayerComponents
        {
            processController = input.GetComponent<PlayerProcessController>(),
            uiManager = input.GetComponent<UiInputManager>(),
            handler = input.GetComponent<PlayerInputHandler>(),
            state = input.GetComponent<PlayerState>(),
            move = input.GetComponent<SimpleMovement>(),
            collider = input.GetComponent<TatuCollderManager>(),
        };
    }
    private void SetupCommon(PlayerInput input, PlayerComponents component, Transform spawnPos)
    {

        input.transform.position = spawnPos.position;
        input.defaultActionMap = "Player";
        //デバイスが登録されていたら入力On
        if (input.devices.Count > 0 && !string.IsNullOrEmpty(input.currentControlScheme))
        {
            input.SwitchCurrentActionMap("Player");
            input.ActivateInput();
            input.actions.Enable();
        }

        component.processController.SetUpAnimation();
    }
    private void SetupPlayer1(PlayerInput input, PlayerComponents component)
    {
        //debag用のスクリプトに移る
        component.processController.SetDebagManager(player1TextManager);

        int playerId = 1;

        component.processController.SetUpPlayer("PlayerA", playerId);

        UIHpBarManager.InitializePlayerUI(component.state);

        //ここは外部で誰が敵かをセットしている
        component.collider.SetOpponentAndSubscribeEvent(Player2InputDevice.GetComponent<TatuCollderManager>());
        component.move.SetEnemy(Player2InputDevice.transform);

        cameraFollowManager.SetTarget(input.transform, playerId);
        PlayerImage2.sprite = PlayerDataManager.Instance.Player1Data.CostomData.Sprite;

        //一回解除してから登録する
        UnsubscribeAttackEvent(input, component.handler,component.uiManager);
        SubscribeAttackEvent(component.handler,component.uiManager, input);
      
    }
    private void SetupPlayer2(PlayerInput input, PlayerComponents component)
    {
        int playerId = 2;
        component.processController.SetDebagManager(player2TextManager);

        component.processController.SetUpPlayer("PlayerB", playerId);

        UIHpBarManager.InitializePlayerUI(component.state);

        //ここは外部で誰が敵かをセットしている
        component.collider.SetOpponentAndSubscribeEvent(Player1InputDevice.GetComponent<TatuCollderManager>());
        component.move.SetEnemy(Player1InputDevice.transform);

        cameraFollowManager.SetTarget(input.transform, playerId);
        PlayerImage1.sprite = PlayerDataManager.Instance.Player2Data.CostomData.Sprite;

        //重複を防ぐため一回解除してから登録する
        UnsubscribeAttackEvent(input, component.handler, component.uiManager);
        SubscribeAttackEvent(component.handler,component.uiManager, input);
    
    }
 
    public void StartSet()
    {
        if (Player1InputDevice == null || Player2InputDevice == null)
        {
            Debug.LogError("プレイヤーが初期化されていません！Initilize()の完了前にStartSetが呼ばれています。");
            return; // Null参照を防ぐ
        }

        //生成したキャラクターに情報をセット
        PlayerInfo(Player1InputDevice, PlayerDataManager.Instance.Player1Data.PlayerId, playerPos1);
        PlayerInfo(Player2InputDevice, PlayerDataManager.Instance.Player2Data.PlayerId, playerPos2);

    }
    public void UserRemove()
    {
        for (int i = 0; i < _inputUsersList.Count; i++)
        {
            var user = _inputUsersList[i];
            if (user.valid)
            {
                user.UnpairDevicesAndRemoveUser();
            }
        }
        _inputDevicesList.Clear();
        _inputUsersList.Clear();
    }

    //プレイヤーを取得
    public List<PlayerInput> GetPlayersList() { return _playerInputList; }
    public void ClearList()
    {
        _playerInputList.Clear();
    }
    private void ClearPlayerInputList()
    {
        foreach(var player in _playerInputList)
        {
            if (player.enabled)
                player.DeactivateInput();
        }
    }
    public void EnablePlayerInputList()
    {
        foreach (var player in _playerInputList)
        {
            if (player.enabled)
                player.ActivateInput();
        }
    }
    private void RemoveUser()//入力デバイスの解除
    {
        var alldevice = InputUser.all.ToList();
        foreach (var device in alldevice)
        {
            if (device.valid)
            {
                Debug.Log("解除");
                device.UnpairDevicesAndRemoveUser();
            }
        }
    }
    public void RemovePlayerPos()//バグで落ちたときに移動用
    {
        Vector3 pos1 = playerPos1.position;
        Vector3 pos2 = playerPos2.position;
        Player1InputDevice.transform.position = pos1;
        Player2InputDevice.transform.position = pos2;
    }
    //コンポーネントの構造体
    class PlayerComponents
    {
        public PlayerProcessController processController;
        public UiInputManager uiManager;
        public PlayerInputHandler handler;
        public PlayerState state;
        public SimpleMovement move;
        public TatuCollderManager collider;
    }
}