using System.Collections;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;
using UnityEngine.InputSystem.Users;

public class inputUserMakeManager : MonoBehaviour
{
    public static inputUserMakeManager Instance { get; private set; }
    private List<PlayerInput> list = new List<PlayerInput>();
    List<PlayerInput> inputs = new List<PlayerInput>();
    private bool Is_Ui = false;
    private void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
        }
        else
        {
            Destroy(gameObject);
        }
    }
    private void OnEnable()
    {
        InputSystem.onDeviceChange += OnDeviceChange;
    }
    private void OnDisable()
    {
        InputSystem.onDeviceChange -= OnDeviceChange;
    }
    [SerializeField] private GameObject G_Playerinput;
    [SerializeField] private GameObject G_Player1setinput;
    [SerializeField] private GameObject G_Player2setinput;

    [Header("SelecteCharactor")]
    [SerializeField] private PlayerInput Player1input;
    [SerializeField] private PlayerInput Player2input;

    [Header("Refarence")]
    [SerializeField] private UiPlayerSet uiPlayerSet;
    [SerializeField] private InputSystemUIInputModule uIInputModules;
    [SerializeField] private UiInputManager uiInputManager;
    [SerializeField] private MainManager mainManager;

    [Header("Parent")]
    [SerializeField] private Transform SetPlayerParent;
    [SerializeField] private Transform SetCharactorParent;



    private bool Is_TwoGamaPad = false;
    [SerializeField] private bool Is_GameMenu = false;
    void init()
    {
       SetCharactorParent = TatuGameManager.Instance.transform.Find("PlayerinputPokect");  
    }
    //private void OnValidate()
    //{
 
//}
     void Start()
    {

            StartCoroutine(InitFirst());

 
    }
    IEnumerator InitFirst()
    {
        yield return null; // 1フレーム待つ
        yield return new WaitForSeconds(0.1f); // デバイス認識を待つ
        init();
        InputUser.listenForUnpairedDeviceActivity = 0;
        if (Gamepad.all.Count > 1)
        {
            Is_TwoGamaPad = true;
        }
        else if (Gamepad.all.Count > 0)
        {
            Is_TwoGamaPad = false;
        }
        else
        {
            Debug.Log("playできません");
           yield return null;
        }
        if (Is_GameMenu)
        {
            UiSetUser();
            GetChildAndDestroy();
        }
        else
        {
            SetInputInfo();
        }
    }
    private void CheckDeviceController()
    {
        if (Gamepad.all.Count > 1)
        {
            Is_TwoGamaPad = true;
        }
        else if (Gamepad.all.Count > 0)
        {
            Is_TwoGamaPad = false;
        }
        else
        {
            Debug.Log("playできません");
            return;
        }
    }
    public void CheckUserCount() => Is_Ui = !Is_Ui;
    public void CreateUser()
    {
        Debug.Log("Seisei");
        if (Is_Ui)
        {
            if (Is_TwoGamaPad)
            {
                var player1 = PlayerInput.Instantiate(G_Playerinput, controlScheme: "GamePad", pairWithDevice: Gamepad.all[0]);
                list.Add(player1);
                player1.defaultControlScheme = "Gamepad";
                player1.SwitchCurrentControlScheme();
                player1.uiInputModule = uIInputModules;
                mainManager.Input = player1;
                mainManager.Inputs = player1.GetComponent<UiInputManager>();
            }
            else
            {
                var player1 = PlayerInput.Instantiate(G_Playerinput, controlScheme: "KeyBord", pairWithDevice: Keyboard.current);
                list.Add(player1);
                player1.defaultControlScheme = "KeyBord";
                player1.SwitchCurrentControlScheme();
                player1.uiInputModule = uIInputModules;
                mainManager.Input = player1;
                mainManager.Inputs = player1.GetComponent<UiInputManager>();
            }
        }
        else
        {
            if (Is_TwoGamaPad)
            {
                var player1 = PlayerInput.Instantiate(G_Player1setinput, controlScheme: "GamePad", pairWithDevice: Gamepad.all[0]);
                list.Add(player1);
                player1.defaultControlScheme = "Gamepad";
                player1.SwitchCurrentControlScheme();
                var player2 = PlayerInput.Instantiate(G_Player2setinput, controlScheme: "GamePad", pairWithDevice: Gamepad.all[1]);
                list.Add(player2);
                player2.defaultControlScheme = "Gamepad";
                player2.SwitchCurrentControlScheme();
              //  SecondMake(player1,player2);
            }
            else
            {
                var player1 = PlayerInput.Instantiate(G_Player1setinput, controlScheme: "KeyBord", pairWithDevice: Keyboard.current);
                list.Add(player1);
                player1.defaultControlScheme = "KeyBord";
                player1.SwitchCurrentControlScheme();
                var player2 = PlayerInput.Instantiate(G_Player2setinput, controlScheme: "GamePad", pairWithDevice: Gamepad.all[0]);
                list.Add(player2);
                player2.defaultControlScheme = "Gamepad";
                player2.SwitchCurrentControlScheme();
                //SecondMake(player1,player2);
            }
            SetParent("Move",list);
        }

    }
    private void SecondMake(PlayerInput player1,PlayerInput player2)
    {
        var second1 = PlayerInput.Instantiate(G_Playerinput);
        var second2 = PlayerInput.Instantiate(G_Playerinput);
        inputs.Add(second1);
        inputs.Add(second2);
        second1.actions = player1.actions;
        second1.defaultControlScheme = player1.defaultControlScheme;
        second1.defaultActionMap = player1.defaultActionMap;
        second1.notificationBehavior = player1.notificationBehavior;
        second1.enabled = false;
        second2.actions = player2.actions;
        second2.defaultControlScheme = player2.defaultControlScheme;
        second2.defaultActionMap = player2.defaultActionMap;
        second2.notificationBehavior = player2.notificationBehavior;
        second2.enabled = false;
        SetParent("SelectChara",inputs);
    }

    public void UiSetUser()//gamemenu時に呼ばれる
    {
        Is_Ui = true;
        BreakeUser();
        RemoveUser();
        CreateUser();
        if (Is_TwoGamaPad)
        {
            var user0 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Gamepad.all[0], user0);
            user0.AssociateActionsWithUser(list[0].actions);
        }
        else
        {
            var user0 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Keyboard.current, user0);
            user0.AssociateActionsWithUser(list[0].actions);
        }
    }
    public void SetPlayerUser()//playerset時に呼ばれる用
    {
        Is_Ui = false;
        BreakeUser();
        RemoveUser();
        CreateUser();
        StartCoroutine(User());
    }
    IEnumerator User()
    {
        yield return new WaitForSeconds(0.15f);
        if (InputUser.all.Count == 0)
        {
            SetPlayerUser();
            Debug.Log("ユーザーはまだ0人です");
            yield break;
        }
        else if (InputUser.all.Count > 2)
        {
            SetPlayerUser();
            Debug.Log("多いい");
            yield break;
        }
        else
        {
            Debug.Log($"ユーザー数: {InputUser.all.Count}");
        }
       // DebagUser.text = InputUser.all.Count.ToString();
        if (Is_TwoGamaPad)
        {
            Debug.Log("2");
            var user0 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Gamepad.all[1], user0);
            user0.AssociateActionsWithUser(list[0].actions);
            var user1 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Gamepad.all[0], user1);
            user1.AssociateActionsWithUser(list[1].actions);
        }
        else
        {
            Debug.Log("1");
            var user0 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Keyboard.current, user0);
            user0.AssociateActionsWithUser(list[0].actions);
            var user1 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Gamepad.all[0], user1);
            user1.AssociateActionsWithUser(list[1].actions);

        }
    }
    public void SetParent(string state,List<PlayerInput> listst)
    {
        int Count =0;
        foreach (var t in listst)
        {
            switch (state)
            {
                case "Move":
                    Count++;
                    var move = t.GetComponent<MoveUiPlayer>();
                    uiPlayerSet.SetList(move, Count);
                    Transform tra = t.transform;
                    tra.SetParent(SetPlayerParent);break;
                case "SelectChara":
                    Transform trans = t.transform;
                    trans.SetParent(SetCharactorParent);
                    break;
            }

        
        
        }
    }
    public void SetInputInfo()
    {
        int count = 0;
        var select = GameObject.Find("SelectMenu").GetComponent<UiselectedMenuChractor>();
        RemoveUser();
        foreach (Transform chlid in SetCharactorParent.transform)
        {
            count++;
            Debug.Log(chlid.name);
            var input = chlid.GetComponent<PlayerInput>();
            if (Is_TwoGamaPad)
            {

            }
            else
            {
                if (count == 1)
                {
                    input.enabled = true;
                    input.uiInputModule = uIInputModules;
                    select.playerInput1KeyBord = input;
                }
                else
                {
                    input.enabled = true;
                    input.uiInputModule = uIInputModules;
                    select.playerInputGamePad = input;
                }
            }
            #region
            //if(input.currentControlScheme == "GamePad")
            //{
            //    Debug.Log("1");
            //    if (Is_TwoGamaPad)
            //    {
            //        var user0 = InputUser.CreateUserWithoutPairedDevices();
            //        InputUser.PerformPairingWithDevice(Gamepad.all[1], user0);
            //        Player1input.defaultControlScheme = "Gamepad";
            //        Player1input.SwitchCurrentControlScheme();
            //    }
            //    else
            //    {
            //        var user0 = InputUser.CreateUserWithoutPairedDevices();
            //        InputUser.PerformPairingWithDevice(Gamepad.all[0], user0);
            //        Player1input.defaultControlScheme = "Gamepad";
            //        Player1input.SwitchCurrentControlScheme();
            //    }
            //}
            //else //if(input.currentControlScheme == "KeyBord")
            //{
            //    Debug.Log("Keybord");
            //    var user0 = InputUser.CreateUserWithoutPairedDevices();
            //    InputUser.PerformPairingWithDevice(Keyboard.current, user0);
            //    Player2input.defaultControlScheme = "KeyBord";
            //    Player2input.SwitchCurrentControlScheme();
            //}
            #endregion
        }
        // GetChildAndDestroy();
    }
    public void RemoveUser()
    {

        var allUsers = InputUser.all.ToList();
        foreach (var user in allUsers)
        {
            user.UnpairDevicesAndRemoveUser();
        }
        Debug.Log("After Remove: " + InputUser.all.Count);
    }
    public void BreakeUser()
    {
        foreach (var dev in list)
        {
            Destroy(dev.gameObject);
        }
        list.Clear();
    }
    public void BreakeSecondUser()
    {
        foreach(var li in inputs)
        {
            Destroy(li.gameObject);
        }
        inputs.Clear();
    }
    private void GetChildAndDestroy()
    {
        foreach(Transform chlid in SetCharactorParent)
        {
            Destroy(chlid.gameObject);
        }
    }
    void OnDeviceChange(InputDevice device, InputDeviceChange change)
    {
        if (change == InputDeviceChange.Added)
        {
            // コントローラーが追加されたとき
            if (device is Gamepad)
            {
                Debug.Log("コントローラー接続された: " + device.displayName);
                OnGamepadConnected(device as Gamepad);
            }
        }
        else if (change == InputDeviceChange.Removed)
        {
            if (device is Gamepad)
            {
                Debug.Log("コントローラー切断された: " + device.displayName);
                OnGamepadDisconnected(device as Gamepad);
            }
        }
    }
    void OnGamepadConnected(InputDevice input)
    {
        CheckDeviceController();
        UiSetUser();
        Debug.Log("関数が呼ばれました（接続）");
    }
    void OnGamepadDisconnected(InputDevice input)
    {
        CheckDeviceController();
        UiSetUser();
        Debug.Log("関数が呼ばれました（解除）");
    }
}

