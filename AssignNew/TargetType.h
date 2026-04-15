#pragma once
#ifndef TARGETTYPE_H
#define TARGETTYPE_H
#include "Util.h"

class Mount;
class Load;
class TargetPlan;
class Target;
class TargetType
{//目标类型类
private:
	static int _countId;
	int _id;
	string _name;
	vector<TargetPlan* > _targetPlanList;		//该目标类型的可选需求模式集合
	vector<Target*> _targetList;				//是该目标类型的目标集合
	int _totalNum;							//该目标类型的总数量	
public:
	TargetType(string name);					//目标类型构造函数
	~TargetType();							//目标类型解构函数

	//属性的设置和获得函数
	int getId() { return _id; }
	void setId(int id) { _id = id; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	int getTotalNum() { return _totalNum; }
	void addTotalNum() { _totalNum++; }
	void initTotalNum() { _totalNum = 1; }

	vector<TargetPlan* > getTargetPlanList() { return _targetPlanList; }
	void pushTargetPlan(TargetPlan* plan) { _targetPlanList.push_back(plan); }

	vector<Target*> getTargetList() { return _targetList; }
	void pushTarget(Target* target) { _targetList.push_back(target); }
	static void initCountId() { _countId = 0; }
	/*TargetPlan* getLongTimeConsumePlan();
	TargetPlan* getShortTimeConsumePlan();*/

	static bool sortPlanByLTC(pair<TargetPlan*, pair<bool, int>> A, pair<TargetPlan*, pair<bool, int>> B);

	void print();
};

#endif // TARGETTYPE_H