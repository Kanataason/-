using System.Collections;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

/// <summary>
/// Uiの種類分ステータスを宣言
/// </summary>
public enum UiState 
{ 
   Title,
   Home,
   Setting,
   Manual,
   SetPlayer,
   GameStart,
   SelectCharactor,
   MainGame,
   Battle,
   Pose,
   SettingInfo,
   ManualInfo,
   None
}
public class TatuGameManager : MonoBehaviour
{
    //現在のプレイ人数ステート
    public enum PlayMode
    {
        Solo,
        Multi
    }
    [Header("Reference")]
    public static TatuGameManager Instance { get; private set; }


    public UiState CurrentUiState = UiState.Title;
    public PlayMode CurrentMode;//プレイモード

    public Volume Volume;//画面のエフェクト設定用
    public Image FadeImage;//画面の明るさ設定用

    public bool IsSoloKeyboardMode = false;

    //カーソルをロックしているかの変数
    private float moveThreshold = 1f;
    private bool isCursorLocked = false;


    //シーンをまたいで保持したいからシングルトン
    private void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
        }
    }

    void Start()
    {
        Application.targetFrameRate = 60;//フレームを60に固定
        Cursor.lockState = CursorLockMode.Locked;//カーソルを隠す
    }
    private void Update()
    {
        //プレイヤーの入力を取得
        Vector2 delta = Mouse.current.delta.ReadValue();

        var anyButton = Input.anyKeyDown;

        if (anyButton && isCursorLocked)
        {
            SetCorsorLock();
        }
        // 少しでも動いたら解除 sqrは二乗で返すから値も二乗しないといけない
        if (delta.sqrMagnitude > moveThreshold * moveThreshold && !isCursorLocked)
        {
            SetCorsorLock(true);
        }

    }
    //カーソルを表示させる関数
    private void SetCorsorLock(bool Active = false)
    {
        CursorLockMode mode = Active ? CursorLockMode.None : CursorLockMode.Locked;

        //カーソルの情報をセット
        Cursor.lockState = mode;
        Cursor.visible = Active;
        isCursorLocked = Active;

    }
    /// <summary>
    /// 指定したシーンへ遷移する
    /// </summary>
    public void ChangeScene(string Name)
    {
        SceneManager.LoadScene(Name);
    }
    /// <summary>
    /// UIの状態を変更する
    /// </summary>
    /// <param name="name"></param>
    public void ChangeState(UiState name)
    {
        CurrentUiState = name;
        Debug.Log($"{CurrentUiState}+ に変更");   
    }
    /// <summary>
    /// プレイモードを変更する
    /// </summary>
    public void ChangePlayMode(PlayMode mode)
    {
        CurrentMode = mode;
        Debug.Log($"{CurrentMode}+ に変更");
    }

    /// <summary>
    /// この変数を有効にすることでキーボードだけでプレイすることができる
    /// </summary>
    public void ChangeSoloKeyBoard()//キーボード単独仕様フラグ
    {
        IsSoloKeyboardMode = !IsSoloKeyboardMode;
        InputUserMakeManager.Instance.ChangeController();//プレイヤーの入力を新しく登録
    }
    public void ExitGame()//ゲーム終了処理する関数
    {
        StartCoroutine(ExitTime());
    }

    public void LoadNextStage(string name,UiState uiState)//タイトルから呼ばれる
    {
        ChangeState(uiState);
        ChangeScene(name);
    }
     IEnumerator ExitTime()//ゲームを終わるための関数
    {
        yield return new WaitForSeconds(1);
#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
#else
    Application.Quit();
#endif
    }

}
