using System;
using UnityEngine;

public class BattlePresentationManager : MonoBehaviour
{
    public static BattlePresentationManager Instance { get; private set; }

    private void Awake()
    {
        Instance = this;

        if (Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(this.gameObject);
        }
        else
        {
            Destroy(Instance);
        }
    }

}
