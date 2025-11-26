using System.Collections;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.Users;

public class BattleManager : MonoBehaviour
{
    [Header("初期ポジション")]
    [SerializeField] private Transform Player1Pos;
    [SerializeField] private Transform Player2Pos;

    [Header("Refalence")]
    [SerializeField] private SwichAcitonMap SwichAcitonMap;
    private GameObject Player1;
    private GameObject Player2;

    private CharacterData characterplayer1;
    private CharacterData characterplayer2;

    private GameObject BackupPlayer1;
    private GameObject BackupPlayer2;

    private PlayerInput player1input;
    private PlayerInput player2input;

    private PlayerInputHandler playerInputHandler1;
    private PlayerInputHandler playerInputHandler2;

    [Header("inputAction")]
    [SerializeField] private InputActionAsset inputActions1;
    [SerializeField] private InputActionAsset inputActions2;






    private void Start()
    {

        //PlayerDataManagerからplayerが選んだキャラクターの情報を取ってそれを特定のpositionに設定
        Player1 = PlayerDataManager.Instace.Player1.SelectedChractor.Chractor;
        int Id1 = PlayerDataManager.Instace.Player1.PlayerId;
        Debug.Log(Id1);
        characterplayer1 = PlayerDataManager.Instace.Player1.SelectedChractor.SkilInfo;
        //Initialize(Player1);
        //PlayerInfo(Player1,Id1);    
        Player2 = PlayerDataManager.Instace.Player2.SelectedChractor.Chractor;
        int Id2 = PlayerDataManager.Instace.Player2.PlayerId;
        characterplayer2 = PlayerDataManager.Instace.Player2.SelectedChractor.SkilInfo;
        //Initialize(Player2);
        //PlayerInfo(Player2, Id2);
        BackupPlayer1 = Instantiate(Player1, Player1Pos.position, Quaternion.identity);
        BackupPlayer2 = Instantiate(Player2, Player2Pos.position, Quaternion.identity);
        Initialize(BackupPlayer1);
        PlayerInfo(BackupPlayer1, Id1);
        Initialize(BackupPlayer2);
        PlayerInfo(BackupPlayer2, Id2);
    }
    private void OnDisable()
    {
        // Player1.
    }
    void Initialize(GameObject player)
    {
        if (player.GetComponent<SimpleMovement>() == null)
            player.AddComponent<SimpleMovement>();

        if (player.GetComponent<PlayerInputHandler>() == null)
            player.AddComponent<PlayerInputHandler>();

        if (player.GetComponent<PlayerGuard>() == null)
            player.AddComponent<PlayerGuard>();

        if (player.GetComponent<PlayerAttack>() == null)
        {

            player.AddComponent<PlayerAttack>();
        }

        if (player.GetComponent<PlayerHealth>() == null)
            player.AddComponent<PlayerHealth>();

        if (player.GetComponent<PlayerOrientation>() == null)
            player.AddComponent<PlayerOrientation>();

        if (player.GetComponent<Rigidbody>() == null)
        {
            var rb = player.AddComponent<Rigidbody>();
            rb.isKinematic = true;
        }

        if (player.GetComponent<CapsuleCollider>() == null)
            player.AddComponent<CapsuleCollider>();
    }
    void InitializeSetEvent(PlayerInput playerInput, PlayerInputHandler handler)
    {
        playerInput.actions["Move"].performed += ctx => handler.Move(ctx);
        playerInput.actions["WeakAttack"].performed += ctx => handler.WeakAttack(ctx);
        playerInput.actions["MidlleAttack"].performed += ctx => handler.MidlleAttack(ctx);
        playerInput.actions["SpecialAttack"].performed += ctx => handler.SpecialAttack(ctx);
        playerInput.actions["StrongAttack"].performed += ctx => handler.StrongAttack(ctx);
        playerInput.actions["Jump"].performed += ctx => handler.Jump(ctx);
        // playerInput.actions["Jump"].performed += ctx => handler.

        playerInput.actions["Move"].canceled += ctx => handler.Move(ctx);
        playerInput.actions["WeakAttack"].canceled += ctx => handler.WeakAttack(ctx);
        playerInput.actions["MidlleAttack"].canceled += ctx => handler.MidlleAttack(ctx);
        playerInput.actions["SpecialAttack"].canceled += ctx => handler.SpecialAttack(ctx);
        playerInput.actions["StrongAttack"].canceled += ctx => handler.StrongAttack(ctx);
        playerInput.actions["Jump"].canceled += ctx => handler.Jump(ctx);
          //  playerInput.actions["Jump"].canceled += ctx => handler.OnCrouch(ctx.ReadValueAsButton());

        }

    void PlayerInfo(GameObject player,int Id)
    {
        var swich = player.GetComponent<SwichAcitonMap>();
        var input = player.GetComponent<PlayerInput>();
        input.enabled = true;
        var handler = player.GetComponent<PlayerInputHandler>();
        var guard = player.GetComponent<PlayerGuard>();
        if (Id == 1)
        {

            Debug.Log("fuck");
            player1input = input;
            playerInputHandler1 = handler;
            player1input.actions = inputActions1;
            player1input.defaultActionMap = "Player"; // InputActionAsset内の正しい名前に
            player1input.defaultControlScheme = "GamePad";
            player1input.SwitchCurrentControlScheme("GamePad");
            player1input.SwitchCurrentActionMap("Player");
            player1input.enabled = true;

            var user0 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Keyboard.current, user0);
            if (Mouse.current != null) InputUser.PerformPairingWithDevice(Mouse.current, user0); // マウスも必要なら
            user0.AssociateActionsWithUser(player1input.actions);
            player1input.ActivateInput();
            InitializeSetEvent(player1input, playerInputHandler1);
            guard.opponentTransform = BackupPlayer2.transform;
        }
        else if (Id == 2)
        {
            Debug.Log("fuck2");
            player2input = input;
            playerInputHandler2 = handler;
            player2input.actions = inputActions2;
            player2input.defaultActionMap = "Player";
            player2input.defaultControlScheme = "KeyBord";
            player2input.SwitchCurrentControlScheme("KeyBord");// InputActionAsset内の正しい名前に
            player2input.SwitchCurrentActionMap("Player"); // 明示的に切り替える
            player2input.enabled = true;
            var user1 = InputUser.CreateUserWithoutPairedDevices();
            InputUser.PerformPairingWithDevice(Gamepad.current, user1);
            user1.AssociateActionsWithUser(player2input.actions);
            player2input.ActivateInput();
            InitializeSetEvent(player2input, playerInputHandler2);
            guard.opponentTransform = BackupPlayer1.transform;
        }
     //  if (player1input != null&&player2input != null)
       // StartCoroutine( InitializePlayerSet(player1input,player2input));
        
        //swich.ChengeActionMap(input, Id);
    }
        IEnumerator InitializePlayerSet(PlayerInput player0,PlayerInput player1)
        {
        yield return new WaitForSeconds(0.5f);
            Debug.Log("aaa");
            // 安全確認
            if (Keyboard.current == null || Gamepad.current == null)
            {
                Debug.LogWarning("必要な入力デバイスが接続されていません。");
                yield return null ;
            }
            if (Gamepad.all.Count > 1)
            {
                //  Player0 → GamePad
                var user0 = InputUser.CreateUserWithoutPairedDevices();//何も接続していない空のplayerを作成
                InputUser.PerformPairingWithDevice(Gamepad.all[0], user0);//引数1を引数2にアタッチ
                if (Mouse.current != null) InputUser.PerformPairingWithDevice(Mouse.current, user0); // マウスも必要なら
                user0.AssociateActionsWithUser(player0.actions);//action設定
                
                player0.ActivateInput();//明示てきにonにする
                InitializeSetEvent(player0, playerInputHandler1);
                // Player1 → Gamepad（最初のもの）
                var user1 = InputUser.CreateUserWithoutPairedDevices();
                InputUser.PerformPairingWithDevice(Gamepad.all[1], user1);
                user1.AssociateActionsWithUser(player1.actions);
                player1.ActivateInput();
                InitializeSetEvent(player1, playerInputHandler2);

        }
            else
            {
                //  Player0 → キーボード（＋マウス）
                var user0 = InputUser.CreateUserWithoutPairedDevices();
                InputUser.PerformPairingWithDevice(Keyboard.current, user0);
                if (Mouse.current != null) InputUser.PerformPairingWithDevice(Mouse.current, user0); // マウスも必要なら
                user0.AssociateActionsWithUser(player0.actions);
                player0.ActivateInput();
                InitializeSetEvent(player0, playerInputHandler1);


                // Player1 → Gamepad（最初のもの） s
                var user1 = InputUser.CreateUserWithoutPairedDevices();
                InputUser.PerformPairingWithDevice(Gamepad.current, user1);
                user1.AssociateActionsWithUser(player1.actions);
                player1.ActivateInput();
                InitializeSetEvent(player1, playerInputHandler2);
            }


    }

}
