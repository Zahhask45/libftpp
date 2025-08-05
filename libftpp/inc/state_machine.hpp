#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <functional>
#include <utility>
#include <unordered_map>
#include <vector>
#include <sstream>



template<typename TState>
class StateMachine{
	public:
		template<typename T, typename U>
		struct TransitionKey{
			T start;
			U end;

			bool operator==(const TransitionKey &rhs) const{
				return (this->start == rhs.start && this->end == rhs.nd);
			}
		};
		template<typename T, typename U>
		struct stateHash {
			std::size_t operator()(const TransitionKey<T, U>& t) const{
				return std::hash<T>()(t.start);
			}	
		};


	
		void addState(const TState& state){
			if (!_curstate){
				_curstate = new TState(state);
			}
			_states[state] = state;
			
		};
		void addAction(const TState& state, const std::function<void()>& lambda){
			_states[state].push_back(&lambda);
		};
		void addTransition(const TState& startState, const TState& finalState, const std::function<void()>& lambda){
			_transitions[{startState,finalState}].push_back(&lambda);
		};
		void transitionTo(const TState& state){
			// {
			// 	auto it = _states.find(state);
			// 	if (it == _states.end()){
			// 		throw std::invalid_argument("Wrong");
			// 	}
			// }
			// if (*_curstate == state){
			// 	this->update();
			// }
			auto it = _transitions.find({*_curstate, state});
			if (it == _transitions.end())throw std::invalid_argument("Invalid");
			delete _curstate;
			_curstate = new TState(state);
			for (auto lambda : it->second)
				(*lambda)(); 
			
		};
		void update(){
			if (!_curstate) throw std::runtime_error("empty");
			for (auto lambda : _states[_curstate])
				(*lambda)();
		};
		StateMachine(){
			_curstate = nullptr;
		};
	private:
		std::unordered_map<TState, std::vector<const std::function<void()>*>> _states;
		std::unordered_map<TransitionKey<TState, TState>, std::vector<const std::function<void()>*>, stateHash<TState, TState>> _transitions;
		TState *_curstate;

};

#endif
