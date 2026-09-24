#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

template<typename TState>
class StateMachine{
	public:
		struct TransitionKey{
			TState from;
			TState to;

			bool operator==(const TransitionKey &rhs) const{
				return from == rhs.from && to == rhs.to;
			}
		};

		
		struct TransitionKeyHash {
			std::size_t operator()(const TransitionKey& key) const{
				std::size_t h1= std::hash<TState>{}(key.from);
				std::size_t h2= std::hash<TState>{}(key.to);
				return h1 ^ (h2 << 1);
			}	
		};


		void addState(const TState& state){
			_stateActions[state];
			if (!_hasCurrentState){
				_currentState = state, _hasCurrentState = true;
			}
			
		};
		
		void addAction(const TState& state, const std::function<void()>& lambda){
			_stateActions[state].push_back(&lambda);
		};
		
		void addTransition(const TState& startState, const TState& finalState, const std::function<void()>& lambda){
			_transitions[{startState,finalState}].push_back(&lambda);
		};
		
		void transitionTo(const TState& state){
			if (!_hasCurrentState)
				throw std::runtime_error("StateMachine: no current state");

			auto it = _transitions.find({ _currentState, state });
			if (it == _transitions.end())
				throw std::invalid_argument("StateMachine: invalid transition");
			
			_currentState = state;
			for (const auto& callback : it->second)
				if (callback) 
					callback();	
		};
		
		void update(){
			if (!_hasCurrentState)
				throw std::runtime_error("StateMachine: current state not set");

			auto it = _stateActions.find(_currentState);
			if (it == _stateActions.end())
				throw std::runtime_error("StateMachine: state not found");
			for (const auto& callback : it->second)
				if (callback)
					callback();
		};
		// 
		// StateMachine(){
		// 	_currentState = nullptr;
		// };
		// ~StateMachine(){
		// 	delete _curstate;
		// }
	private:
		TState _currentState{};
		bool _hasCurrentState = false;
		std::unordered_map<TState, std::vector<const std::function<void()>>> _stateActions;
		std::unordered_map<TransitionKey, std::vector<std::function<void()>>, TransitionKeyHash> _transitions;

};

#endif
