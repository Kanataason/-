using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;

public class UiInputManager : MonoBehaviour
{
    public Vector2 moveInput { get; private set; }
    public float moveSpeed = 5f;
    public bool Is_weakAttack { get; private set; }

    public bool Is_nomalAttack { get; private set; }

    public bool Is_strongAttack { get; private set; }

    public bool Is_specialAttack { get; private set; }

    /*---------------Ui用-------------*/

    public Vector2 Navigate { get; private set; }
    public bool Is_Submit { get; private set; }//決定フラグ
    public bool Is_UiCancel { get; private set; }//キャンセルフラグ
                                                 // 入力値の取得（移動）
    /*---------------Ui用-------------*/
   public static GameObject CurrentObj;
    private void OnDestroy()
    {
        CurrentObj = null;
    }
    private void Start()
    {
         CurrentObj = EventSystem.current.currentSelectedGameObject;
    }
    public void Move(InputAction.CallbackContext context)
    {
        moveInput = context.ReadValue<Vector2>();
    }

    // 弱攻撃
    public void weakAttack(InputAction.CallbackContext context)
    {
        if (context.performed)
        {
            Is_weakAttack = true;
            // 弱攻撃のロジックを書く
        }
        else if (context.canceled)
        {
            Is_weakAttack = false;
        }
    }

    // 通常攻撃（ノーマルアタック）
    public void normalAttack(InputAction.CallbackContext context)
    {
        if (context.performed)
        {
            Is_nomalAttack = true;
            // 通常攻撃のロジックを書く（エフェクト再生、当たり判定、アニメーションなど）
        }
        else if (context.canceled)
        {
            Is_nomalAttack = false;
        }
    }

    public void strongAttack(InputAction.CallbackContext context)
    {
        if (context.performed)
        {
            Is_strongAttack = true;
            // 通常攻撃のロジックを書く（エフェクト再生、当たり判定、アニメーションなど）
        }
        else if (context.canceled)
        {
            Is_strongAttack = false;
        }
    }

    public void specialAttack(InputAction.CallbackContext context)
    {
        if (context.performed)
        {
            Is_specialAttack = true;
            // 通常攻撃のロジックを書く（エフェクト再生、当たり判定、アニメーションなど）
        }
        else if (context.canceled)
        {
            Is_specialAttack = false;
        }
    }

    public void middleAttack(InputAction.CallbackContext context)
    {
        if (context.performed)
        {
            Is_specialAttack = true;
            // 通常攻撃のロジックを書く（エフェクト再生、当たり判定、アニメーションなど）
        }
        else if (context.canceled)
        {
            Is_specialAttack = false;
        }
    }



    /*------------------------------------ここからUiの価取得-----------------------------------------*/

    public void OnNavigate(InputAction.CallbackContext ctx)
    {
      
       // Debug.Log(moveInput + "mmm");
        if (ctx.started)
        {
            Navigate = ctx.ReadValue<Vector2>();
        }
        GameObject selected = EventSystem.current.currentSelectedGameObject;
        if(selected != null&&selected != CurrentObj)
        {
            CurrentObj = selected;
            UIAudioSound.Instance.MoveSe();
        }

        if (ctx.canceled)
        {
            Navigate = Vector2.zero;
        }
    }
    public void Submit(InputAction.CallbackContext ctx)//決定ボタン
    {
        if (ctx.performed)
        {
            Debug.Log("Submit Push");
            Is_Submit = true;
        }
        if (ctx.started )
        {

            var selected = EventSystem.current?.currentSelectedGameObject;

            if (selected == null)
            {
                Debug.LogWarning("UI: No selected object - Submit ignored");
                return;
            }

            var button = selected.GetComponent<UnityEngine.UI.Button>();
            if (button == null)
            {
                Debug.LogWarning($"UI: Selected object '{selected.name}' has no Button component");
                return;
            }

           // button.onClick.Invoke();
        }
        if (ctx.canceled)
            Is_Submit = false;
    }
    public void Cancel(InputAction.CallbackContext ctx)//キャンセルボタン
    {

        
        if (ctx.started)
        {
            Debug.Log("あおあおあ");
            Is_UiCancel = true;
        }
        if (ctx.canceled)
        {
            UiEventManager.Instance.Is_First = false;
            Is_UiCancel = false;
        }
    }

    public void Test(InputAction.CallbackContext ctx)
    {
        if (ctx.performed)
        {

        }
    }
    public bool ResetFlag( bool Flag)
    {
        Flag = false;
        Is_UiCancel = false;
        return Flag;
    }
}

