using System.Collections;
using UnityEngine;
using UnityEngine.UI;

public class UiHpBer : MonoBehaviour
{
   // private PlayerState.Gauge gauge;
    private PlayerState targetStatus;

    [SerializeField] public Image immediateBar; // 赤バー
    [SerializeField] public Image delayedBar;   // 白バー

    private Coroutine smoothCoroutine;

    //public void SetTarget(PlayerState status, PlayerState.GaugeType type)
    //{
    //    if (gauge != null)
    //        gauge.OnValueChanged -= OnGaugeValueChanged;

    //    targetStatus = status;
    //    gauge = status.GetGauge(type);

    //    if (gauge != null)
    //    {
    //        gauge.OnValueChanged += OnGaugeValueChanged;
    //        UpdateImmediate(gauge.Current, gauge.Max);
    //        UpdateDelayed(gauge.Current, gauge.Max); // 最初は同じに
    //    }
    //    else
    //    {
    //        Debug.LogError("Gauge is null. Type: " + type);
    //    }
    //}

    private void OnGaugeValueChanged(float current, float max)
    {
        float hpRate = Mathf.Clamp01(current / max);
        UpdateImmediate(current, max);

        if (smoothCoroutine != null)
            StopCoroutine(smoothCoroutine);
        smoothCoroutine = StartCoroutine(SmoothDelayedBar(hpRate, 0.3f));
    }

    private void UpdateImmediate(float current, float max)
    {
        if (immediateBar != null)
            immediateBar.fillAmount = Mathf.Clamp01(current / max);
    }

    private void UpdateDelayed(float current, float max)
    {
        if (delayedBar != null)
            delayedBar.fillAmount = Mathf.Clamp01(current / max);
    }

    private IEnumerator SmoothDelayedBar(float target, float duration)
    {
        float time = 0f;
        float start = delayedBar.fillAmount;

        while (time < duration)
        {
            time += Time.deltaTime;
            float t = time / duration;
            delayedBar.fillAmount = Mathf.Lerp(start, target, t);
            yield return null;
        }

        delayedBar.fillAmount = target;
    }
}
