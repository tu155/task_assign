#include "Base.h"
//#include "Tour.h"
int Base::_countId = 0;
Base::Base(string name, double longitude, double latitude, time_t depTimeInterval)
	:_name(name), _longitude(longitude), _latitude(latitude), _depTimeInterval(depTimeInterval)
{
	_id = _countId++;
}

Base::~Base()
{
}

int Base::getMountNum(Mount* mount)
{
	if (_mountList.find(mount) != _mountList.end())
	{
		return _mountList[mount];
	}
	return 0;
}

int Base::getLoadNum(Load* load)
{
	if (_loadList.find(load) != _loadList.end())
	{
		return _loadList[load];
	}
	return 0;
}

int Base::getFleetNum(Fleet* fleet)
{
	if (_fleetList.find(fleet) != _fleetList.end())
	{
		return _fleetList[fleet];
	}
	return 0;
}

int Base::getFleetTypeAftNum(FleetType* fleetType)
{
	if (_fleetTypeList.find(fleetType) != _fleetTypeList.end())
	{
		return _fleetTypeList[fleetType];
	}
	return 0;
}

void Base::computeFleetTypeAftNum()
{
	map<Fleet*, int>::iterator iter = _fleetList.begin();
	for (iter; iter != _fleetList.end(); iter++)
	{
		pushFleetTypeAftNum((*iter).first->getType(), (*iter).second);
	}
}

//void ZZBase::genGroup()
//{
//	map<ZZFleet*, int >::iterator iter;
//	int count = 0;
//	for (iter = _fleetList.begin(); iter != _fleetList.end(); iter++)
//	{
//		cout << _fleetList.size() << endl;
//		int grpNum = (iter->second) / (*iter).first->getGrpMaxAftNum();
//		for (int i = 0; i < grpNum; i++)
//		{
//			ZZGroup* group = new ZZGroup("G" + to_string(count) + "B" + to_string(_id), this, iter->first, (*iter).first->getGrpMaxAftNum());
//			group->print();
//			count++;
//			pushGroup(group);
//		}
//	}
//}

void Base::pushTargetPre(Target* target, double cost)
{
	_targetPreferList[target] = cost;
}

double Base::getTargetPre(Target* target)
{
	if (_targetPreferList.find(target) != _targetPreferList.end())
	{
		return _targetPreferList[target];
	}
	return 0.0;
}

void Base::pushUsedFleetMap(Fleet* fleet, int num)
{
	if (_usedFleetMap.find(fleet) != _usedFleetMap.end())
	{
		_usedFleetMap[fleet] = _usedFleetMap[fleet] + num;
	}
	else
	{
		_usedFleetMap[fleet] = num;
	}
}

void Base::pushUsedMountMap(Mount* mount, int num)
{
	if (_usedMountMap.find(mount) != _usedMountMap.end())
	{
		_usedMountMap[mount] = _usedMountMap[mount] + num;
	}
	else
	{
		_usedMountMap[mount] = num;
	}
}

void Base::pushUsedLoadMap(Load* load, int num)
{
	if (_usedLoadMap.find(load) != _usedLoadMap.end())
	{
		_usedLoadMap[load] = _usedLoadMap[load] + num;
	}
	else
	{
		_usedLoadMap[load] = num;
	}
}

int Base::getUsedFleetNum(Fleet* fleet)
{
	if (_usedFleetMap.find(fleet) != _usedFleetMap.end())
	{
		return _usedFleetMap[fleet];
	}
	return 0;
}

int Base::getUsedMountNum(Mount* mount)
{
	if (_usedMountMap.find(mount) != _usedMountMap.end())
	{
		return _usedMountMap[mount];
	}
	return 0;
}

int Base::getUsedLoadNum(Load* load)
{
	if (_usedLoadMap.find(load) != _usedLoadMap.end())
	{
		return _usedLoadMap[load];
	}
	return 0;
}

void Base::clearSoln()
{
	_usedFleetMap.clear();
	_usedMountMap.clear();
	_usedLoadMap.clear();
}

void Base::print()
{
	cout << " name: " << _name << " longitude: " << _longitude << " latitude: " << _latitude << " depTimeInter: " << _depTimeInterval << endl;
	map <Load*, int>::iterator iter;
	map<Mount*, int>::iterator iter1;
	map<Fleet*, int>::iterator iter2;
	for (iter2 = _fleetList.begin(); iter2 != _fleetList.end(); iter2++)
	{
		cout << iter2->first->getName() << ":" << iter2->second << ";";
	}
	for (iter1 = _mountList.begin(); iter1 != _mountList.end(); iter1++)
	{
		cout << iter1->first->getName() << ":" << iter1->second << ";";
	}
	for (iter = _loadList.begin(); iter != _loadList.end(); iter++)
	{
		cout << iter->first->getName() << ":" << iter->second << ";";
	}
	cout << endl;
}