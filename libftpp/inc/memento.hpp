#ifndef MEMENTO_HPP
#define MEMENTO_HPP

#include <iostream>
#include <string>
#include "data_structures.hpp"


class Memento{
	private:
		DataBuffer objs;
	public:
		class Snapshot{
			private:
				DataBuffer objs;
				void _saveToSnapshot(Memento::Snapshot& snapshot);
				void _loadFromSnapshot(Memento::Snapshot& snapshot);
		};
	friend void _saveToSnapshot(Memento::Snapshot& snapshot);
	friend void _loadFromSnapshot(Memento::Snapshot& snapshot);
	Snapshot save();
	void load(const Memento::Snapshot& snapshot);
};


#endif
