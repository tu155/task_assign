#ifndef FLEET_H
#define FLEET_H
#include "Util.h"
#include "Mount.h"
class Fleet;
class FleetType
{
public:
	int _id;
	string _name;
	vector<Fleet*> _fleetList;
	int _totalAftNum;

	FleetType(string name) :_name(name) { _totalAftNum = 0; };

	int getId() { return _id; }
	string getName() { return _name; }
	//新增force
	void pushFleet(Fleet* fleet) { _fleetList.push_back(fleet); }
	//获取所有force
	vector<Fleet*> getFleetList() { return _fleetList; }
	void setId(int id) { _id = id; }
	void addAftNum(double aftNum) { _totalAftNum = _totalAftNum + aftNum; }
    int getAftNum() { return _totalAftNum; }
	void print()
	{
		cout << "ForceType name-" << _name << "总数量为"<< _totalAftNum << endl;
	}
};

class Fleet
{//机型类
private:
	static int _countId;
	int _id;
	string _name;

	double _range;							//航程
	double _longitude;						//经度
	double _latitude;							//纬度
	double _optSpeed;								//估计巡航速度 km/h
	pair<double, double> _speedRange;				//巡航速度范围(km/s)TODO 问题二单位确定
	double _fuel;									//油量（吨）
	time_t _prepareTime;							//复飞准备时间
	double _oilPerMin;								//油耗每分钟
	FleetType* _type;								//机型类型

	double _costUse;								//使用成本
	double _costSpeed;								//速度偏离罚分

	vector<pair<Mount*, int>> _mountPlanList;		//挂载方案列表

public:
	Fleet(string name, double range,double optSpeed, pair<double, double> speedRange, 
		double oilPerMin, time_t prepareTime, double costUse, double costSpeed);	//机型构造函数
	Fleet(string name, double costUse, double optSpeed, double oilPerMin);
	Fleet() : _type(nullptr) {}  // 明确初始化为nullptr

	//机型解构函数
	~Fleet();//析构函数，释放内存

	//属性的设置和获得函数
	pair<double, double> getPos() { return make_pair(_longitude, _latitude); }
    void setPos(double longitude, double latitude) { _longitude = longitude; _latitude = latitude; }

	int getId() { return _id; }
	void setId(int id) { _id = id; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	void setFleetType(FleetType* type) { _type = type; }
	FleetType* getFleetType() { return _type; }

	double getRange() { return _range; }
	void setRange(double range) { _range = range; }

	double getOptSpeed() { return _optSpeed; }
	void setOptSpeed(double optSpeed) { _optSpeed = optSpeed; }

	double getOilPerMin() { return _oilPerMin; }
	void setOilPerMin(double oilPerMin) { _oilPerMin = oilPerMin; }

	pair<double, double> getSpeedRange() { return _speedRange; }
	void setSpeedRange(double speedLB, double speedUB) { _speedRange = make_pair(speedLB, speedUB); }

	double getFuel() { return _fuel; }
	void setFuel(double fuel) { _fuel = fuel; }

	time_t getPrepareTime() { return _prepareTime; }
	void setPrepareTime(time_t prepareTime) { _prepareTime = prepareTime; }

	double getCostUse() { return _costUse; }
	void setCostUse(double costUse) { _costUse = costUse; }

	double getCostSpeed() { return _costSpeed; }
	void setCostSpeed(double costSpeed) { _costSpeed = costSpeed; }

	vector<pair<Mount*, int>> getMountPlanList() { return _mountPlanList; }
	void pushMountPlan(Mount* mount, int num) { _mountPlanList.push_back(make_pair(mount, num)); }
	int getMountNum(Mount* mount);

	FleetType* getType() { return _type; }
	void setType(FleetType* type) { _type = type; }

	void print();
};

#endif // !ZZFLEET_H
