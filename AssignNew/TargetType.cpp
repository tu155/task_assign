#include "TargetType.h"
#include "Mount.h"
#include "Load.h"
#include "Schedule.h"
int TargetType::_countId = 0;

TargetType::TargetType(string name)
	:_name(name)
{
	_id = _countId++;
}

TargetType::~TargetType()
{
}

void TargetType::print()
{
	cout << "TargetType: id: " << _id << " name: " << _name << " TotalNum :" << _totalNum << endl;
	for (int i = 0; i < _targetPlanList.size(); i++)
	{
		map<MountLoad*, int>::iterator  iter = (_targetPlanList[i]->_mountLoadPlan).begin();
		cout << "-plan " << i;
		for (iter; iter != (_targetPlanList[i]->_mountLoadPlan).end(); iter++)
		{
			string dmdName = ((*iter).first->_mount->getName()) + ";" + ((*iter).first->_load->getName());
			cout << " " << dmdName << " - " << (*iter).second << " ";
		}
		cout << endl;
	}
}

//TargetPlan* TargetType::getLongTimeConsumePlan()
//{
//	vector<pair<TargetPlan*, pair<bool, int>>> planList;
//	for (int i = 0; i < _targetPlanList.size(); i++)
//	{
//		TargetPlan* plan = _targetPlanList[i];
//		map<MountLoad*, int> mountLoadPlan = plan->_mountLoadPlan;
//		int totalWeaponNum = 0;
//		bool ifContainOccupy = false;
//		map<MountLoad*, int>::iterator iter = mountLoadPlan.begin();
//		for (iter; iter != mountLoadPlan.end(); iter++)
//		{
//			totalWeaponNum += (*iter).second;
//			if ((*iter).first->_mount->isOccupy())
//			{
//				ifContainOccupy = true;
//			}
//		}
//		planList.push_back(make_pair(plan, make_pair(ifContainOccupy, totalWeaponNum)));
//	}
//	sort(planList.begin(), planList.end(), TargetType::sortPlanByLTC);
//	return planList[0].first;
//}
//
//TargetPlan* TargetType::getShortTimeConsumePlan()
//{
//	//vector<pair<ZZTargetPlan*, pair<bool, int>>> planList;
//	//for (int i = 0; i < _targetPlanList.size(); i++)
//	//{
//	//	ZZTargetPlan* plan = _targetPlanList[i];
//	//	map<ZZMountLoad*, int> mountLoadPlan = plan->_mountLoadPlan;
//	//	int totalWeaponNum = 0;
//	//	bool ifTotalNoOccupy = false;
//	//	map<ZZMountLoad*, int>::iterator iter = mountLoadPlan.begin();
//	//	for (iter; iter != mountLoadPlan.end(); iter++)
//	//	{
//	//		totalWeaponNum += (*iter).second;
//	//		if ((*iter).first->_mount->isOccupy())
//	//		{
//	//			ifContainOccupy = true;
//	//		}
//	//	}
//	//	planList.push_back(make_pair(plan, make_pair(ifContainOccupy, totalWeaponNum)));
//	//}
//	//sort(planList.begin(), planList.end(), ZZTargetType::sortPlanByLTC);
//	//return planList[0].first;
//	return NULL;
//}

bool TargetType::sortPlanByLTC(pair<TargetPlan*, pair<bool, int>> A, pair<TargetPlan*, pair<bool, int>> B)
{
	if (A.second.first > B.second.first)
	{
		return true;
	}
	else
	{
		if (A.second.second > B.second.second)
		{
			return true;
		}
	}
	return false;
}