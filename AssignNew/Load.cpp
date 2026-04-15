#include "Load.h"

int Load::_countId = 0;
Load::Load(string name, double costUse)
	: _name(name), _costUse(costUse)
{
	_id = _countId++;
	_totalNum = 0;
}

Load::~Load()
{
}

void Load::print()
{
	cout << "Load: id: " << _id << " name: " << _name << " costUse: " << _costUse << endl;
}