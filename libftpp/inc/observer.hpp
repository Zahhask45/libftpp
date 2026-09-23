#ifndef OBSERVER_HPP
#define OBSERVER_HPP


#include <functional>
#include <unordered_map>
#include <vector>

template<typename TEvent>
class Observer{
	public:
		void subscribe(const TEvent& event, const std::function<void()>& lambda){
			events[event].push_back(lambda);
		};
		void notify(const TEvent& event){
			// this auto is new in this version of code
			auto it = event.find(event);
			if (it == event.end())
				return ;

			// for received update since i use the iterator
			for (auto& callback : it->second)
				callback();
		};
	private:
		std::unordered_map<TEvent, std::vector<const std::function<void()>>> events;
};


#endif
