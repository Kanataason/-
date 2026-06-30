using System;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class TestUi : MonoBehaviour
{
    [SerializeField] private Image Player1Red;
    [SerializeField] private Image Player2Red;

    [SerializeField] private Image Player1White;
    [SerializeField] private Image Player2White;

     [SerializeField] private Image[] Player1WinMarks; // プレイヤー1の○マーク
    [SerializeField] private Image[] Player2WinMarks; // プレイヤー2の○マーク

    [SerializeField] private Image[] LevelBarsplayer1; // 3本のゲージ
    [SerializeField] private Image[] LevelBarsPlayer2;

    [SerializeField] private TextMeshProUGUI PowerTimer1;
    [SerializeField] private TextMeshProUGUI PowerTimer2;

    [SerializeField] private TextMeshProUGUI Gauge1;
    [SerializeField] private TextMeshProUGUI Gauge2;

    public List<PlayerInfo> infosList = new List<PlayerInfo>();
    void Start()
    {
        SubscribeEvents();

        //WinUiの更新
        BattleStateManager.Instance.InitDisplayWinUiImage();
    }
    private void OnDisable()
    {
        UnSubscribeEvents();
    }
    private void SubscribeEvents()
    {
        if (BattleStateManager.Instance != null)
            BattleStateManager.Instance.OnChangeWinUiAction += OnUpdateWinUI;
    }
    private void UnSubscribeEvents()
    {
        if (BattleStateManager.Instance != null)
            BattleStateManager.Instance.OnChangeWinUiAction -= OnUpdateWinUI;
        foreach (var info in infosList) // メモリリーク防ぐ
        {
            if (info != null && info.state != null)
            {
                Debug.Log("Event解除");
                var Plstate = info.state;
                Plstate.OnHpbarChange -= OnHpChanged;
                Plstate.OnOffHpBarChange -= OnUpdateDelayedHpBar;
                Plstate.OnGaugeChange -= OnGaugeChanged;
                Plstate.OnPowerUptime -= OnStartPowerTimer;
            }
        }
        infosList.Clear();
    }

    public void InitializePlayerUI(PlayerState state)//生成されたタイミングで設定
    {
        var existingInfo = GetPlayerInfo(state);
        if (existingInfo != null)//event 解除
        {
            //event
            state.OnHpbarChange -= OnHpChanged;
            state.OnOffHpBarChange -= OnUpdateDelayedHpBar;
            state.OnGaugeChange -= OnGaugeChanged;
            state.OnPowerUptime -= OnStartPowerTimer;
        }
        PlayerInfo info = null;
        if (state.PlayerNumber == 1)
        {
            state.SetPlayerInfo(PlayerDataManager.Instance.Player1Data.CostomData);
            if (infosList.Count < 1)
                infosList.Add(new PlayerInfo());
            info = infosList[0];
        }
        else if (state.PlayerNumber == 2)
        {
            state.SetPlayerInfo(PlayerDataManager.Instance.Player2Data.CostomData);
            if (infosList.Count < 2)
                infosList.Add(new PlayerInfo());
            info = infosList[1];
        }
        info.NowHp = state.CurrentHp;
        info.MaxHp = state.MaxHp;
        info.NowGauge = state.CurrentPlayerGauge;
        info.MaxGauge = state.PlayerMaxGauge;
        info.state = state;

        state.OnHpbarChange += OnHpChanged;
        state.OnOffHpBarChange += OnUpdateDelayedHpBar;
        state.OnGaugeChange += OnGaugeChanged;
        state.OnPowerUptime += OnStartPowerTimer;
    }
    private void OnUpdateWinUI(int Player1Win, int Player2Win)
    {
        //// プレイヤー1の○を更新
        //for (int i = 0; i < Player1WinMarks.Length; i++)
        //{
        //    if (i < Player1Win)
        //    {
        //        Player1WinMarks[i].color = Color.red; // 勝った分だけ赤
        //    }
        //    else
        //    {
        //        Player1WinMarks[i].color = Color.white; // 残りは白
        //    }
        //}

        //// プレイヤー2の○を更新
        //for (int i = 0; i < Player2WinMarks.Length; i++)
        //{
        //    if (i < Player2Win)
        //    {
        //        Player2WinMarks[i].color = Color.red;
        //    }
        //    else
        //    {
        //        Player2WinMarks[i].color = Color.white;
        //    }
        //}
        UpdateWinMarks(Player1WinMarks, Player1Win);
        UpdateWinMarks(Player2WinMarks, Player2Win);
    }
    private void UpdateWinMarks(Image[] marks, int winCount)
    {
        for (int i = 0; i < marks.Length; i++)
        {
            marks[i].color = i < winCount ? Color.red : Color.white;
        }
    }
    private void OnGaugeChanged(PlayerState state,bool IsAdd,float value)
    {
        //CurrentGaugeP1 = 0;
        //CurrentGaugeP2 = 0;
        //if(state.PlayerNumber == 1)
        //{
        //    if (IsAdd)
        //    {
        //        CurrentGaugeP1 = Mathf.Clamp(CurrentGaugeP1 + value, 0, 10000f * LevelBarsplayer1.Length);
        //        Gauge1.text = ((int)value).ToString();
        //        UpdateGauge(value, LevelBarsplayer1);
        //    }
        //    else
        //    {
        //        Gauge1.text = ((int)value).ToString();
        //        RefreshGaugeUI(value, LevelBarsplayer1, state);
        //    }
        //}
        //else if(state.PlayerNumber == 2)
        //{
        //    if (IsAdd)
        //    {
        //        CurrentGaugeP2 = Mathf.Clamp(CurrentGaugeP2 + value, 0, 10000f * LevelBarsPlayer2.Length);
        //        Gauge2.text = ((int)CurrentGaugeP2).ToString();
        //        UpdateGauge(CurrentGaugeP2,LevelBarsPlayer2);
        //    }
        //    else
        //    {
        //        Gauge2.text = ((int)value).ToString();
        //        RefreshGaugeUI(value, LevelBarsPlayer2, state);
        //    }
        //}
        UpdatePlayerGaugeUI(state, value);
    }
    private void UpdatePlayerGaugeUI(PlayerState state, float value)
    {
        var gaugeText = state.PlayerNumber == 1 ? Gauge1 : Gauge2;
        var gaugeBar = state.PlayerNumber == 1 ? LevelBarsplayer1 : LevelBarsPlayer2;

        gaugeText.text = ((int)value).ToString();
        UpdateGauge(value, gaugeBar);
    }
    private void RefreshGaugeUI(float value, Image[] gauge, PlayerState state)
    {
        float remaining = value;

        // UI更新
        float temp = remaining; // 残量をコピーしてUI計算用に使う
        for (int i = 0; i < gauge.Length; i++)
        {
            if (temp >= 10000f)
            {
                gauge[i].fillAmount = 1f;
                temp -= 10000f;
            }
            else if (temp > 0f)
            {
                float fill = temp / 10000f;
                gauge[i].fillAmount = fill;
                temp = 0f;
            }
            else
            {
                gauge[i].fillAmount = 0f; // 残量が無ければ0をセット
            }
        }
    }
    private void UpdateGauge(float value, Image[] gauge)
    {
        float remaining = value;

        for (int i = 0; i < gauge.Length; i++)
        {
            if (remaining >= 10000f)
            {
                gauge[i].fillAmount = 1f;
                remaining -= 10000f;
            }
            else
            {
                gauge[i].fillAmount = remaining / 10000f;
                remaining = 0f;
            }
        }
    }

    //パワーアップの残り時間を非同期で表示させる
    private void OnStartPowerTimer(PlayerState state,float timer)
    {
        StartCoroutine(PowerTimerCoroutine(timer, state.PlayerNumber,state));
    }
    System.Collections.IEnumerator PowerTimerCoroutine(float timer, int Id,PlayerState state)
    {
        var text = Id == 1 ? PowerTimer1 : PowerTimer2;
        float elapsed = timer;
        BattleStateManager battleState = BattleStateManager.Instance;

        while (elapsed > 0)
        {
            //GameStartになったら強制終了
            if (battleState.CurrentBattleState is BattleStateManager.BattleState.GameStating)
                break;

            text.text = ((int)elapsed).ToString();
            elapsed -= Time.deltaTime;
            yield return null;
        }
        text.text = "";
        state.OnStartPowerUp();
    }

    private void OnHpChanged(PlayerState state)
    {
        SetWhiteHpBarVisible(true, state);
        //var info = GetPlayerInfo(state);
        //if(info != null)
        //{
        //    float radio = state.CurrentHp / info.MaxHp;//normalize
        //    if (state.PlayerNumber == 1) Player1Red.fillAmount = radio;
        //    else if (state.PlayerNumber == 2) Player2Red.fillAmount = radio;
        //}
        UpdateHpBar(state, true);
    }
    private void OnUpdateDelayedHpBar(PlayerState state)
    {
        SetWhiteHpBarVisible(false,state);
        //var info = GetPlayerInfo(state);
        //if (info != null)
        //{
        //    float radio = state.CurrentHp / info.MaxHp;//normalize
        //    if (state.PlayerNumber == 1) Player1White.fillAmount = radio;
        //    else if (state.PlayerNumber == 2) Player2White.fillAmount = radio;
        //}
        UpdateHpBar(state, false);
    }
    private void UpdateHpBar(PlayerState state, bool isRedBar)
    {
        SetWhiteHpBarVisible(isRedBar, state);

        var info = GetPlayerInfo(state);
        if (info == null) return;

        Image target = state.PlayerNumber == 1
            ? (isRedBar ? Player1Red : Player1White)
            : (isRedBar ? Player2Red : Player2White);

        target.fillAmount = state.CurrentHp / info.MaxHp;
    }
    private PlayerInfo GetPlayerInfo(PlayerState state)=> infosList.Find(i => i.state == state);
    private void SetWhiteHpBarVisible(bool IsOn,PlayerState state)
    {
        Image target = state.PlayerNumber == 1 ? Player1White : Player2White;
        Color c = target.color;
        c.a = IsOn ? 1 : 0;
        target.color = c;
    }

}

[Serializable] 
public class PlayerInfo
{
    public float NowHp;
    public float MaxHp;
    public float NowGauge;
    public float MaxGauge;
    public PlayerState state;
}
