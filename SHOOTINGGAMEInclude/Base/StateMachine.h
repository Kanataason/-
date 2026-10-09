#pragma once
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

//イベントIDで状態を切り替えるステートマシン
//TOwnerは持ち主のクラスで、各Stateからowner()で触れる
template<class TOwner>
class StateMachine {
public:
    //すべての状態の基底クラス
    //ステートマシン本体の内部クラスにしているので、TOwnerごとに別の型になる
    class State {
    public:
        virtual ~State() = default;

    protected:
        //継承先のStateから使える
        //自分を持っているステートマシン。Dispatchで状態を切り替えるときに使う
        StateMachine<TOwner>& stateMachine() {
            return *_stateMachine;
        }

        //ステートマシンの持ち主
        TOwner& owner() {
            return _stateMachine->owner();
        }

        //継承先で必要なものだけ override する
        //この状態に入ったとき1回。previousStateは直前の状態で、Start直後はnullptr
        virtual void OnEnter(State* previousState) {}
        //この状態の間、毎フレーム
        virtual void OnUpdate(float deltaTime) {}
        //今のプロジェクトでは未使用
        virtual void OnFixedUpdate(float deltaTime) {}
        //この状態から出るとき1回。nextStateは次の状態
        virtual void OnExit(State* nextState) {}
        //この状態の間、毎フレームの描画
        virtual void OnDraw(){}

    private:
        //ステートマシン本体からは、privateな_transitionsやEnterなどに触れるようにする
        friend class StateMachine<TOwner>;

        //Addで生成したときにステートマシンが自分を入れる
        StateMachine<TOwner>* _stateMachine = nullptr;

        //イベントID → 移る先の状態。この状態にいるときだけ有効な遷移
        std::unordered_map<int, State*> _transitions;
        //移る先の状態 → イベントID。逆引き用だが、今は使っていない
        std::unordered_map<State*, int> _reverseTransitions;

        //StateMachineだけが呼べる
        //外から直接OnEnterなどを呼ばせないために、privateな入口を一段はさんでいる
        void Enter(State* previousState) {
            OnEnter(previousState);
        }

        void Update(float deltaTime) {
            OnUpdate(deltaTime);
        }

        void FixedUpdate(float deltaTime) {
            OnFixedUpdate(deltaTime);
        }

        void Exit(State* nextState) {
            OnExit(nextState);
        }
        void Draw()
        {
            OnDraw();
        }
    };

    //「どの状態からでも」を表す特別な状態
    //実際にこの状態になることはなく、遷移の登録先としてだけ使う
    //Dispatchは、今の状態に遷移が無いときにAnyStateの遷移を探す
    class AnyState final : public State {};

    explicit StateMachine(TOwner& owner)
        : _owner(&owner) {
    }

    TOwner& owner() {
        return *_owner;
    }

    const TOwner& owner() const {
        return *_owner;
    }

    //今の状態。Start前はnullptr
    State* currentState() const {
        return _currentState;
    }

    //最後に状態を切り替えたイベントID。今は使っていない
    int nextStateEventId() const {
        return _nextStateEventId;
    }

    //状態を生成して登録する
    //同じ型を2回呼ぶと2個できるので、普段はAddTransition経由のGetOrAddStateを使う
    template<class TState>
    TState& Add() {
        //Stateを継承していない型を渡したらコンパイルエラーにする
        static_assert(std::is_base_of_v<State, TState>);

        auto state = std::make_unique<TState>();
        state->_stateMachine = this;

        //moveするとstateは空になるので、先に参照を取っておく
        TState& result = *state;
        _statesList.push_back(std::move(state));
        return result;
    }

    //TFromの状態でeventIdが来たら、TToへ移る遷移を登録する
    //状態がまだ無ければ、ここで自動的に生成される
    template<class TFrom, class TTo>
    void AddTransition(int eventId) {
        TFrom& from = GetOrAddState<TFrom>();

        //同じ状態に同じイベントは1つだけ。2回目は無視する
        if (from._transitions.contains(eventId)) {
            std::cerr << "すでに登録されています。\n";
            return;
        }

        TTo& to = GetOrAddState<TTo>();
        from._transitions.emplace(eventId, &to);
        from._reverseTransitions.emplace(&to, eventId);
    }

    //どの状態にいても、eventIdが来たらTToへ移る遷移を登録する
    //死亡のように、いつ起きてもおかしくない遷移に使う
    template<class TTo>
    void AnyAddTransition(int eventId) {
        AddTransition<AnyState, TTo>(eventId);
    }

    //最初の状態に入る。遷移の登録が終わってから呼ぶ
    template<class TFirst>
    void Start() {
        Start(GetOrAddState<TFirst>());
    }

    void Start(State& firstState) {
        _currentState = &firstState;
        _currentState->Enter(nullptr);
    }

    //イベントを送り、登録された遷移があれば状態を切り替える
    //どちらにも無ければ、エラーを出力して何もしない
    void Dispatch(int eventId) {
        if (_currentState == nullptr) {
            throw std::logic_error("StateMachine is not started.");
        }

        State* nextState = nullptr;

        //今の状態に登録された遷移を優先する
        auto current = _currentState->_transitions.find(eventId);
        if (current != _currentState->_transitions.end()) {
            nextState = current->second;
        }
        else {
            //無ければ、どの状態からでも有効な遷移を探す
            AnyState& any = GetOrAddState<AnyState>();
            auto anyTransition = any._transitions.find(eventId);

            if (anyTransition == any._transitions.end()) {
                std::cerr << "Current state has no event: "
                    << eventId << '\n';
                return;
            }

            nextState = anyTransition->second;
        }

        _nextStateEventId = eventId;
        Change(*nextState);
    }

    //今の状態のOnUpdateを呼ぶ
    void Update(float deltaTime) {
        if (_currentState != nullptr) {
            _currentState->Update(deltaTime);
        }
    }

    //今の状態のOnDrawを呼ぶ
    void Draw() {
        if (_currentState != nullptr) {
            _currentState->Draw();
        }
    }

    void FixedUpdate(float deltaTime) {
        if (_currentState != nullptr) {
            _currentState->FixedUpdate(deltaTime);
        }
    }

private:
    //TStateの状態がすでにあればそれを返し、無ければ生成する
    //同じ型の状態を1つだけにするための関数
    template<class TState>
    TState& GetOrAddState() {
        static_assert(std::is_base_of_v<State, TState>);

        //型で探す。dynamic_castは、中身がTStateかその派生クラスのときだけnullptr以外を返す
        //そのため、状態同士で継承関係があると、派生側の状態が見つかってしまう点に注意
        for (const auto& state : _statesList) {
            if (auto* result = dynamic_cast<TState*>(state.get())) {
                return *result;
            }
        }

        return Add<TState>();
    }

    //今の状態を抜けて、次の状態に入る
    void Change(State& nextState)
    {
        _currentState->Exit(&nextState);
        nextState.Enter(_currentState);
        _currentState = &nextState;
    }

    TOwner* _owner;
    State* _currentState = nullptr; //_statesListが所有するため非所有ポインタ
    int _nextStateEventId = -1;

    //すべての状態の実体を持つ
    //_transitionsや_currentStateは状態の生ポインタを持っているので、状態の住所が変わらないことが大事
    std::list<std::unique_ptr<State>> _statesList;
};
