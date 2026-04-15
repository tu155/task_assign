#ifndef BASE_H
#define BASE_H

#include "Util.h"
#include "Mount.h"
#include "Load.h"
#include "Target.h"
#include "Fleet.h"
class Base
{//基地类
private:
	static int _countId;
	int _id;

	string _name;							//名称
	double _longitude;						//经度
	double _latitude;						//纬度
	string _region;							//区域（N）
	map<Mount*, int> _mountList;			//挂载清单
	map<Load*, int> _loadList;			//载荷清单
	map<Fleet*, int > _fleetList;		//机型清单
	map<FleetType*, int> _fleetTypeList;	//机型大类数量清单
	int _aftNum;							//飞机总量

	time_t _depTimeInterval;				//编队起飞时间间隔
	//vector<Group* > _groupList;			//编队集合
	map<Target*, double> _targetPreferList;//基地-目标罚分偏好
	//soln
	map<Fleet*, int> _usedFleetMap;		//使用的飞机集合
	map<Mount*, int> _usedMountMap;		//使用的挂载集合
	map<Load*, int> _usedLoadMap;			//使用的载荷集合

public:
	Base(string name, double longitude, double latitude, time_t depTimeInterval);		//基地构造函数
	~Base();																			//基地解构函数

	//属性的设置和获得函数
	int getId() { return _id; }
	void setId(int id) { _id = id; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	double getLongitude() { return _longitude; }
	void setLongitude(double longitude) { _longitude = longitude; }

	double getLatitude() { return _latitude; }
	void setLatitude(double latitude) { _latitude = latitude; }

	string getRegion() { return _region; }
	void setRegion(string region) { _region = region; }

	map<Mount*, int> getMountList() { return _mountList; }
	void pushMount(Mount* mount, int mountNum) { _mountList[mount] = mountNum; }
	int getMountNum(Mount* mount);

	map<Load*, int> getLoadList() { return _loadList; }
	void pushLoad(Load* load, int loadNum) { _loadList[load] = loadNum; }
	int getLoadNum(Load* load);

	map<Fleet*, int > getFleetList() { return _fleetList; }
	void pushFleet(Fleet* fleet, int fleetNum) { _fleetList[fleet] = fleetNum; }
	int getFleetNum(Fleet* fleet);

	map<FleetType*, int> getFleetTypeList() { return _fleetTypeList; }
	void pushFleetTypeAftNum(FleetType* fleetType, int aftNum) { _fleetTypeList[fleetType] = _fleetTypeList[fleetType] + aftNum; }
	int getFleetTypeAftNum(FleetType* fleetType);

	time_t getDepTimeInterval() { return _depTimeInterval; }
	void setDepTimeInterval(time_t depTimeInterval) { _depTimeInterval = depTimeInterval; }

	//vector<ZZGroup* > getHGroupList() { return _groupList; }
	//void pushGroup(ZZGroup* group) { _groupList.push_back(group); }

	void pushTargetPre(Target* target, double cost);
	double getTargetPre(Target* target);

	int getAftNum() { return _aftNum; }
	void pushAftNum(int aftNum) { _aftNum = _aftNum + aftNum; }

	void computeFleetTypeAftNum();													//计算该基地各机型大类的飞机数量
	map<Fleet*, int> getUsedFleetMap() { return _usedFleetMap; }					//用于获得使用的机型类型及数量
	map<Mount*, int> getUsedMountMap() { return _usedMountMap; }					//用于获得使用的挂载类型及数量
	map<Load*, int> getUsedLoadMap() { return _usedLoadMap; }						//用于获得使用的载荷类型及数量

	void pushUsedFleetMap(Fleet* fleet, int num);									//用于添加某机型的类型及使用数量
	void pushUsedMountMap(Mount* mount, int num);									//用于添加某挂载的类型及使用数量
	void pushUsedLoadMap(Load* load, int num);									//用于添加某载荷的类型及使用数量

	int getUsedFleetNum(Fleet* fleet);											//用于获得指定机型的使用数量
	int getUsedMountNum(Mount* mount);											//用于获得指定挂载的使用数量
	int getUsedLoadNum(Load* load);												//用于获得指定载荷的使用数量

	//void genGroup();																//用于生成编队
	static void initCountId() { _countId = 0; }

	void clearSoln();																//清空soln相关内容
	void print();
};
#endif // !ZZBASE_H
