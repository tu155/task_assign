#pragma once
#include"Schedule.h"

typedef IloArray<IloNumVarArray> NumVar2Matrix;
typedef IloArray<IloNumArray> Num2Matrix;


class Model
{
private:
	Schedule* _schedule;
	double _tourTime;
	vector<Tour*> _tourList;
	vector<Target*> _targetList;
	/*不需要编队和基地，注释掉*/
	//vector<Group*> _groupList;
	vector<Mount*> _mountList;
	vector<Fleet*> _fleetList;
	vector<Load*> _loadList;
	//vector<Base*> _baseList;
	/*taskpoint 不确定含义，先注释*/
	//vector<TaskPoint*> _taskPointList;
	vector<MountLoad*> _mountLoadList;
	vector<FleetType*> _fleetTypeList;
	/*不需要演训经验*/
	//vector<AftTrainPair*> _aftTrainPairList;
	vector<Slot* > _slotList;
	vector<MainTarget*> _mainTargetList;
	vector<pair<MainTarget*, MainTarget*>> _mtPrioPairList;
	/*不考虑编队Group，修改为Fleet**/
	vector<pair<Fleet*, Tour*>> _solnList;
	vector<Tour* > _solnTourList;
	vector<MountType*> _mountTypeList;
	map<Slot*, map<MountType*, int>> _regionGrpCapMap;
	set<Target*> _addedTargetSet;
	vector<pair<string, vector<Target*>>> _batchTargetMap;

	IloEnv _env;
	IloModel _model;
	IloCplex _solver;
	IloObjective _obj;

	IloNumVarArray _y_gr;
	IloRangeArray _assignRng;
	IloNumVarArray _x_gtr;

	NumVar2Matrix _o_tm;
	IloNumVarArray _z_t;
	IloNumVarArray _p_t;
	IloNumVarArray _gn_t;
	NumVar2Matrix _q_ts;
	IloNumVarArray _q_t;
	IloNumVarArray _q_c;


	IloRangeArray _fCapRng;
	IloRangeArray _wDmdRng;
	IloRangeArray _uDmdRng;
	IloRangeArray _wCapRng;
	IloRangeArray _uCapRng;
	IloRangeArray _dmdAsgRng;
	IloRangeArray _pairDmdRng;
	IloRangeArray _slotRng;

	IloRangeArray _mtSlotLbRng;
	IloRangeArray _mtSlotUbRng;

	IloRangeArray _mtEarlyBgnTimeRng;
	IloRangeArray _mtEarlyBgnTimeUbRng;
	IloRangeArray _mtEarlyBgnTimeLbRng;
	IloRangeArray _mtPriorityRng;
	IloRangeArray _mtPriorityCopyRng;
	IloRangeArray _mtCoverUbRng;
	IloRangeArray _mtCoverLbRng;
	IloRangeArray _regionGrpUbRng;

	IloNumVarArray _u_f;
	IloNumVarArray _overAft_bf;
	IloNumVarArray _lessAft_bf;
	IloNumVarArray _lessLbAft_bf;

	IloRangeArray _aftBaseLbRng;
	IloRangeArray _aftUseRateRng;
	IloRangeArray _aftBaseFairRng;

	IloNumVarArray _u_m;
	IloNumVarArray _overMount_bm;
	IloNumVarArray _lessMount_bm;
	IloNumVarArray _lessLbMount_bm;

	IloRangeArray _mountBaseLbRng;
	IloRangeArray _mountUseRateRng;
	IloRangeArray _mountBaseFairRng;

	IloNumVarArray _u_l;
	IloNumVarArray _overLoad_bl;
	IloNumVarArray _lessLoad_bl;
	IloNumVarArray _lessLbLoad_bl;

	IloRangeArray _loadBaseLbRng;
	IloRangeArray _loadUseRateRng;
	IloRangeArray _loadBaseFairRng;

	IloNumVarArray _u_ft;
	IloNumVarArray _overAft_bft;
	IloNumVarArray _lessAft_bft;
	IloNumVarArray _lessLbFAft_bft;

	IloRangeArray _aftFBaseLbRng;
	IloRangeArray _aftFUseRateRng;
	IloRangeArray _aftFBaseFairRng;

	IloNumVarArray _u_s;
	IloNumVarArray _overMTSlot_mt;
	IloNumVarArray _lessMTSlot_mt;

	IloRangeArray _mtSlotRateRng;
	IloRangeArray _mtSlotFairRng;

	IloRangeArray _attackRobustRng;
	IloNumVarArray _slackRb_t;

	IloNumVarArray _q_tbf;
	IloRangeArray _targetGrpPreRng;

	IloNumVarArray _q_tbbff;
	IloRangeArray _trainFreqPreRng;

	IloRangeArray _tgAsgNumRng;
	IloRangeArray _ttpAsgRng;
	IloRangeArray _ttpLimitRng;

	IloConversion _convertIP;
	IloConversion _convertIP1;
	IloConversion _convertIP2;


	clock_t _startClock;
	clock_t _currentClock;

	int _colGenCount;
	int _solnCount;

	double _costTotalTour = 0;
	double _maxTourTime = 0;
public:
	Model(Schedule* sch);
	~Model();

	void init();
	void addColsByGrp(vector<Tour*> tourList);
	void addFleetSelectionConstraints(const map<int, vector<IloNumVar>>& fleetToVarsMap);
	void addColsByGrpType(vector<Tour*> tourList);

	void solveLP();
	void solveIP();
	void solveDiving();
	void solve();

	void setSolnInfor();
	void resultAnalysis();
	void assignTimeForSubTargets();

	void setSolnInforSub();
	bool feasiblePush(Tour* tour);
	int getMaxPrevMTBgnTime(int mtInd);
	int setSolnLP();
	vector<Tour*> getSolnTourList() { return _solnTourList; };
	double getTourTime() { return _tourTime; };
	void setTourTime(double tourTime) { _tourTime = tourTime; };
};