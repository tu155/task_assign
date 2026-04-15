#pragma once
#include "Mount.h"
class MountType
{
public:
	int id = 0;
	vector<Mount*> mountList;
	int intervalTime = 30 * 60;
	int grpNumUb = 9999;
	string name;
	MountType(string n, int i, int g) :name(n), intervalTime(i), grpNumUb(g) {};
	int getId() { return id; }
	void setId(int ind) { id = ind; }
	void pushMount(Mount* mount) { mountList.push_back(mount); }
};
class MountLoad
{
public:
	int _id;
	Mount* _mount;
	Load* _load;
	MountLoad(Mount* mount, Load* load) :_mount(mount), _load(load) {};

	int getId() { return _id; }
	void setId(int id) { _id = id; }
	void print()
	{
		cout << "id-" << _id << " Mount-" << _mount->getName() << " Load-" << _load->getName() << endl;
	};
};

class TargetPlan
{
public:
	Mount* _mount;
	int mountNum;
	map<Load*, int> loadPlan;
	map<Load*, double>loadDamageProbability;
	TargetPlan(Mount* Mount, map<Load*, int> LoadPlan) :_mount(Mount), loadPlan(LoadPlan) {};//构造函数
	TargetPlan(Mount* Mount, map<Load*, double> LoadDamageProbability) :_mount(Mount), loadDamageProbability(LoadDamageProbability) {};
	string getName() { return _mount->getName(); }
	Mount* getMount() { return _mount; }
	map<Load*, int> getLoadPlan() { return loadPlan; }
	int getLoadNum(Load* load)
	{
		if (loadPlan.find(load) != loadPlan.end())
		{
			return loadPlan[load];
		}
		return 0;
	}
	void setMountNum(int num) { mountNum = num; }

	int getMountNum(Mount* mount)
	{
		if (mount != _mount)
		{
			return 0;
		}
		else
		{
			return mountNum;
		}
	}

	map<MountLoad*, int> _mountLoadPlan;
	map<Target*, double> _rateMap;//越小越好，计划对目标的毁伤几率？
	map<Target*, double> _preferMap;
	void setMountLoadPlan(map<MountLoad*, int> mountLoadPlan) { _mountLoadPlan = mountLoadPlan; }
	int getMountLoadNum(MountLoad* mountLoad)
	{
		if (_mountLoadPlan.find(mountLoad) != _mountLoadPlan.end())
		{
			return _mountLoadPlan[mountLoad];
		}
		return 0;
	}
	string name;

	void pushRate(Target* target, double rate) { _rateMap[target] = rate; }
	double getRate(Target* target)
	{
		if (_rateMap.find(target) != _rateMap.end())
		{
			return _rateMap[target];
		}
		return 0.0;
	}
	void pushPreference(Target* target, double preference) { _preferMap[target] = preference; }
	double getPreference(Target* target)
	{
		if (_preferMap.find(target) != _preferMap.end())
		{
			return _preferMap[target];
		}
		return 0.0;
	}
};

class MainTargetPlan
{
public:
	Mount* _mount;
	vector<Mount*> _mountNeedList;
	int mountNum;
	map<Load*, int> loadPlan;
	map<Load*, double>loadDamageProbability;
	MainTargetPlan(Mount* Mount, map<Load*, int> LoadPlan) :_mount(Mount), loadPlan(LoadPlan) {};//构造函数
	MainTargetPlan(Mount* Mount, map<Load*, double> LoadDamageProbability) :_mount(Mount), loadDamageProbability(LoadDamageProbability) {};
	string getName() { return _mount->getName(); }
	int getLoadNum(Load* load)
	{
		if (loadPlan.find(load) != loadPlan.end())
		{
			return loadPlan[load];
		}
		return 0;
	}
	void setMountNum(int num) { mountNum = num; }

	int getMountNum(Mount* mount)
	{
		if (mount != _mount)
		{
			return 0;
		}
		else
		{
			return mountNum;
		}
	}

	map<MountLoad*, int> _mountLoadPlan;
	map<Target*, double> _rateMap;
	map<Target*, double> _preferMap;
	void setMountLoadPlan(map<MountLoad*, int> mountLoadPlan) { _mountLoadPlan = mountLoadPlan; }
	int getMountLoadNum(MountLoad* mountLoad)
	{
		if (_mountLoadPlan.find(mountLoad) != _mountLoadPlan.end())
		{
			return _mountLoadPlan[mountLoad];
		}
		return 0;
	}
	string name;

	void pushRate(Target* target, double rate) { _rateMap[target] = rate; }
	double getRate(Target* target)
	{
		if (_rateMap.find(target) != _rateMap.end())
		{
			return _rateMap[target];
		}
		return 0.0;
	}
	void pushPreference(Target* target, double preference) { _preferMap[target] = preference; }
	double getPreference(Target* target)
	{
		if (_preferMap.find(target) != _preferMap.end())
		{
			return _preferMap[target];
		}
		return 0.0;
	}
};

