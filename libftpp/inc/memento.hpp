#ifndef MEMENTO_HPP
#define MEMENTO_HPP

#include <iostream>
#include <string>
#include "data_structures.hpp"


class Memento{
	public:
		virtual ~Memento();
		using Snapshot = DataBuffer;
		Memento::Snapshot save();
		void load(const Memento::Snapshot& snapshot);
	private:
		virtual void _saveToSnapshot(Memento::Snapshot& snapshot)const = 0;
		virtual void _loadFromSnapshot(Memento::Snapshot& snapshot) = 0;
};


#endif
