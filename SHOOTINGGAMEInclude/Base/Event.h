#pragma once
#include <functional>
#include <vector>

//一人が登録したイベントを設定したら登録された全員に通知する
template<typename ...Args>
class Event
{
public:
	void Subscribe(std::function<void(Args...)>args) { _handlers.push_back(std::move(args)); }

	//登録されているイベントをすべて呼ぶ
	void Invoke(Args... args) const
	{
		for (const auto& handler : _handlers)
			handler(args...);
	}
private:
	std::vector<std::function<void(Args...)>>_handlers;
};