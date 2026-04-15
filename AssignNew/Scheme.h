#ifndef SCHEME_H
#define SCHEME_H

#include "Util.h"
#include "Tour.h"
class Target;
class Fleet;
class TargetPlan;
class Mount;
class Load;
class Schedule;
class Scheme
{//不同类型编队备选组合的生成类
private:

	Schedule* _schedule;
	Fleet* _fleet;																				//机型
	int _aftNum;																					//数量

	vector<TargetType* > _targetTypeList;															//目标类型
	set<string> _targetDmd;																			//目标中出现过的 挂载-载荷（分号隔开）
	vector<Target* > _targetList;																	//该类型编队可分配目标集合
	set<Target* > _targetSet;																		//该类型编队可分配目标集合（set形式）

	vector<vector<int>> _mountPlanList;																//该编队类型的挂载类型集合
	vector<map<Mount*, int>> _mountMergeClassPlanList;											//该编队类型同类挂载数量上整合后的挂载类型方案集合
	vector<map<Mount*, vector<vector<Load* >>>> _loadMergePlanClassList;						//该编队类型不同挂载方案的载荷分配方案集合
	vector<map<string, int>> _totalLoadPlan;														//该编队类型的装载方案集合（挂载—载荷分配方案，不涉及目标）

	vector<vector<pair<TargetType*, map<string, int>>>>	_targetTypePlanList;					//装载方案——目标类型的分配方案集合
	vector<map<string, int>> _remainMountLoadList;													//装载方案——目标类型的分配方案多余装载情况集合
	vector<vector<TargetType* >> _tpComList;														//可分配目标类型方案的集合（只包含类型）
	vector<vector<Target*>> _targetListList;														//可分配目标方案的集合（只包含目标）

	
	vector<vector<pair<Target*, map<string, int>>>> _targetPlanList;								//该编队类型到目标的分配方案集合（常规分配逻辑）
	vector<int> _targetTPIndexList;																	//该编队类型到目标的分配方案对应的到目标类型的方案的序号集合

	vector<Tour*> _tourList;																		//该编队类型到目标的tour形式的分配方案集合
	vector<vector<int>> _exploSeqList;																//每个tour成爆时刻集合的集合
	static void _split(string& str, vector<string>& subStrs, const string& c)
	{
		int count = 0;
		for (int i = 0; i < str.size(); i++)
		{
			if (str[i] == '"')
			{
				count++;
			}
			if (str[i] == ',' && count % 2 != 0)
			{
				str[i] = ';';
			}
		}

		subStrs.clear();
		string::size_type pos1, pos2, pos3;
		pos2 = str.find(c);
		pos1 = 0;
		while (string::npos != pos2)
		{
			pos3 = pos2;
			subStrs.push_back(str.substr(pos1, pos2 - pos1));
			pos1 = pos2 + c.size();
			pos2 = str.find(c, pos1);
		}
		if (pos1 != str.length() || pos3 == str.length() - c.size())
		{
			subStrs.push_back(str.substr(pos1));
		}

		count = 0;
		for (int i = 0; i < subStrs.size(); i++)
		{
			for (int j = 0; j < subStrs[i].size(); j++)
			{
				if (subStrs[i][j] == '"')
				{
					count++;
				}
				if (subStrs[i][j] == ',' && count % 2 != 0)
				{
					subStrs[i][j] = ',';
				}
			}
		}
	}

public:
	Scheme(Fleet* fleet, vector<TargetType* > targetTypeList, Schedule* schedule);	//构造函数
	~Scheme();																															//解构函数

	//属性的设置和获得函数
	vector<TargetType* > getTargetTypeList() { return _targetTypeList; }
	void setTargetTypeList(vector<TargetType* > targetTypeList) { _targetTypeList = targetTypeList; }

	Fleet* getFleet() { return _fleet; }
	void setFleet(Fleet* fleet) { _fleet = fleet; }

	int getAftNum() { return _aftNum; }
	void setAftNum(int aftNum) { _aftNum = aftNum; }

	vector<Tour*> getTourList() { return _tourList; }
	void pushTour(Tour* tour) { _tourList.push_back(tour); }
	void clearTourList() { _tourList.clear(); }

	//生成分配方案
	//逻辑类函数
	void generateTotalLoadPlan();						//生成该编队类型的装载方案集合（挂载—载荷分配方案，不涉及目标）
	void generateTargetTypePlan();						//以装满为基本逻辑去生成 装载方案——目标类型的分配方案集合
	void generateTargetTypePlan(int maxTargetNum);		//以最大分配目标数为主要逻辑去生成 装载方案——目标类型的分配方案集合

	void generateTotalTourByTP();						//根据任务点生成该编队类型到目标的分配方案集合

	void generateAftAssignTotalTour();							//生成该编队类型到目标的分配方案集合（常规）
	void geneTargetList();				//以编队分配目标之间的距离限制为逻辑生成可由一个编队执行作业的目标集合

	static bool cmp(pair<TargetType*, map<string, int>> pair1, pair<TargetType*, map<string, int>> pair2);
	bool checkSameSet(vector<TargetType*> list1, vector<TargetType*> list2);	//检查目标类型集合是否重复

	//递归函数
	void findMount(int num, int mountInd, vector<int> mountList);
	void findLoad(int i, Mount* mount, int num, int mountInd, vector<Load*> loadList);
	void findLoadPlan(int i, int j, map<string, int> loadPlan);
	void findTP(vector<TargetType*> tpList, int num, int numTP);
	void findTTPlan(vector<TargetType*> tpList, map<string, int> loadPlan, vector<pair<TargetType*, map<string, int>>> subTPPlanList, int numSubTP, int tpInd);
	void findTargetList(vector<Target*> tList, vector<TargetType*> ttList, int ttInd, double dst, vector<pair<TargetType*, int>> ttIndMap);
	void findExploSeq(int aftNum, vector<int> exploSeq, int preInd);

	void geneOperTargetList(vector<pair<Target*, map<string, int>>> operTargetList, int typePlanIndex, set<Target* > targetSet, int planNum, int targetNum, int num, int targetTypeNum);//根据任务点
	void geneOperTargetList(vector<pair<Target*, map<string, int>>> operTargetList, vector<pair<pair<TargetType*, map<string, int>>, int>> targetTypePlan, set<Target*> tSet, vector<Target*> targetList, int ttInd, int remainNum, int fixedTtInd, int preTInd);	//常规

	//生成tour的频点方案
	//void  generateTimeAssignTotalTour(vector<Slot*> slotList);
	//vector<map<Slot*, int>> generateTimeAssignSchds(Tour* tour, vector<Slot*> slotList);

	//预分配（上界）
	//void generateTimeAssignTotalTourPreUb(vector<Slot*> slotList);
	//vector<map<Slot*, int>> generateTimeAssignSchdsPreUb(Tour* tour, vector<Slot*> slotList);
};

#endif // !ZZSCHEME_H
