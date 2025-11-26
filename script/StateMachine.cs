using System.Collections.Generic;
using UnityEngine;
using static UnityEngine.UI.GridLayoutGroup;

public class StateMachine<TOwner> 
{
    public abstract class State
    {

        protected StateMachine<TOwner> StateMachine => stateMachine;//こう書くことによって継承したところでも使うことができる
        internal StateMachine<TOwner> stateMachine;//stateMacineとStateでしか参照できない。

        internal Dictionary<int, State> state = new Dictionary<int, State>();

        protected TOwner owner => stateMachine.owner;

        internal void Enter(State prevstate)
        {
            OnEnter(prevstate);
        }
        protected virtual void OnEnter(State prevstate) { }

        internal void Updata()
        {
            OnUpdata();
        }
        protected virtual void OnUpdata() { }

        internal void FixedUpdata()
        {
            FixedUpdate();
        }
        protected virtual void FixedUpdate() { }
        internal void Exit(State nextstate)
        {
            OnExit(nextstate);
        }
        protected virtual void OnExit(State nextstate) { }
    }
    public sealed class AnyState : State { }
    public TOwner owner { get; }
    public State CurrentState { get; private set; }

    /// <summary>
    /// 循環型リスト ぴんぽいんとの要素にアクセスする必要がないときだけ使う
    /// </summary>
    private LinkedList<State> stateslist = new LinkedList<State>();
    public StateMachine(TOwner My)//コンストラクタ
    {
        owner = My;
    }
    public T Add<T>() where T : State, new()
    {
        var state = new T();
        state.stateMachine = this;
        stateslist.AddLast(state);
        return state;
    }
    public void AddTransition<TFrom, TTo>(int eventId) where TFrom : State, new() where TTo : State, new()
    {
        var from = GetOrAddState<TFrom>();
        if (from.state.ContainsKey(eventId))
        {
            Debug.Log("できないよ");
            return;
        }
        var to = GetOrAddState<TTo>();
        from.state.Add(eventId, to);
    }
    public void AnyAddTrasition<TTo>(int eventId) where TTo : State, new()
    {
        AddTransition<AnyState, TTo>(eventId);
    }
    private T GetOrAddState<T>() where T : State, new()
    {
        foreach (var state in stateslist)
        {
            if (state is T resurt)
            {
                return resurt;
            }
        }
        return Add<T>();
    }

    public void Dispatch(int eventId)
    {
        State to;
        if (!CurrentState.state.TryGetValue(eventId, out to))
        {
            if (!GetOrAddState<AnyState>().state.TryGetValue(eventId, out to))
            {
                Debug.Log("Not Event");
                return;
            }
        }
        Change(to);
    }
    public void Start<Tfirst>() where Tfirst : State, new()
    {
        Starts(GetOrAddState<Tfirst>());
    }
    public void Starts(State first)
    {
        CurrentState = first;
        CurrentState.Enter(null);
    }
    public void Updata()
    {
        CurrentState.Updata();
    }
    public void FixedUpdates()
    {
        CurrentState.FixedUpdata();
    }
    private void Change(State nextstate)
    {
        CurrentState.Exit(nextstate);
        nextstate.Enter(CurrentState);
        CurrentState = nextstate;
    }
}
