#ifndef OPERTARGET_H
#define OPERTARGET_H
#include"Util.h"
#include"Target.h"
class MountLoad;
class Slot;
class OperTarget
{//编队组合中的目标类
private:
	int _id;
	static int _countId;

	Target* _target;								//分配目标
	vector<pair<Mount*, int>> _mountPlanList;		//分配给该目标的挂载类型/数量
	vector<pair<Load*, int>> _loadPlanList;		//分配给该目标的载荷类型/数量

	vector<pair<MountLoad*, int>> _mountLoadPlan;	//分配给该目标的（挂载+载荷）对类型/数量

	map< Slot*, map<MountLoad*, int>> _solnTimeMountMap;//不是在结果分配的，在预生成的时候就已经分配
	map< Slot*, map<MountLoad*, vector<int>>> _solnTimeAftMap;

public:
	OperTarget(Target* target, vector<pair<Mount*, int >> mountPlanList, vector<pair<Load*, int>> loadPlanList);//编队组合中的目标构造函数
	OperTarget(Target* target, vector<pair<MountLoad*, int>> mountLoadPlan);										//编队组合中的目标构造函数
	OperTarget(Target* target);																						//编队组合中的目标构造函数
	~OperTarget();
	//属性的设置和获得函数
	Target* getTarget() { return _target; }
	void setTarget(Target* target) { _target = target; }

	vector<pair<Mount*, int >> getMountPlanList() { return _mountPlanList; }
	void setMountPlanList(vector<pair<Mount*, int >> mountList) { _mountPlanList = mountList; }
	void pushMountPlan(pair<Mount*, int> mount) { _mountPlanList.push_back(mount); }

	vector<pair<Load*, int>> getLoadPlanList() { return _loadPlanList; }
	void setLoadPlanList(vector<pair<Load*, int >> loadList) { _loadPlanList = loadList; }
	void pushLoadPlan(pair<Load*, int> load) { _loadPlanList.push_back(load); }

	vector<pair<MountLoad*, int>> getMountLoadPlan() { return _mountLoadPlan; }
	void setMountLoadPlan(vector<pair<MountLoad*, int>> mountLoadPlan) { _mountLoadPlan = mountLoadPlan; }
	void pushMountLoadPlan(pair<MountLoad*, int> mountLoadPlan) { _mountLoadPlan.push_back(mountLoadPlan); }
	static bool cmpByOrder(OperTarget* ota, OperTarget* otb);

	static void initCountId() { _countId = 0; }

	map< Slot*, map<MountLoad*, int>> getSolnTimeMountMap() { return _solnTimeMountMap; }
	void pushSolnTimeMount(Slot* slot, MountLoad* mountLoad, int num);
	map< Slot*, map<MountLoad*, vector<int>>> getSolnTimeAftMap() { return _solnTimeAftMap; }
	void pushSolnTimeAft(Slot* slot, MountLoad* mountLoad, vector<int> numList);

	int getEarliestSlotInd();

	void clearSoln();

	void print();
};
#endif // OPERTARGET_H