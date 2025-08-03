#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <functional>

template<typename TState>
class StateMachine{
	public:
		void addState(const TState& state){
			
		};
		void addTransition(const TState& startState, const TState& finalState, const std::function<void()>& lambda){
			
		};
		void addAction(const TState& state, const std::function<void()>& lambda){
			
		}
		void transitionTo(const TState& state){
			
		};
		void update(){
			
		};
};

#endif
