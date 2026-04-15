#pragma once
#include "Util.h"
#include "OperTarget.h"

#include "Fleet.h"
#include "Base.h"
#include "tspSolver.h"
#include "MountType.h"
#include "Slot.h"
#include "MainTarget.h"

class Tour
{//分配方案/轮次类
private:
	static int _countId;
	int _id;
	int _roundId;
	double _cost;								//成本
	double _totalTime;								//总时间
	map<Target*, double> _targetTimeMap;				//方案包含的打击目标的访问时间
	double _totalOilCost;
	double _totalCost;
	double _lpVal;								//对应轮次覆盖变量的值
	double _dual;								//对应覆盖约束的对偶值	
	string _name;								//名称
	map<MountLoad*, int>  _remainMLList;		//可以再加的载荷配载对
	TourType _tourType;

	Base* _base;									//基地
	Fleet* _fleet;									//方案所属运力
	vector<OperTarget* > _operTargetList;	//方案包含的打击目标集合
	map<Slot*, int> _timeMap;					//频点集合<时刻，占用频点数>
	time_t _earliestExploTime;					//最早的成爆时刻
	map<Slot*, vector<int>> _slotPointMap;	//占用的某频点具体位置
	map<Load*, int> _loadMap;				//方案的载荷总量（类型/数量）
	map<Mount*, int> _mountMap;				//方案的挂载总量（类型/数量）
	map<MountLoad*, int>  _mountLoadMap;		//方案的挂载-载荷 总量（类型/数量）

	bool _fixed;								//这个tour是否被选中，选中为true，未选中为false
	int _value;									//被fix的value
	bool _isVisit;								//是否被访问过
	time_t _prevActStTime;						//前一轮结果确定的实际起飞时间
	time_t _prevActEndTime;						//前一轮结果确定的实际落地时间
	time_t _startTime;							//该编队的最早可出发时间

	//for topo
	int _order;

public:
	Tour( vector<OperTarget*> targetList);				//方案/组合 构造函数
	Tour(Fleet* fleet, vector<OperTarget*> targetList);
	~Tour();																//解构函数

	//属性的设置和获得函数
	void setId(int id) { _id = id; }
	int getId() { return _id; }
	
	void setBase(Base* base) { _base = base; }

	vector<OperTarget* > getOperTargetList() { return _operTargetList; }
	void setOperTargetList(vector<OperTarget*> operTargetList) { _operTargetList = operTargetList; }
	void pushOperTarget(OperTarget* operTarget) { _operTargetList.push_back(operTarget); }
	OperTarget* getOperTarget(string targetName);

	double  getCost() { return _cost; }
	void setCost(double cost) { _cost = cost; }

    double getTotalTime() { return _totalTime; }
    void setTotalTime(double time) { _totalTime = time; }

	map<Load*, int> getLoadMap() { return _loadMap; };
	int getLoadNum(Load* load);

	map<Mount*, int> getMountMap() { return _mountMap; }
	int getMountNum(Mount* mount);

	int getRoundId() { return _roundId; }
	void setRoundId(int id) { _roundId = id; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	double getLpVal() { return _lpVal; }
	void setLpVal(double val) { _lpVal = val; }

	double getDual() { return _dual; }
	void setDual(double dual) { _dual = dual; }

	void setFixed(bool flag) { _fixed = flag; }
	bool isFixed() { return _fixed; }

	double getTotalOilCost() { return _totalOilCost; }
	double getTotalCost() { return _totalCost; }

	time_t getPrevActStTime() { return _prevActStTime; }
	void setPrevActStTime(time_t stTime) { _prevActStTime = stTime; }

	time_t getPrevActEndTime() { return _prevActEndTime; }
	void setPrevActEndTime(time_t endTime) { _prevActEndTime = endTime; }

	time_t getStartTime() { return _startTime; }
	void setStartTime(time_t startTime) { _startTime = startTime; }

	map<MountLoad*, int>   getRemainMLList() { return _remainMLList; }
	void pushRemainML(MountLoad* mountLoad, int num);
	void setRemainMLList(map<MountLoad*, int > remainMLList) { _remainMLList = remainMLList; }

	map<Slot*, int> getTimeMap() { return _timeMap; }
	int getFreqOccupyNum(Slot* slot) { if (_timeMap.find(slot) != _timeMap.end()) { return _timeMap[slot]; } else { return 0; } }
	void setTimeMap(map<Slot*, int> timeMap);

	time_t getEarliestExploTime() { return _earliestExploTime; }
	void setEarliestExploTime(time_t eet) { _earliestExploTime = eet; }

	map<Slot*, vector<int>> getSlotPointMap() { return _slotPointMap; }
	void setSlotPoint(Slot* slot, vector<int> pointList) { _slotPointMap[slot] = pointList; }

	map<Target*, double>getTargetTimeMap() { return _targetTimeMap; }

	bool isVisit() { return _isVisit; };
	void setVisit(bool isV) { _isVisit = isV; }

	int getValue() { return _value; }
	void setValue(int value) { _value = value; }

	TourType getTourType() { return _tourType; }
	void setTourType(TourType tourType) { _tourType = tourType; }

	static bool cmpByCost(Tour* tourA, Tour* tourB);
	static bool cmpByOrder(Tour* tourA, Tour* tourB);

	Base* getBase() {return _base; };
	Fleet* getFleet() {return _fleet; };
    void setFleet(Fleet* fleet) {_fleet = fleet; };
	//返回的是飞机数量，但是这个项目是针对单个飞机的，所以不需要返回数量
	//int getAftNum();

	void computeCost();														//计算成本
	void computeLoadNum();													//计算总载荷类型/数量
	void computeMountNum();													//计算总挂载类型/数量
	void computeMountLoadNum();												//计算总挂载-载荷 类型/数量

	void computeCostAvgDistance();											//计算平均距离罚分
	void computeCostAftOil();												//计算燃油费
	double computeAvgPrio();												//计算平均优先级	
	bool isOccupy();														//该组合是否有挂载占频点
	static void initCountId() { _countId = 0; }
	void clearSoln();
	void assignTimeForSubTargets(Fleet* fleet, int aftNum);

	double getTargetDst(OperTarget* operTarget);

	void computeTimeMap();				//计算运力经过每个目标的时间，总航行时间

	void computeCostNew();				//计算成本(适用于本项目，路径规划包含在分配问题中)

	void print();
};

