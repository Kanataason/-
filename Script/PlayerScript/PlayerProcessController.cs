using UnityEngine;

public class PlayerProcessController : MonoBehaviour
{
    private TatsuAnimationController animationController;
    private PlayerController playerController;
    private Animator animator;
    private PlayerState playerState;
    private MakePlayable playerble;

    [SerializeField] private RuntimeAnimatorController animatorController;
    [SerializeField] private AvatarMask normalMask;
    [SerializeField] private AvatarMask crouchMask;

    private void OnDisable()
    {
        UnSubscribeEvents();
    }
    private void UnSubscribeEvents()//イベント解除
    {
        if (PlayerInitializeManager.Instance != null)
            playerController.OnRemovePos -= PlayerInitializeManager.Instance.RemovePlayerPos;
    }
    private void SubscribeEvents()//イベント登録
    {
        playerController.OnRemovePos += PlayerInitializeManager.Instance.RemovePlayerPos;
    }
    public void SetUpPlayer(string tagName,int playerId)//プレイヤーのセットアップをする関数
    {
        SubscribeEvents();

        playerState.PlayerNumber = playerId;

        gameObject.tag = tagName;
    }
    public void SetUpAnimation()//アニメーション関連の処理セットアップする　１番最初に流れる
    {
        TryGetComponents();

        animator.runtimeAnimatorController = animatorController;

        //参照を渡してあげる アニメーション側がタイミングが違うのでGetComponentできないため
        animationController.SetReferencePlayable(playerble);
    }
    private void TryGetComponents()//コンポーネントを取得
    {
        animationController = GetComponent<TatsuAnimationController>();
        playerController = GetComponent<PlayerController>();
        animator = GetComponent<Animator>();
        playerState = GetComponent<PlayerState>();
        playerble = GetComponent<MakePlayable>();
    }

    public void SetDebagManager(PlayerDebagTextManager manager)//debag用のテキストを参照をコントローラーに渡す処理
    {
        playerController.SetDebagTextManager(manager);
    }
    
}
