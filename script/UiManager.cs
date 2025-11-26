using System.Collections;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
public class UiManager : MonoBehaviour
{
    public static UiManager Instance { get; private set; }

    [Header("Main")]
    public bool isTest = false;

    [SerializeField] private float F_maxPerGauge = 10000;
    [Header("UiRefarence")]
    [SerializeField] private TextMeshProUGUI Ui_DeadText;
    [Header("プレイヤー情報")]
    [SerializeField] private Image Ui_Player1Image;
    [SerializeField] private Image Ui_Player1Imagered;
    [SerializeField] private Image Ui_Player2Image;
    [SerializeField] private Image Ui_Player2Imagered;
    [SerializeField] private Image[] levelBarsplayer1; // 3本のゲージ
    [SerializeField] private Image[] LevelBarsPlayer2;

    private float F_currentplayer1red = 1f;
    private float F_currentplayer2red = 1f;
    private float F_currentplayer1white = 1f;
    private float F_currentplauer2white = 1f;

    private float F_CurrentGauge1 = 0;
    private float F_CurrentGauge2 = 0;
    [Header("参照")]
    public PlayerState Playerstate1;
    public PlayerState Playerstate2;
    [SerializeField] Animator animator;
    private Coroutine coroutinePlayer1;
    private Coroutine coroutinePlayer2;
    [SerializeField] private UITextMove uITextMove;
    [SerializeField] private UITextMove TimeOverT;

    [SerializeField] private UiHpBer player1HpUI;
    [SerializeField] private UiHpBer player2HpUI; 
    private bool isRunningPlayer1;
    private bool isRunningPlayer2;
    private void Awake()//シングルトン形式にして一つしかないことを明示化
    {
        if(Instance == null)
        {
            Instance = this;
        }
        else
        {
            Destroy(Instance);
        }
    }
    private void OnDestroy()
    {
        Destroy(gameObject);
    }
    private void Start()
    {
        // Ui_DeadText.enabled = false;
    }
    public void Setinfp(UiHpBer uiHpbar, UiHpBer uiHpbar1)
    {
        player1HpUI = uiHpbar;
        player2HpUI = uiHpbar1;
    }
    public void Sethp(PlayerState player1, PlayerState player2)
    {
        player1HpUI.SetTarget(player1, PlayerState.GaugeType.Hp);
        player2HpUI.SetTarget(player2, PlayerState.GaugeType.Hp);
    }
    public void SetState(PlayerState playerState,int Id)
    {
        if (Id == 1) Playerstate1 = playerState;
        else Playerstate2 = playerState;
        
    }
    // Start is called once before the first execution of Update after the MonoBehaviour is created
    public void SetHp(int I_playerid, float F_NowHp, float Flame, PlayerState state)
    {
        state.PState = PLayerState.BegingAttacked;
        float hpRate = F_NowHp / state.MaxHp;

        if (I_playerid == 1)
        {
            F_currentplayer1red = hpRate;
            Ui_Player1Imagered.fillAmount = F_currentplayer1red;

            if (coroutinePlayer1 != null)
                StopCoroutine(coroutinePlayer1);
            coroutinePlayer1 = StartCoroutine(SmoothPlayerUi(1, hpRate, Flame, state));
        }
        else if (I_playerid == 2)
        {
            F_currentplayer2red = hpRate;
            Ui_Player2Imagered.fillAmount = F_currentplayer2red;

            if (coroutinePlayer2 != null)
                StopCoroutine(coroutinePlayer2);
            coroutinePlayer2 = StartCoroutine(SmoothPlayerUi(2, hpRate, Flame, state));
        }
    }
    private IEnumerator SmoothPlayerUi(int I_playerId, float F_targetHpRate, float flame, PlayerState state)
    {
      //  float time = 0;

        // 多重起動防止（最初に return しないようにする）
        if (I_playerId == 1 && isRunningPlayer1)
        {
            StopCoroutine(coroutinePlayer1);
        }
        else if (I_playerId == 2 && isRunningPlayer2)
        {
            StopCoroutine(coroutinePlayer2);
        }

        if (I_playerId == 1) isRunningPlayer1 = true;
        else isRunningPlayer2 = true;

        // 少し待つ（赤が表示されたあと遅れて白が追いかける演出）
        yield return new WaitForSeconds(flame);

        if (I_playerId == 1)
        {
            while (Mathf.Abs(F_currentplayer1white - F_targetHpRate) > 0.01f)
            {
                F_currentplayer1white = Mathf.Lerp(F_currentplayer1white, F_targetHpRate, Time.deltaTime * 5f);
                Ui_Player1Image.fillAmount = F_currentplayer1white;
                yield return null;
            }
            F_currentplayer1white = F_targetHpRate;
            Ui_Player1Image.fillAmount = F_currentplayer1white;
            isRunningPlayer1 = false;
        }
        else if (I_playerId == 2)
        {
            while (Mathf.Abs(F_currentplauer2white - F_targetHpRate) > 0.01f)
            {
                F_currentplauer2white = Mathf.Lerp(F_currentplauer2white, F_targetHpRate, Time.deltaTime * 5f);
                Ui_Player2Image.fillAmount = F_currentplauer2white;
                yield return null;
            }
            F_currentplauer2white = F_targetHpRate;
            Ui_Player2Image.fillAmount = F_currentplauer2white;
            isRunningPlayer2 = false;
        }

        state.ResetNomal();
        state.ComboCount = 0;
    }
    public void SetInstantHp(float F_NowHp)
    {
        //F_currentred = F_currentwhite = F_NowHp;
        //Ui_Player1Imagered.fillAmount = F_NowHp;
        //Ui_Player1Image.fillAmount = F_NowHp;
        //Ui_Player2Image.fillAmount = F_NowHp;
        //Ui_Player2Imagered.fillAmount = F_NowHp;
    }
    public void TimeOver()
    {
        StartCoroutine(PlayKoDirector(TimeOverT));
    }
    public void DeadUi()
    {
        StartCoroutine(PlayKoDirector(uITextMove));
    }

    IEnumerator PlayKoDirector(UITextMove text)
    {
        Time.timeScale = 0.3f;
        yield return new WaitForSeconds(0.5f);
        Time.timeScale = 1;
        text.DeadText();
    }
    public void SetGauge(float value,int playerid)
    {
        if (playerid == 1)
        {
            F_CurrentGauge1 = Mathf.Clamp(F_CurrentGauge1 + value, 0, F_maxPerGauge * levelBarsplayer1.Length);
            UpdataGaugeUi(F_CurrentGauge1,levelBarsplayer1);//現在のゲージと受けたダメージが, 10000*3の範囲のどこにいるか
        }
        else if(playerid == 2)
        {
            F_CurrentGauge2 = Mathf.Clamp(F_CurrentGauge2 + value, 0, F_maxPerGauge * LevelBarsPlayer2.Length);
            UpdataGaugeUi(F_CurrentGauge2,LevelBarsPlayer2);
        }
    }
    private void UpdataGaugeUi(float F_PlayerGauge, Image[] Gauge)//増やす値,プレイヤーのゲージ
    {
        float remaining = F_PlayerGauge;
        

        for (int i = 0; i < Gauge.Length; i++)
        {
            if(remaining >= F_maxPerGauge)
            {
                Gauge[i].fillAmount = 1;
                remaining -= F_maxPerGauge;
            }
            else
            {
                float fill = (float)remaining / F_maxPerGauge;
                Gauge[i].fillAmount = fill;
                remaining = 0;
            }
        }
    }
}
