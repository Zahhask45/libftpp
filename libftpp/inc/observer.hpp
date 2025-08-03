#ifndef OBSERVER_HPP
#define OBSERVER_HPP


#include <functional>
#include <unordered_map>
#include <vector>

template<typename TEvent>
class Observer{
	public:
		void subscribe(const TEvent& event, const std::function<void()>& lambda){
			events[event].push_back(&lambda);
		};
		void notify(const TEvent& event){
			for (auto lambda : events[event])
				(*lambda)();
		};
	private:
		std::unordered_map<TEvent, std::vector<const std::function<void()>*>> events;
};


#endif
