using System;
using UnityEngine;

public class UiUtility : MonoBehaviour
{
    /// <summary>
    /// Uiのテキスト、パネル、ボタンなどのステータスが入っているクラス
    /// </summary>
    [Serializable]
    public static class StateClass
    {
        //必要になるステータスを宣言する
        public enum UiEventType
        {
            GameStart,
            GameStartCancel,
            SetPlayerStart,
            SettingStart,
            SettingCancel,
            SettingDispray,
            SettingVolume,
            SettingPanelCancel,
            ManualStart,
            ManualMove,
            ManualAttack,
            ManualUi,
            ManualPanelCancel,
            ManualCancel,
            MainGameStart,
            BattleStart,
            BattlePose,
            BattlePoseExit,
            BattlePoseSinceMenu,
            Resalt,
            SettingController,
            None
        }

        public enum SpecialEventState
        {
            Setting,
            Manual,
            Battle,
            None
        }
        public enum PanelState
        {
            GameMenu,
            SelectGameMenu,
            Manual,
            SelectCharacter,
            CharacterInfoPlayer1,
            CharacterInfoPlayer2,
            BattlePose,
            PlayerSet,
            MenuMove,
            MenuAttack,
            MenuUi,
            MainSetting,
            SettingDisplay,
            SettingVolume,
            Result,
            SettingController,
            None
        }

        public enum ButtonState
        {
            FirstGameSelect,
            FirstGameStart,
            FirstSettingStart,
            SelectCharacterStart,
            FirstBattlePose,
            FirstManual,
            FirstPlayerSet,
            SettingDisplay,
            SettingVolume,
            Resalt,
            SettingController,
            SettingSoloKeyBoard,
            None
        }
        public enum SliderState
        {
            ManualMove,
            ManualAttack,
            ManualUi
        }
        public enum TextState
        {
            CurrentController,
        }
    }

    public static class TimeClass
    {
        public static void TimeStop(MonoBehaviour owner,Func<bool> func,float startTimeScale,float normalTimeScale)
        {
            owner.StartCoroutine(HisStop(func,startTimeScale,normalTimeScale));
        }
        private static System.Collections.IEnumerator HisStop(Func<bool> func,float startTime,float normalTimeScale)
        {
            Time.timeScale = startTime;

            yield return new WaitUntil(func);

            Time.timeScale = normalTimeScale;
            yield return null;
        }
    }

    /// <summary>
    /// 画面に影響がある関数のクラス
    /// </summary>
    public static class DisplayEfect
    {
        /// <summary>
        /// ズームをさせる
        /// </summary>
        public static void ZoomIn(MonoBehaviour owner, Camera camera, float duration = 1f)
        {
            if (camera == null) return;
            owner.StartCoroutine(UpdateZoomIn(camera, duration));
        }
        private static System.Collections.IEnumerator UpdateZoomIn(Camera camera, float duration)
        {
            float targetFov = 1f;
            float startFov = camera.fieldOfView;
            float t = 0f;
            
            //渡されたdurationまで処理をする
            while (t < duration)
            {
                t += Time.deltaTime;

                //カメラの焦点を縮めていく
                camera.fieldOfView =
                    Mathf.Lerp(startFov, targetFov, t / duration);

                yield return null;
            }

            //最終的な焦点を合わせる
            camera.fieldOfView = targetFov;
        }

        /// <summary>
        /// 渡された数値によってフェードさせる
        /// </summary>
        public static void Fade(MonoBehaviour owner,UnityEngine.UI.Image fadeImage, float startAlpha, float endAlpha, float duration = 0.5f)
        {
            if (fadeImage == null) return;

            owner.StartCoroutine(
                UpdateFade(fadeImage, startAlpha, endAlpha, duration));
        }

        private static System.Collections.IEnumerator UpdateFade(UnityEngine.UI.Image fadeImage, float startAlpha, float endAlpha, float duration)
        {
            float t = 0f;
            Color c = fadeImage.color;

            while (t < duration)
            {
                t += Time.deltaTime;

                //渡されたスタートのアルファ値に応じてfadeOutとfadeInかを分ける
                float alpha = Mathf.Lerp(
                    startAlpha,
                    endAlpha,
                    t / duration);

                fadeImage.color =
                    new Color(c.r, c.g, c.b, alpha);

                yield return null;
            }

            fadeImage.color =
                new Color(c.r, c.g, c.b, endAlpha);
        }
    }
}
