using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;

[Serializable]
public class ObjectPool<T> where T:Enum
{
    /// <summary>
    /// 自分で型を設定をしてディクショナリーに保存するためのクラス
    /// </summary>
    [Serializable]
    public class Entry
    {
        public T Key;//属性
        public GameObject Value;//属性に合ったオブジェクト
        public int InstantiateCount;//何個生成するか
    }
    //生成したいオブジェクトの分インスペクターで設定
    [SerializeField] private List<Entry> _ObjectList;

    //生成した元のプレハブを保存ディクショナリー
    private Dictionary<T, GameObject> _prefabList = new Dictionary<T, GameObject>();
    //生成したインスタンスを保存するディクショナリー
    private Dictionary<T, Queue<GameObject>> _objectPool = new Dictionary<T, Queue<GameObject>>();

    public void InitList()//リストを初期化
    {
        //リストが存在してなければ新しく生成
        _prefabList ??= new();
        _objectPool ??= new();

        //中身を初期化
        _prefabList.Clear();
        _objectPool.Clear();

        //リストで設定した情報をディクショナリーに設定
        foreach (var obj in _ObjectList)
        {
            _prefabList[obj.Key] = obj.Value;
            _objectPool[obj.Key] = new Queue<GameObject>();

            //設定した数だけ生成
            for (int i = 0; i < obj.InstantiateCount; i++)
            {
                var pre = MonoBehaviour.Instantiate(obj.Value,Vector3.zero,Quaternion.identity);
                pre.SetActive(false);
                _objectPool[obj.Key].Enqueue(pre);
            }
        }
        Debug.Log(_objectPool.Count);
    }
    public GameObject Get(T type,Transform pos)//生成したものを取得
    {
        //リストに引数のタイプが設定されてなかったら返す
        if (!_objectPool.ContainsKey(type))
        {
            return null;
        }

        var obj = _objectPool[type];
        //クエリに存在していなかったら生成、存在していたら取り出してオブジェクトを返す
        if (obj.Count > 0)
        {
            var pre = obj.Dequeue();
            pre.SetActive(true);
            pre.transform.position = pos.position;
            return pre;
        }
        else
        {
            var pre = MonoBehaviour.Instantiate(_prefabList[type]);
            pre.SetActive(true);
            pre.transform.position = pos.position;
            return pre;
        }
    }
    public GameObject GetPrefab(T type)//インスタンスを取得
    {
        //生成元のリストに引数のタイプが設定されていたら返す
        if (_prefabList.TryGetValue(type,out var obj))
        {
            return obj;
        }
        return null;
    }
    public void ReturnObject(T type, GameObject obj)//返却処理
    {
        if (obj == null) return;

        obj.SetActive(false);
        _objectPool[type].Enqueue(obj);
    }


}

[Serializable]
public class AddresableObjectPool<T>where T:Enum
{
    [Serializable]
    public class Entry
    {
        public T Key;
        public AssetReferenceT<GameObject> Value;//生成元のオブジェクト
        public int InstantiateCount;//何個生成するか
    }

    [SerializeField] private List<Entry> entryList;

    //非同期でロードが完了したものを保管するリスト
    private Dictionary<T, AsyncOperationHandle<GameObject>> loadedHandleList = new();

    //生成したものを保管するリスト
    private Dictionary<T, Queue<GameObject>> cacheList = new();
    public async Task Init(Transform parent = null)
    {
        //リストが無かったら新しく生成
        loadedHandleList ??= new();
        cacheList ??= new();

        //中身を空にする
        loadedHandleList.Clear();
        cacheList.Clear();

        foreach (var entry in entryList)
        {
            //ゲームオブジェクトを非同期でロード
            var handle = entry.Value.LoadAssetAsync<GameObject>();
            
            //ロード中
            await handle.Task;

            if (handle.Status != AsyncOperationStatus.Succeeded)//例外が発生したら戻す
                continue;

            loadedHandleList[entry.Key] = handle;//完成したら生成元を保管

            var queue = new Queue<GameObject>();

            //指定した回数分生成する
            for (int i = 0; i < entry.InstantiateCount; i++)
            {
                var instance = GameObject.Instantiate(handle.Result,parent);
                instance.SetActive(false);

                queue.Enqueue(instance);
            }

            cacheList[entry.Key] = queue;
        }
    }

    public GameObject Get(T state)
    {

        if (!cacheList.TryGetValue(state, out var queue))
            return null;

        if (queue.Count > 0)
        {
            var obj = queue.Dequeue();
            obj.SetActive(true);
            return obj;
        }

        return null;
    }
    public GameObject GetPefab(T state)
    {
        //生成元を取得
      if (loadedHandleList.TryGetValue(state, out var obj))
        {
            return obj.Result;
        }
        return null;

    }
    public void Return(T state, GameObject obj)//エフェクトを返す
    {

        obj.SetActive(false);

        cacheList[state].Enqueue(obj);
    }
    /// <summary>
    /// キャッシュされているオブジェクトを消したり、メモリを解放する
    /// </summary>
    public void Release()//シーンを移動したり、オブジェクトが消えたらよばれる
    {

        foreach (var queue in cacheList.Values)
        {
            while (queue.Count > 0)
            {
                var obj = queue.Dequeue();

                if (obj != null)
                {
                    GameObject.Destroy(obj);
                }
            }
        }

        foreach (var handle in loadedHandleList.Values)
        {
            Addressables.Release(handle);
        }

        cacheList.Clear();
        loadedHandleList.Clear();
    }
}