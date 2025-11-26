using UnityEngine;
using System.Collections.Generic;
public enum EffectType
{
    Hit,
    Guard,
    Step,
    Smash,
    PowerUp,
    Charge
}
public class PlayerObjctPool : MonoBehaviour
{
    [System.Serializable]
    public class EffectData
    {
        public EffectType type;
        public GameObject prefab;
        public int initialCount = 5; 
    }

    [SerializeField] private List<EffectData> effectDataList;

    private Dictionary<EffectType, Queue<GameObject>> effectPools = new();
    private Dictionary<EffectType, GameObject> effectPrefabs = new();

    void Awake()
    {
        // èâä˙âª
        foreach (var data in effectDataList)
        {
            effectPrefabs[data.type] = data.prefab;
            effectPools[data.type] = new Queue<GameObject>();

            for (int i = 0; i < data.initialCount; i++)
            {
                var obj = Instantiate(data.prefab, transform); // PoolÇÃéqÇ…ÇµÇƒä«óù
                obj.SetActive(false);
                effectPools[data.type].Enqueue(obj);
            }
        }
    }

    public GameObject GetEffect(EffectType type)
    {
        if (!effectPools.ContainsKey(type))
        {
            //Debug.LogWarning($"EffectType {type} ÇÕñ¢ìoò^Ç≈Ç∑");
            return null;
        }

        Queue<GameObject> pool = effectPools[type];
        if (pool.Count > 0)
        {
            var obj = pool.Dequeue();
            obj.SetActive(true);
            return obj;
        }
        else
        {
            var obj = Instantiate(effectPrefabs[type],transform);
            obj.SetActive(true);
            return obj;
        }
    }

    public void ReturnEffect(EffectType type, GameObject obj)
    {
        if (obj == null) return;
        obj.SetActive(false);
        effectPools[type].Enqueue(obj);
    }
}
