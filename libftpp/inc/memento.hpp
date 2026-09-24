#ifndef MEMENTO_HPP
#define MEMENTO_HPP

#include "data_structures.hpp"


class Memento{
	public:
		using Snapshot = DataBuffer;
		
		virtual ~Memento() = default;
		
		Snapshot save() const;
		void load(const Snapshot& snapshot);
	private:
		virtual void _saveToSnapshot(Snapshot& snapshot)const = 0;
		virtual void _loadFromSnapshot(Snapshot& snapshot) = 0;
};


#endif
