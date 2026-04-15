#ifndef TARGET_H
#define TARGET_H
#include "Util.h"
#include "TargetType.h"
#include "Fleet.h"
class Tour;
class MountLoad;
class MainTarget;
class Slot;
class Target
{//目标类
private:
	static int _countId;
	int _id;
	string _name;
	double _lpVal;							//目标是否被覆盖的求解结果值
	double _coef;							//罚分权重

	TargetType* _type;					//目标类型
	pair<time_t, time_t> _timeRange;		//时间窗
	int _priority;							//优先级
	int _order;								//打击相对顺序
	int _maxGrpNum;							//单目标分配运力数量上限

	double _longitude;						//经度
	double _latitude;						//纬度
	TargetStatus _targetStatus;

	/*先注释掉，目标都设置统一的毁伤率*/
	//vector<pair<double, string>>_rateObjList;	//目标损毁率，分强弱

	//soln
	TargetPlan* _solnTargetPlan;			//模型求解后选择的目标方案

	MainTarget* _mainTarget;				//所属主目标
	double _earliestCmpTime=0.0;				//目标的最早完成时间，计算完成后赋值，这里初始化为0
	double _lastPlanCmpTime=0.0;				//上次规划目标的完成时间
	//soln
	map<Fleet*, pair<int, map<MountLoad*, int>>>_asgResourse;
	map<Fleet*, double>_asgTime;
	double _airforceCost;
	double _oilCost;
	map< Slot*, map<MountLoad*, int>> _solnTimeMountMap;
	map< Slot*, map<MountLoad*, vector<int>>> _solnTimeAftMap;

public:
	Target(string name, TargetType* type, int maxGrpNum,pair<time_t, time_t> timeRange, int priority, double longitude, double latitude);	//目标构造函数
    Target(string name, TargetType* type,  int priority);	//目标构造函数
	~Target();
	//目标解构函数
	//属性的设置和获得函数
	int getId() { return _id; }
	void setId(int id) { _id = id; }

	//求解结果
	double getLpVal() { return _lpVal; }
	void setLpVal(double val) { _lpVal = val; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	pair<double, double> getPosition() { return make_pair(_longitude, _latitude); }
    void setPosition(double longitude, double latitude) { _longitude = longitude; _latitude = latitude; }

	TargetType* getType() { return _type; }
	void setTargetType(TargetType* type) { _type = type; }

	pair<time_t, time_t> getTimeRange() { return _timeRange; }
	void setTimeRange(time_t bgnTime, time_t endTime) { _timeRange = make_pair(bgnTime, endTime); }

	int getPriority() { return _priority; }
	void setPriority(int priority) { _priority = priority; }

	int getOrder() { return _order; }
	void setOrder(int order) { _order = order; }
	
	int getMaxGrpNum() { return _maxGrpNum; }
	void setMaxGrpNum(int num) { _maxGrpNum = num; }

	//void pushRateObj(double objRate, string lvl) { _rateObjList.push_back(make_pair(objRate, lvl)); }
	//vector<pair<double, string>> getRateObjList() { return _rateObjList; }

	double getLongitude() { return _longitude; }
	void setLongitude(double longitude) { _longitude = longitude; }

	double getLatitude() { return _latitude; }
	void setLatitude(double latitude) { _latitude = latitude; }

	map< Slot*, map<MountLoad*, int>> getSolnTimeMountMap() { return _solnTimeMountMap; }
	map< Slot*, map<MountLoad*, vector<int>>> getSolnTimeAftMap() { return _solnTimeAftMap; }
	void pushSolnTimeMount(Slot* slot, MountLoad* mountLoad, int num);
	void pushSolnTimeAft(Slot* slot, MountLoad* mountLoad, vector<int> numList);

	TargetStatus getStatus() { return _targetStatus; }
	void setStatus(TargetStatus targetStatus) { _targetStatus = targetStatus; }

	double getCoef() { return _coef; }
	void setCoef(double coef) { _coef = coef; }

	TargetPlan* getSolnTargetPlan() { return _solnTargetPlan; }
	void setSolnTargetPlan(TargetPlan* plan) { _solnTargetPlan = plan; }

	double getAirforceCost() { return _airforceCost; }
	double getOilCost() { return _oilCost; }
	

	double getEarliestCmpTime() { return _earliestCmpTime; }
	void setEarliestCmpTime(double eBgnT) { _earliestCmpTime = eBgnT; }

    double getLastPlanCmpTime() { return _lastPlanCmpTime; }
	void setLastPlanCmpTime(double lastPlanCmpTime) { _lastPlanCmpTime = lastPlanCmpTime; }

	MainTarget* getMainTarget() { return _mainTarget; }
	void setMainTarget(MainTarget* mTarget) { _mainTarget = mTarget; }

	static bool cmpByPrio(Target* t1, Target* t2);
	//static bool cmpByMTOrder(Target* t1, Target* t2);

	double getPlanPrefCost(TargetPlan* plan);

	void clearSoln();
	static void initCountId() { _countId = 0; }

	map<Fleet*, pair<int, map<MountLoad*, int>>> getAsgResourse() { return _asgResourse; }
	void pushResourse(Tour* tour, vector<pair<MountLoad*, int>> mlist);
	void print();
	map<Fleet*,double> getAsgTime() { return _asgTime; }

    void pushAsgTime(Fleet* fleet, double time) { _asgTime[fleet] = time; }
	
	// 获取按值排序的结果（返回vector）
	vector<pair<Fleet*, double>> getSortedByValue() const {
		vector<pair<Fleet*, double>> vec(_asgTime.begin(), _asgTime.end());

		// 按double值从小到大排序
		sort(vec.begin(), vec.end(),
			[](const pair<Fleet*, double>& a, const pair<Fleet*, double>& b) {
				return a.second < b.second;
			});

		return vec;
	}
};
#endif // ! TARGET_H