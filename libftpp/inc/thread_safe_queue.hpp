#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include<mutex>
#include<deque>
#include<thread>

template<typename TType>
class ThreadSafeQueue{
	public:
		void push_back(const TType& newElement){
			std::unique_lock<std::mutex> lock(_mutex);

			_safe_queue.push_back(newElement);
		};
		
		void push_front(const TType& newElement){
			std::unique_lock<std::mutex> lock(_mutex);
			
			_safe_queue.push_front(newElement);
		};
		TType pop_back(){
			std::unique_lock<std::mutex> lock(_mutex);
			if (_safe_queue.empty())
				throw std::runtime_error("Empty queue");
				
			TType temp = _safe_queue.back();
			_safe_queue.pop_back();
			return temp;
		};
		TType pop_front(){
			std::unique_lock<std::mutex> lock(_mutex);
			if (_safe_queue.empty())
				throw std::runtime_error("Empty queue");
			TType temp = _safe_queue.front();
			_safe_queue.pop_front();
			return temp;
		};
		bool empty() const {
	        return (_safe_queue.empty());
	    }
	private:
		std::mutex _mutex;
		std::deque<TType> _safe_queue;
};

#endif
