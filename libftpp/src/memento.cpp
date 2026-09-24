#include "memento.hpp"

Memento::Snapshot Memento::save() const{
	Memento::Snapshot snapshot;
	_saveToSnapshot(snapshot);
	return snapshot;
}


void Memento::load(const Memento::Snapshot& snapshot){
	Memento::Snapshot copy = snapshot;
	_loadFromSnapshot(copy);
}
