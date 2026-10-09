#pragma once
#include <cassert>
#include <cstddef>
#include <new>
#include <utility>


template<typename T,std::size_t Capacity>
class ObjectPool
{
public:
	ObjectPool()
	{
		//次の参照先を設定して、すべてアクティブじゃなくす
		for (std::size_t slotIndex = 0 ; slotIndex < Capacity - 1 ; slotIndex++)
		{
			_slots[slotIndex].nextFreeSlot = &_slots[slotIndex + 1];
			_isActive[slotIndex] = false;
		}
		//初期化設定、最後のスロットだけnullptrにすることで
		//範囲を超えて参照をしないようにするため
		_slots[Capacity - 1].nextFreeSlot = nullptr;
		_isActive[Capacity - 1] = false;
		_firstFreeSlot = &_slots[0];
	}
	~ObjectPool()
	{
		//アクティブのデストラクタを呼ぶ
		for (std::size_t slotIndex = 0;slotIndex < Capacity; slotIndex++)
		{
			if (_isActive[slotIndex])
			{
				GetObjectAt(slotIndex)->~T();
			}
		}
	}

    //コピーはさせない
	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;

    //Moveもさせない
	ObjectPool(ObjectPool&&) = delete;
	ObjectPool& operator=(ObjectPool&&) = delete;
    //空きスロットにオブジェクトを生成して返す。満杯なら nullptr
    template <typename... ConstructorArgs>
    T* Acquire(ConstructorArgs&&... constructorArgs)
    {
        if (_firstFreeSlot == nullptr)
        {
            return nullptr;
        }

        Slot* acquiredSlot = _firstFreeSlot;
        _firstFreeSlot = acquiredSlot->nextFreeSlot;

        //確保済みメモリの上でコンストラクタだけを呼ぶ
        //forwardは渡された引数を存在する変数か、一時変数か分けて生成する
        T* createdObject = new (acquiredSlot->storage) T(std::forward<ConstructorArgs>(constructorArgs)...);

        _isActive[GetSlotIndex(acquiredSlot)] = true;
        ++_activeCount;
        return createdObject;
    }

    //オブジェクトを破棄してスロットをフリーリストに戻す
    void Release(T* releasedObject)
    {
        assert(releasedObject != nullptr);

        //弾のポインタをスロットのポインタとして変換する
        Slot* releasedSlot = reinterpret_cast<Slot*>(releasedObject);
        const std::size_t slotIndex = GetSlotIndex(releasedSlot);

        assert(slotIndex < Capacity && "このプールのオブジェクトではありません");
        assert(_isActive[slotIndex] && "二重に Release されています");

        //デストラクタだけを呼ぶ。メモリは解放しない
        releasedObject->~T();

        //アクティブじゃなくして、先頭を更新する
        _isActive[slotIndex] = false;
        releasedSlot->nextFreeSlot = _firstFreeSlot;
        _firstFreeSlot = releasedSlot;
        --_activeCount;
    }

    //使用中のオブジェクトすべてに処理を行う
    template <typename Function>
    void ForEachActive(Function function)
    {
        for (std::size_t slotIndex = 0; slotIndex < Capacity; ++slotIndex)
        {
            if (_isActive[slotIndex])
            {
                function(*GetObjectAt(slotIndex));
            }
        }
    }

    //ステージ切り替え時などに全オブジェクトを返却する
    void ReleaseAll()
    {
        for (std::size_t slotIndex = 0; slotIndex < Capacity; ++slotIndex)
        {
            if (_isActive[slotIndex])
            {
                Release(GetObjectAt(slotIndex));
            }
        }
    }

    std::size_t GetActiveCount() const { return _activeCount; }
    std::size_t GetCapacity() const { return Capacity; }
    bool IsFull() const { return _firstFreeSlot == nullptr; }

private:
	union Slot
	{
        //alignasは開始位置をそろえて境界をそろえる
        alignas(T) unsigned char storage[sizeof(T)];
		Slot* nextFreeSlot;
	};

	std::size_t GetSlotIndex(const Slot* slot) const
	{
		return static_cast<std::size_t>(slot - _slots);
	}

	T* GetObjectAt(std::size_t slotIndex)
	{
		return std::launder(reinterpret_cast<T*>(_slots[slotIndex].storage));
	}

	Slot _slots[Capacity];
	bool _isActive[Capacity];
	Slot* _firstFreeSlot = nullptr;
	std::size_t _activeCount = 0;
};