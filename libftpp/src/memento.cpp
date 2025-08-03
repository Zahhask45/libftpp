#include "memento.hpp"

Memento::Snapshot Memento::save(){
	Memento::Snapshot snap;
	_saveToSnapshot(snap);
	return snap;
}


void Memento::load(const Memento::Snapshot& snapshot){
	_loadFromSnapshot(const_cast<Memento::Snapshot&>(snapshot));
}

Memento::~Memento(){};
