#include "Target.h"
#include "TargetType.h"
#include "MountType.h"
#include "Tour.h"
#include "Base.h"
//#include "Schedule.h"
int Target::_countId = 0;
Target::Target(string name, TargetType* type, int maxGrpNum, pair<time_t, time_t> timeRange, int priority, double longitude, double latitude)
	:_name(name), _type(type), _maxGrpNum(maxGrpNum),_timeRange(timeRange), _priority(priority), _longitude(longitude), _latitude(latitude)
{
	_id = _countId++;
	_lpVal = 0.0;
	_airforceCost = 0;
	_oilCost = 0;
	_mainTarget = NULL;
	_order = 0;
	_earliestCmpTime = 0;

}
Target::Target(string name, TargetType* type, int priority)
	:_name(name), _type(type), _priority(priority)
{
	_id = _countId++;
	_lpVal = 0.0;
	_airforceCost = 0;
	_oilCost = 0;
	_mainTarget = NULL;
	_order = 0;
	_earliestCmpTime = 0;
}
Target::~Target()
{
}

void Target::pushResourse(Tour* tour, vector<pair<MountLoad*, int>> mlList)
{
	//如果这个目标已经由这个机型分配过货物
	if (_asgResourse.find(tour->getFleet()) != _asgResourse.end())
	{
		pair<int, map<MountLoad*, int>> fleetSecond = _asgResourse[tour->getFleet()];
		fleetSecond.first += 1;//fleet经过的目标数量+1
		for (int i = 0; i < mlList.size(); i++)
		{
			//携带的货物数量更新
			if (fleetSecond.second.find(mlList[i].first) != fleetSecond.second.end())
			{
				fleetSecond.second[mlList[i].first] = fleetSecond.second[mlList[i].first] + mlList[i].second;
			}
			else
			{
				fleetSecond.second[mlList[i].first] = mlList[i].second;
			}
			_airforceCost += mlList[i].second * (mlList[i].first->_mount->getCostUse() + mlList[i].first->_load->getCostUse());
		}
		_asgResourse[tour->getFleet()] = fleetSecond;
		_airforceCost += tour->getFleet()->getCostUse();
	}
	//如果这个飞机还没有分配货物
	else
	{
		map<MountLoad*, int> mlMap;
		for (int i = 0; i < mlList.size(); i++)
		{
			if (mlMap.find(mlList[i].first) != mlMap.end())
			{
				mlMap[mlList[i].first] = mlMap[mlList[i].first] + mlList[i].second;
			}
			else
			{
				mlMap[mlList[i].first] = mlList[i].second;
			}
			_airforceCost += mlList[i].second * (mlList[i].first->_mount->getCostUse() + mlList[i].first->_load->getCostUse());
		}
		_asgResourse[tour->getFleet()] = make_pair(1, mlMap);//如果这个飞机还没有分配货物，就初始化经过的目标数量为1，同时更新资源分配数量
		_airforceCost += tour->getFleet()->getCostUse();
	}

	_oilCost += getDistance(tour->getBase()->getLongitude(), tour->getBase()->getLatitude(), this->getLongitude(), this->getLatitude()) * 2 / tour->getFleet()->getOptSpeed() * 60 * tour->getFleet()->getOilPerMin() * 1;
}


void Target::clearSoln()
{
	_asgResourse.clear();
	_airforceCost = 0;
	_solnTimeAftMap.clear();
	_solnTimeMountMap.clear();
	_oilCost = 0;
	_order = 0;
	_earliestCmpTime = 0;
	_lpVal = 0.0;
	_solnTargetPlan = NULL;
}

bool Target::cmpByPrio(Target* t1, Target* t2)
{
	return t1->getPriority() < t2->getPriority();
}



void Target::print()
{
	cout << "Target: id: " << _id << " name: " << _name << " type: " << _type->getName()
		<< " priority: " << _priority << " longitude: " << _longitude << " latitude: " << _latitude << " ifGather: "<< endl;
}

void Target::pushSolnTimeMount(Slot* slot, MountLoad* mountLoad, int num)
{
	if (_solnTimeMountMap.find(slot) != _solnTimeMountMap.end())
	{
		if (_solnTimeMountMap[slot].find(mountLoad) != _solnTimeMountMap[slot].end())
		{
			_solnTimeMountMap[slot][mountLoad] = _solnTimeMountMap[slot][mountLoad] + num;
		}
		else
		{
			_solnTimeMountMap[slot][mountLoad] = num;
		}
	}
	else
	{
		_solnTimeMountMap[slot][mountLoad] = num;
	}
}

void Target::pushSolnTimeAft(Slot* slot, MountLoad* mountLoad, vector<int> numList)
{
	if (_solnTimeAftMap.find(slot) != _solnTimeAftMap.end())
	{
		if (_solnTimeAftMap[slot].find(mountLoad) != _solnTimeAftMap[slot].end())
		{
			vector<int> pointList = _solnTimeAftMap[slot][mountLoad];
			vector<int> newPointList = pointList;
			for (int i = 0; i < numList.size(); i++)
			{
				bool ifFind = false;
				for (int j = 0; j < pointList.size(); j++)
				{
					if (numList[i] == pointList[j])
					{
						ifFind = true;
					}
				}
				if (!ifFind)
				{
					newPointList.push_back(numList[i]);
				}
			}
			_solnTimeAftMap[slot][mountLoad] = newPointList;
		}
		else
		{
			_solnTimeAftMap[slot][mountLoad] = numList;
		}
	}
	else
	{
		_solnTimeAftMap[slot][mountLoad] = numList;
	}
}

double Target::getPlanPrefCost(TargetPlan* plan)
{
	vector<TargetPlan*> planList = _type->getTargetPlanList();
	double minUseCost = 0;
	double thisUseCost = 0;
	for (int i = 0; i < planList.size(); i++)
	{
		int useCost = 0;
		map<MountLoad*, int> mountLoadPlan = planList[i]->_mountLoadPlan;
		map<MountLoad*, int>::iterator iter = mountLoadPlan.begin();
		for (iter; iter != mountLoadPlan.end(); iter++)
		{
			useCost += (*iter).first->_mount->getCostUse() * (*iter).second * Util::costUseWeight;
			useCost += (*iter).first->_load->getCostUse() * (*iter).second * Util::costUseWeight;
		}
		if (i == 0)
		{
			minUseCost = useCost;
		}
		else if (useCost < minUseCost)
		{
			minUseCost = useCost;
		}
		if (plan == planList[i])
		{
			thisUseCost = useCost;
		}
	}
	return thisUseCost - minUseCost;
}