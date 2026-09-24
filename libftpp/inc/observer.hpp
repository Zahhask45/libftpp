#ifndef OBSERVER_HPP
#define OBSERVER_HPP


#include <functional>
#include <unordered_map>
#include <vector>

template<typename TEvent>
class Observer{
	public:
		void subscribe(const TEvent& event, const std::function<void()>& callback){
			_events[event].push_back(callback);
		};
		void notify(const TEvent& event){
			// this auto is new in this version of code
			auto it = _events.find(event);
			if (it == _events.end())
				return ;

			// for received update since i use the iterator
			for (auto& callback : it->second)
				callback();
		};
	private:
		std::unordered_map<TEvent, std::vector<std::function<void()>>> _events;
};


#endif
