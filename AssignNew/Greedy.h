#pragma once
#include "Individual.h"
//#include "NSGAII.h"
class Greedy {
private:
	Schedule* _schedule;
	map<string, vector<Tour*>> _result;
	map<string, map<string,float>> _objectives;
public:
	Greedy(Schedule* schedule);
    void greedyRun();
	void check(Individual ind);
	map<string, vector<Tour*>>getResult() {return _result;};
	map<string, map<string, float>>getObjectives() {return _objectives;};
};