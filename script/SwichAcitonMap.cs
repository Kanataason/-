using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;

public class SwichAcitonMap : MonoBehaviour
{
    [SerializeField] private PlayerInput Main;
    [SerializeField] private InputSystemUIInputModule input1;

    [SerializeField] private EventSystem Player1;
    [SerializeField] private EventSystem Player2;

    [Header("バトル用")]
    [SerializeField] private InputActionAsset Charactor1;
    [SerializeField] private InputActionAsset Charactor2;
    private void Start()
    {
        if (input1 != null)
        {
       //     Main.uiInputModule = input1;
        }
    }
    public void SwichActionUiMap(PlayerInput playerinput)
    {
        playerinput.SwitchCurrentActionMap("Ui");//UIのActionMapに切り替え
        Debug.Log("Ui変更");
    }

    public void SwichActionPlayerMap(PlayerInput playerinput)
    {
        playerinput.SwitchCurrentActionMap("Player");//playerのActionMapに切り替え
        Debug.Log("Player変更");
    }
    public void DisableActionPlayerMap(PlayerInput playerinput)
    {
        if (playerinput == null || playerinput.gameObject == null) return;
        playerinput.DeactivateInput();//入力を完全に無効
        Debug.Log("無効"); Debug.Log(playerinput.name);
    }
    public void ActiveActionPlayerMap(PlayerInput playerinput)
    {
        playerinput.ActivateInput();//入力可能
        Debug.Log("有効");
        Debug.Log(playerinput.name);
    }
    public void PlayerInputUiModuleDisable(PlayerInput playerinput)
    {
        if (playerinput == null) return;
        playerinput.uiInputModule.enabled = false;
    }
    public void PlayerInputUiModuleEnable(PlayerInput playerinput)
    {
        EventSystem.current.GetComponent<InputSystemUIInputModule>().actionsAsset = playerinput.actions;
         playerinput.uiInputModule.enabled = true;
    }
    public void EnableActionMap(string Map,InputActionAsset inputActions)//ActionMapを有効
    {
        var map = inputActions.FindActionMap(Map);
        if (map != null)
        {
            map.Enable();
            Debug.Log($"Disabled action map: {Map}");
        }
        else
        {
            Debug.LogWarning($"Action map '{Map}' not found.");
        }
    }
    public void DisableActionMap(string Map, InputActionAsset inputActions)//ActionMapを無効
    {
        var map = inputActions.FindActionMap(Map);
        if(map != null)
        {
            map.Disable();
            Debug.Log($"Disabled action map: {Map}");
        }
        else
        {
            Debug.LogWarning($"Action map '{Map}' not found.");
        }
    }
    // uiMap = inputActions.FindActionMap("UI"); UiMap.Disable Enableでも可
    public void SwitchToPlayer(int index)
    {
        // index: 0 = Player1, 1 = Player2
        if (index == 0)
        {
            Player1.gameObject.SetActive(true);
            Player2.gameObject.SetActive(false);
            Debug.Log("Player1 に UI 操作を切り替え");
        }
        else
        {
            Player1.gameObject.SetActive(false);
            Player2.gameObject.SetActive(true);
            Debug.Log("Player2 に UI 操作を切り替え");
        }
    }
    public void ChengeActionMap(PlayerInput input,int PlayerId)
    {
        if(PlayerId == 1)
        {
            input.actions = Charactor1;
        }
        else if(PlayerId == 2)
        {
            input.actions = Charactor2;
        }
    }
}
