#ifndef THREAD_queue_HPP
#define THREAD_queue_HPP

#include<mutex>
#include<deque>
#include<stdexcept>

template<typename TType>
class ThreadSafeQueue{
	public:
		void push_back(const TType& newElement){
			std::lock_guard<std::mutex> lock(_mutex);

			_queue.push_back(newElement);
		};
		
		void push_front(const TType& newElement){
			std::lock_guard<std::mutex> lock(_mutex);
			
			_queue.push_front(newElement);
		};
		TType pop_back(){
			std::lock_guard<std::mutex> lock(_mutex);
			if (_queue.empty())
				throw std::runtime_error("ThreadSafeQueue: empty queue");
				
			TType value = _queue.back();
			_queue.pop_back();
			return value;
		};
		TType pop_front(){
			std::lock_guard<std::mutex> lock(_mutex);
			if (_queue.empty())
				throw std::runtime_error("ThreadSafeQueue: empty queue");
				
			TType value = _queue.front();
			_queue.pop_front();
			return value;
		};
		bool empty() const {
			std::lock_guard<std::mutex> lock(_mutex);
	        return _queue.empty();
	    }
	private:
		mutable std::mutex _mutex;
		std::deque<TType> _queue;
};

#endif
