#include "Slot.h"

int Slot::_countId = 0;
Slot::Slot(int cap, time_t stTime, time_t endTime)
	:_cap(cap), _stTime(stTime), _endTime(endTime)
{
	_id = _countId++;
	_remainCap = cap;
}

Slot::~Slot()
{
}

bool Slot::cmpByTime(pair<Slot*, int> pA, pair<Slot*, int> pB)
{
	return pA.first->getStTime() < pB.first->getStTime();
}

void Slot::deletePoint(int point)
{
	for (int i = 0; i < _pointList.size(); i++)
	{
		if (_pointList[i] == point)
		{
			_pointList.erase(_pointList.begin() + i);
			_cap--;
			break;
		}
	}
}

bool Slot::checkExist(vector<int> pointList)
{
	for (int i = 0; i < pointList.size(); i++)
	{
		bool ifFind = false;
		for (int j = 0; j < _pointList.size(); j++)
		{
			if (pointList[i] == _pointList[j])
			{
				ifFind = true;
			}
		}
		if (!ifFind)
		{
			return false;
		}
	}
	return true;
}