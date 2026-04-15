#ifndef MOUNT_H
#define MOUNT_H

#include "Util.h"
#include "Load.h"

//#include "Load.h"

class MountType;
class Mount
{
private:
	static int _countId;
	int _id;
	string _name;

	double _costUse;
	time_t _reqTime;
	double _lbRate;
	double _prefRate;
	bool _isOccupy;
	double _range;
	int _stock_num;
	MountType* _mountType;

	vector<Load*> _loadList;
	map<Load*, double> _loadCostList;
	int _totalNum;
public:
	Mount(string name, double costUse);
	Mount(string name, time_t reqTime, double costUse, bool isOccupy, double range);
	Mount(string name, time_t reqTime, double costUse, bool isOccupy, double range,int stock_num);
	~Mount();
	int getId() { return _id; }
	void setId(int id) { _id = id; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	double getCostUse() { return _costUse; }
	void setCostUse(double costUse) { _costUse = costUse; }

	vector<Load*> getLoadList() { return _loadList; }
	void pushLoad(Load* load) { _loadList.push_back(load); }

	double getLoadCost(Load* load);
	void setLoadCost(Load* load, double cost);

	int getTotalNum() { return _totalNum; }
	void addMount(int num) { _totalNum += num; }

    int getStock_num() { return _stock_num; }

	time_t getReqTime() { return _reqTime; }
	void setReqTime(time_t time) { _reqTime = time; }

	double getLbRate() { return _lbRate; }
	void setLbRate(double rate) { _lbRate = rate; }

	double getPrefRate() { return _prefRate; }
	void setPrefRate(double rate) { _prefRate = rate; }

	bool isOccupy() { return _isOccupy; }
	void setOccupy(bool flag) { _isOccupy = flag; }

	MountType* getMountType() { return _mountType; }
	void setMountType(MountType* mt) { _mountType = mt; }

	double getRange() { return _range; }
	void setRange(double range) { _range = range; }
	static void initCountId() { _countId = 0; }

	void print();
};
#endif  