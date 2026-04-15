#ifndef MAINTARGET_H
#define MAINTARGET_H
#include"Util.h"
class Target;
class MainTarget
{
private:
	static int _countId;
	int _id;
	string _batchId;
	string _name;
	int _priority;
	int _order;
	vector<Target* > _targetList;
	int _latestBgnTime;

	int _solnEarliesBgnTime;

public:
	MainTarget(string name, int priority, vector<Target*> targets);
	~MainTarget();

	int getId() { return _id; }
	void setId(int id) { _id = id; }

	string getBatchId() { return _batchId; }
	string getName() { return _name; }
	int getPriority() { return _priority; }
	
	int getOrder() { return _order; }
	void setOrder(int order) { _order = order; }

	vector<Target* > getTargetList() { return _targetList; }
	void pushTarget(Target* target) { _targetList.push_back(target); }
	void setTargetList(vector<Target*> targets) { _targetList = targets; }
	static bool cmpByOrder(MainTarget* tA, MainTarget* tB);

	int getLatestBgnTime() { return _latestBgnTime; }
	void setLatestBgnTime(int lBgnT) { if (lBgnT > _latestBgnTime) { _latestBgnTime = lBgnT; } }

	int getSolnEarliesBgnTime() { return _solnEarliesBgnTime; }
	void setSolnEarliestBgnTime(int seBgnT) { _solnEarliesBgnTime = seBgnT; }
	bool cmpByMTOrder(Target* t1, Target* t2);
	static void initCountId() { _countId = 0; }
	void clearSoln();
	void print();
};

#endif