#include "Model.h"
#include <algorithm>
using std::cout;
Model::Model(Schedule* sch)
	:_schedule(sch)
{
	_targetList = _schedule->getTargetList();
	//_groupList = _schedule->getGroupList();
	_mountList = _schedule->getMountList();
	_loadList = _schedule->getLoadList();
	//_baseList = _schedule->getBaseList();
	_mountLoadList = _schedule->getMountLoadList();
	//_taskPointList = _schedule->getTaskPointList();
	_fleetList = _schedule->getFleetList();
	//_fleetTypeList = _schedule->getFleetTypeList();
	//_aftTrainPairList = _schedule->getAftTrainPairList();
	_slotList = _schedule->getSlotList();
	_mainTargetList = _schedule->getMainTargetList();
	_mountTypeList = _schedule->getMountTypeList();
	/*地区释义不明，注释掉*/
	//_regionGrpCapMap = _schedule->getRegionGrpCapMap();
	_batchTargetMap = _schedule->getBatchTargetMap();

	for (int i = 0; i < _targetList.size(); i++)
	{
		_targetList[i]->setId(i);
	}
	//for (int i = 0; i < _groupList.size(); i++)
	//{
	//	_groupList[i]->setId(i);
	//}
	for (int i = 0; i < _mountList.size(); i++)
	{
		_mountList[i]->setId(i);
	}
	for (int i = 0; i < _loadList.size(); i++)
	{
		_loadList[i]->setId(i);
	}
	//for (int i = 0; i < _baseList.size(); i++)
	//{
	//	_baseList[i]->setId(i);
	//}
	for (int i = 0; i < _mountLoadList.size(); i++)
	{
		_mountLoadList[i]->setId(i);
	}
	//for (int i = 0; i < _taskPointList.size(); i++)
	//{
	//	_taskPointList[i]->setId(i);
	//}
	for (int i = 0; i < _fleetList.size(); i++)
	{
		_fleetList[i]->setId(i);
	}
	//for (int i = 0; i < _fleetTypeList.size(); i++)
	//{
	//	_fleetTypeList[i]->setId(i);
	//}
	//for (int i = 0; i < _aftTrainPairList.size(); i++)
	//{
	//	_aftTrainPairList[i]->setId(i);
	//}
	for (int i = 0; i < _slotList.size(); i++)
	{
		_slotList[i]->setId(i);
	}
	sort(_batchTargetMap.begin(), _batchTargetMap.end(), Util::cmpBatchByOrder);

	sort(_mainTargetList.begin(), _mainTargetList.end(), MainTarget::cmpByOrder);
	//vector<pair<int, vector<MainTarget*>>> orderMTList;
	//for (int i = 0; i < _mainTargetList.size(); i++)
	//{
	//	_mainTargetList[i]->setId(i);

	//	bool ifPush = false;
	//	for (int j = 0; j < orderMTList.size(); j++)
	//	{
	//		if (orderMTList[j].first == _mainTargetList[i]->getOrder())
	//		{
	//			orderMTList[j].second.push_back(_mainTargetList[i]);
	//			ifPush = true;
	//			break;
	//		}
	//	}
	//	if (!ifPush)
	//	{
	//		vector<MainTarget*> mtList;
	//		mtList.push_back(_mainTargetList[i]);
	//		orderMTList.push_back(make_pair(_mainTargetList[i]->getOrder(), mtList));
	//	}
	//}
	//for (int i = 0; i < orderMTList.size() - 1; i++)
	//{
	//	vector<MainTarget*> mtList = orderMTList[i].second;
	//	vector<MainTarget*> nextmtList = orderMTList[i + 1].second;
	//	for (int j = 0; j < mtList.size(); j++)
	//	{
	//		for (int q = 0; q < nextmtList.size(); q++)
	//		{
	//			_mtPrioPairList.push_back(make_pair(mtList[j], nextmtList[q]));
	//		}
	//	}
	//}
	//cout << "mtPrioPairList size is " << _mtPrioPairList.size() << endl;

	_startClock = clock();
	_colGenCount = 0;
	_solnCount = 0;
}

Model::~Model()
{
}

void Model::init()
{
	_model = IloModel(_env);
	_obj = IloAdd(_model, IloMinimize(_env));//目标函数最小化
	_solver = IloCplex(_model);
	_y_gr = IloNumVarArray(_env);
	_x_gtr = IloNumVarArray(_env);//未用到
	_assignRng = IloRangeArray(_env);
	_wDmdRng = IloRangeArray(_env);
	_uDmdRng = IloRangeArray(_env);
	_wCapRng = IloRangeArray(_env);
	_uCapRng = IloRangeArray(_env);
	_fCapRng = IloRangeArray(_env);
	_dmdAsgRng = IloRangeArray(_env);
	_pairDmdRng = IloRangeArray(_env);
	_tgAsgNumRng = IloRangeArray(_env);
	_ttpAsgRng = IloRangeArray(_env);
	_ttpLimitRng = IloRangeArray(_env);
	_attackRobustRng = IloRangeArray(_env);//相比ResModel新增，攻击鲁棒性约束
	_slotRng = IloRangeArray(_env);
	_mtSlotLbRng = IloRangeArray(_env);
	_mtSlotUbRng = IloRangeArray(_env);
	_mtEarlyBgnTimeRng = IloRangeArray(_env);
	_mtEarlyBgnTimeUbRng = IloRangeArray(_env);
	_mtEarlyBgnTimeLbRng = IloRangeArray(_env);
	_mtPriorityRng = IloRangeArray(_env);
	_mtPriorityCopyRng = IloRangeArray(_env);
	_mtCoverUbRng = IloRangeArray(_env);
	_mtCoverLbRng = IloRangeArray(_env);//相比ResModel新增
	_regionGrpUbRng = IloRangeArray(_env);//相比ResModel新增

	int totalRngSize = 0;
	setTourTime(0.0);

	_o_tm = NumVar2Matrix(_env, _targetList.size());
	for (int t = 0; t < _targetList.size(); t++)
	{
		_o_tm[t] = IloNumVarArray(_env);//决策变量，目标t是否选用配置方案m
		vector<TargetPlan* > planList = _targetList[t]->getType()->getTargetPlanList();
		for (int j = 0; j < planList.size(); j++)
		{
			double cost = 0.0;
			int ub = 1;
			//if (Util::ifEvaDmdTargetPrefer)
			//{
			//	cost += (1 - planList[j]->getPreference(_targetList[t])) * Util::costDmdTarPref;
			//}
			/*if (Util::ifEvaDestroyDegree)
			{
				cost += (1 - planList[j]->getRate(_targetList[t])) * Util::costDmdTarRate;
			}*/
			string name = "O_T" + to_string(_targetList[t]->getId()) + "_M" + to_string(j);
			_o_tm[t].add(IloNumVar(_env, 0, ub, ILOFLOAT, name.c_str()));
			//_obj.setLinearCoef(_o_tm[t][j], -cost);//目标t选用配置方案m的成本？？
		}
		_model.add(_o_tm[t]);
	}
	/*
    约束1：每个目标只能选择唯一计划，或者没被覆盖
	*/
	for (int i = 0; i < _targetList.size(); i++)
	{
		vector<TargetPlan*> planList = _targetList[i]->getType()->getTargetPlanList();
		IloExpr expr(_env);
		for (int p = 0; p < planList.size(); p++)
		{
			expr += _o_tm[i][p];
		}
		string name = "C2_DmdAsg_T" + to_string(_targetList[i]->getId());
		double perturb = 0; 		
		_dmdAsgRng.add(IloRange(_env, 1 - perturb, expr, 1 + perturb, name.c_str()));
	}
	_model.add(_dmdAsgRng);
	cout << "#dmdAsgRng size:" << _dmdAsgRng.getSize() << endl;
	totalRngSize += _dmdAsgRng.getSize();
	/*
	约束2：目标的挂载-载荷对的需求满足
	*/
	for (int t = 0; t < _targetList.size(); t++)
	{
		vector<TargetPlan*> planList = _targetList[t]->getType()->getTargetPlanList();
		for (int j = 0; j < _mountLoadList.size(); j++)
		{
			IloExpr expr(_env);
			for (int p = 0; p < planList.size(); p++)
			{
				expr += (-1) * planList[p]->getMountLoadNum(_mountLoadList[j]) * _o_tm[t][p];//计算了计划的需求量
			}
			string name = "C3_pairDmd_P" + to_string(_mountLoadList[j]->getId()) + "_T" + to_string(_targetList[t]->getId());
			_pairDmdRng.add(IloRange(_env, 0, expr, IloInfinity, name.c_str()));
		}
	}
	_model.add(_pairDmdRng);
	cout << "#pairDmdRng size:" << _pairDmdRng.getSize() << endl;
	totalRngSize += _pairDmdRng.getSize();
	
	//约束3：基地的载荷数量上限（只有约束范围，缺少变量和系数设置）
	for (int j = 0; j < _loadList.size(); j++)
	{
		string name = "C5_uCap_U" + to_string(_loadList[j]->getId());
		_uCapRng.add(IloRange(_env, 0, 10000, name.c_str()));
	}
	/*for (int i = 0; i < _baseList.size(); i++)
	{
		for (int j = 0; j < _loadList.size(); j++)
		{
			string name = "C5_uCap_U" + to_string(_loadList[j]->getId()) + "_B" + to_string(_baseList[i]->getId());
			_uCapRng.add(IloRange(_env, 0, _baseList[i]->getLoadNum(_loadList[j]), name.c_str()));
		}
	}*/
	_model.add(_uCapRng);
	cout << "#uCapRng size:" << _uCapRng.getSize() << endl;
	totalRngSize += _uCapRng.getSize();
	
	//约束4：挂载数量上限，默认10000（只有约束范围，缺少变量和系数设置）
	for (int j = 0; j < _mountList.size(); j++)
	{
		string name = "C6_wCap_W" + to_string(_mountList[j]->getId());
		_wCapRng.add(IloRange(_env, 0, 10000, name.c_str()));
	}
	//for (int i = 0; i < _baseList.size(); i++)
	//{
	//	for (int j = 0; j < _mountList.size(); j++)
	//	{
	//		string name = "C6_wCap_W" + to_string(_mountList[j]->getId()) + "_B" + to_string(_baseList[i]->getId());
	//		_wCapRng.add(IloRange(_env, 0, _baseList[i]->getMountNum(_mountList[j]), name.c_str()));
	//	}
	//}
	_model.add(_wCapRng);
	cout << "#wCapRng size:" << _wCapRng.getSize() << endl;
	totalRngSize += _wCapRng.getSize();

	//约束：飞机数量上限（只有约束范围，缺少变量和系数设置）
	for (int j = 0; j < _fleetList.size(); j++)
	{
		string name = "C7_fCap_F" + to_string(_fleetList[j]->getId());
		_fCapRng.add(IloRange(_env, 0, _schedule->getFleetList().size(), name.c_str()));
	}
	/*for (int i = 0; i < _baseList.size(); i++)
	{
		for (int j = 0; j < _fleetList.size(); j++)
		{
			string name = "C7_fCap_F" + to_string(_fleetList[j]->getId()) + "_B" + to_string(_baseList[i]->getId());
			_fCapRng.add(IloRange(_env, 0, _baseList[i]->getFleetNum(_fleetList[j]) * ZZUtil::groupRoundNumLimit, name.c_str()));
		}
	}*/
	_model.add(_fCapRng);
	cout << "#fCapRng size:" << _fCapRng.getSize() << endl;
	totalRngSize += _fCapRng.getSize();
	/*由于计算时间过长，尝试注释*/
	////约束：给每个目标分配的最大运力数量
	//for (int i = 0; i < _targetList.size(); i++)
	//{
	//	string name = "C8_tgAsgNumRng_T" + to_string(_targetList[i]->getId());
	//	int grpPerTarNumUb = Util::groupPertargetNumLimit;
	//	if (Util::ifEvaGrpNumPerTargetUb)
	//	{
	//		if (_targetList[i]->getMaxGrpNum() > 0)
	//		{
	//			grpPerTarNumUb = _targetList[i]->getMaxGrpNum();
	//		}
	//	}
	//	_tgAsgNumRng.add(IloRange(_env, 0, grpPerTarNumUb, name.c_str()));
	//}
	//_model.add(_tgAsgNumRng);
	//cout << "#tgAsgNumRng size:" << _tgAsgNumRng.getSize() << endl;
	//totalRngSize += _tgAsgNumRng.getSize();

	//_p_t = IloNumVarArray(_env);//单目标运力数量未超过阈值的松弛变量，数量为目标总数
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	IloNumColumn col(_env);
	//	col = _obj(Util::revLessGrpTarNum);
	//	col += _tgAsgNumRng[_targetList[t]->getId()](1);
	//	string name = "P_T" + to_string(_targetList[t]->getId());
	//	_p_t.add(IloNumVar(col, 0, Util::targetPerGroupNumLimit, ILOFLOAT, name.c_str()));
	//}
	//_model.add(_p_t);

	//_gn_t = IloNumVarArray(_env);//单目标运力数量超过阈值的松弛变量，数量为目标总数
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	IloNumColumn col(_env);
	//	col = _obj(Util::costOverGrpPerTarNum);
	//	col += _tgAsgNumRng[_targetList[t]->getId()](-1);
	//	string name = "GN_T" + to_string(_targetList[t]->getId());
	//	_gn_t.add(IloNumVar(col, 0, IloInfinity, ILOFLOAT, name.c_str()));
	//}
	//_model.add(_gn_t);

	//先不考虑公平因素，注释掉
	/*
	if (Util::ifEvaBaseAftUseFair)
	{
		_u_f = IloNumVarArray(_env);
		for (int i = 0; i < _fleetList.size(); i++)
		{
			string name = "U_F" + to_string(_fleetList[i]->getId());
			_u_f.add(IloNumVar(_env, 0, 1, ILOFLOAT, name.c_str()));
		}
		_model.add(_u_f);

		_aftUseRateRng = IloRangeArray(_env);
		for (int i = 0; i < _fleetList.size(); i++)
		{
			IloExpr expr(_env);
			expr -= _u_f[i] * _fleetList[i]->getTotalNum();
			string name = "C9_aftUseRateRng _F" + to_string(_fleetList[i]->getId());
			_aftUseRateRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			expr.end();
		}
		_model.add(_aftUseRateRng);
		cout << "#aftUseRateRng size:" << _aftUseRateRng.getSize() << endl;
		totalRngSize += _aftUseRateRng.getSize();

		_aftBaseFairRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetList.size(); j++)
			{
				IloExpr expr(_env);
				string name = "C10_aftBaseFairRng_B" + to_string(_baseList[i]->getId()) + "F_" + to_string(_fleetList[j]->getId());
				expr -= _u_f[j];
				_aftBaseFairRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			}
		}
		_model.add(_aftBaseFairRng);
		cout << "#aftBaseFairRng size:" << _aftBaseFairRng.getSize() << endl;
		totalRngSize += _aftBaseFairRng.getSize();

		_overAft_bf = IloNumVarArray(_env);
		_lessAft_bf = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetList.size(); j++)
			{
				IloNumColumn colOver(_env);
				if (_baseList[i]->getFleetNum(_fleetList[j]) > 0)
				{
					colOver = _obj(ZZUtil::costFleetBaseFair);
				}
				else
				{
					colOver = _obj(0);
				}
				colOver += _aftBaseFairRng[i * _fleetList.size() + j](-1);
				string name = "OverAft_B" + to_string(_baseList[i]->getId()) + "_F" + to_string(_fleetList[j]->getId());
				_overAft_bf.add(IloNumVar(colOver, 0, 1, ILOFLOAT, name.c_str()));

				IloNumColumn colLess(_env);
				if (_baseList[i]->getFleetNum(_fleetList[j]) > 0)
				{
					colLess = _obj(ZZUtil::costFleetBaseFair);
				}
				else
				{
					colLess = _obj(0);
				}
				colLess += _aftBaseFairRng[i * _fleetList.size() + j](1);
				name = "LessAft_B" + to_string(_baseList[i]->getId()) + "_F" + to_string(_fleetList[j]->getId());
				_lessAft_bf.add(IloNumVar(colLess, 0, 1, ILOFLOAT, name.c_str()));
			}
		}

		_aftBaseLbRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetList.size(); j++)
			{
				string name = "C12_aftBaseLbRng_B" + to_string(_baseList[i]->getId()) + "F_" + to_string(_fleetList[j]->getId());
				_aftBaseLbRng.add(IloRange(_env, -IloInfinity, 1 - _fleetList[j]->getLbRate(), name.c_str()));
			}
		}
		_model.add(_aftBaseLbRng);
		cout << "#aftBaseLbRng size:" << _aftBaseLbRng.getSize() << endl;
		totalRngSize += _aftBaseLbRng.getSize();

		_lessLbAft_bf = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetList.size(); j++)
			{
				IloNumColumn col(_env);
				col = _obj(ZZUtil::costBaseLessThanRatio);
				col += _aftBaseLbRng[i * _fleetTypeList.size() + j](-1);
				string name = "LessLbAft_B" + to_string(_baseList[i]->getId()) + "_F" + to_string(_fleetList[j]->getId());
				_lessLbAft_bf.add(IloNumVar(col, 0, 1, ILOFLOAT, name.c_str()));
			}
		}
	}

	if (ZZUtil::ifEvaBaseFleetTypeUseFair == true)
	{
		_u_ft = IloNumVarArray(_env);
		for (int i = 0; i < _fleetTypeList.size(); i++)
		{
			string name = "U_FT" + to_string(_fleetTypeList[i]->getId());
			_u_ft.add(IloNumVar(_env, 0, 1, ILOFLOAT, name.c_str()));
		}
		_model.add(_u_ft);

		_aftFUseRateRng = IloRangeArray(_env);
		for (int i = 0; i < _fleetTypeList.size(); i++)
		{
			IloExpr expr(_env);
			expr -= _u_ft[i] * _fleetTypeList[i]->_totalAftNum;
			string name = "C13_aftUseRateRng_FT" + to_string(_fleetTypeList[i]->getId());
			_aftFUseRateRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			expr.end();
		}
		_model.add(_aftFUseRateRng);
		cout << "#aftFUseRateRng size:" << _aftFUseRateRng.getSize() << endl;
		totalRngSize += _aftFUseRateRng.getSize();

		_aftFBaseFairRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetTypeList.size(); j++)
			{
				IloExpr expr(_env);
				string name = "C14_aftFBaseFairRng_B" + to_string(_baseList[i]->getId()) + "FT_" + to_string(_fleetTypeList[j]->getId());
				expr -= _u_ft[j];
				_aftFBaseFairRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			}
		}
		_model.add(_aftFBaseFairRng);
		cout << "#aftFBaseFairRng size:" << _aftFBaseFairRng.getSize() << endl;
		totalRngSize += _aftFBaseFairRng.getSize();

		_overAft_bft = IloNumVarArray(_env);
		_lessAft_bft = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetTypeList.size(); j++)
			{
				IloNumColumn colOver(_env);
				colOver = _obj(ZZUtil::costFleetTypeBaseFair);
				colOver += _aftFBaseFairRng[i * _fleetTypeList.size() + j](-1);
				string name = "OverAft_B" + to_string(_baseList[i]->getId()) + "_FT" + to_string(_fleetTypeList[j]->getId());
				_overAft_bft.add(IloNumVar(colOver, 0, 1, ILOFLOAT, name.c_str()));

				IloNumColumn colLess(_env);
				colLess = _obj(ZZUtil::costFleetTypeBaseFair);
				colLess += _aftFBaseFairRng[i * _fleetTypeList.size() + j](1);
				name = "LessAft_B" + to_string(_baseList[i]->getId()) + "_FT" + to_string(_fleetTypeList[j]->getId());
				_lessAft_bft.add(IloNumVar(colLess, 0, 1, ILOFLOAT, name.c_str()));
			}
		}
		_model.add(_overAft_bft);
		_model.add(_lessAft_bft);

		_aftFBaseLbRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetTypeList.size(); j++)
			{
				string name = "C12_aftFBaseLbRng_B" + to_string(_baseList[i]->getId()) + "Ft_" + to_string(_fleetTypeList[j]->getId());
				_aftFBaseLbRng.add(IloRange(_env, -IloInfinity, 1 - _fleetTypeList[j]->_remainLowerRate, name.c_str()));
			}
		}
		_model.add(_aftFBaseLbRng);
		cout << "#aftFBaseLbRng size:" << _aftFBaseLbRng.getSize() << endl;
		totalRngSize += _aftFBaseLbRng.getSize();

		_lessLbFAft_bft = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _fleetTypeList.size(); j++)
			{
				IloNumColumn col(_env);
				col = _obj(ZZUtil::costBaseLessThanRatio);
				col += _aftFBaseLbRng[i * _fleetTypeList.size() + j](-1);
				string name = "LessLbAft_B" + to_string(_baseList[i]->getId()) + "_Ft" + to_string(_fleetTypeList[j]->getId());
				_lessLbFAft_bft.add(IloNumVar(col, 0, 1, ILOFLOAT, name.c_str()));
			}
		}
	}

	if (ZZUtil::ifEvaBaseMountUseFair)
	{
		_u_m = IloNumVarArray(_env);
		for (int i = 0; i < _mountList.size(); i++)
		{
			string name = "U_M" + to_string(_mountList[i]->getId());
			_u_m.add(IloNumVar(_env, 0, 1, ILOFLOAT, name.c_str()));
		}
		_model.add(_u_m);

		_mountUseRateRng = IloRangeArray(_env);
		for (int i = 0; i < _mountList.size(); i++)
		{
			IloExpr expr(_env);
			expr -= _u_m[i] * _mountList[i]->getTotalNum();
			string name = "C11_mountUseRateRng _F" + to_string(_mountList[i]->getId());
			_mountUseRateRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			expr.end();
		}
		_model.add(_mountUseRateRng);
		cout << "#mountUseRateRng size:" << _mountUseRateRng.getSize() << endl;
		totalRngSize += _mountUseRateRng.getSize();

		_mountBaseFairRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _mountList.size(); j++)
			{
				IloExpr expr(_env);
				string name = "C12_mountBaseFairRng_B" + to_string(_baseList[i]->getId()) + "M_" + to_string(_mountList[j]->getId());
				expr -= _u_m[j];
				_mountBaseFairRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			}
		}
		_model.add(_mountBaseFairRng);
		cout << "#mountBaseFairRng size:" << _mountBaseFairRng.getSize() << endl;
		totalRngSize += _mountBaseFairRng.getSize();

		_overMount_bm = IloNumVarArray(_env);
		_lessMount_bm = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _mountList.size(); j++)
			{
				IloNumColumn colOver(_env);
				colOver = _obj(ZZUtil::costMountBaseFair);
				colOver += _mountBaseFairRng[i * _mountList.size() + j](-1);
				string name = "OverMount_B" + to_string(_baseList[i]->getId()) + "_M" + to_string(_mountList[j]->getId());
				_overMount_bm.add(IloNumVar(colOver, 0, 1, ILOFLOAT, name.c_str()));

				IloNumColumn colLess(_env);
				colLess = _obj(ZZUtil::costMountBaseFair);
				colLess += _mountBaseFairRng[i * _mountList.size() + j](1);
				name = "LessMount_B" + to_string(_baseList[i]->getId()) + "_M" + to_string(_mountList[j]->getId());
				_lessMount_bm.add(IloNumVar(colLess, 0, 1, ILOFLOAT, name.c_str()));
			}
		}

		_mountBaseLbRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _mountList.size(); j++)
			{
				string name = "C20_mountBaseLbRng_B" + (_baseList[i]->getName()) + "M_" + (_mountList[j]->getName());
				_mountBaseLbRng.add(IloRange(_env, -IloInfinity, 1 - _mountList[j]->getLbRate(), name.c_str()));
			}
		}
		_model.add(_mountBaseLbRng);
		cout << "#mountBaseLbRng size:" << _mountBaseLbRng.getSize() << endl;
		totalRngSize += _mountBaseLbRng.getSize();

		_lessLbMount_bm = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _mountList.size(); j++)
			{
				IloNumColumn col(_env);
				col = _obj(ZZUtil::costBaseLessThanRatio);
				col += _mountBaseLbRng[i * _mountList.size() + j](-1);
				string name = "LessLbMount_B" + to_string(_baseList[i]->getId()) + "_M" + to_string(_mountList[j]->getId());
				_lessLbMount_bm.add(IloNumVar(col, 0, 1, ILOFLOAT, name.c_str()));
			}
		}
	}

	if (ZZUtil::ifEvaBaseLoadUseFair)
	{
		_u_l = IloNumVarArray(_env);
		for (int i = 0; i < _loadList.size(); i++)
		{
			string name = "U_L" + to_string(_loadList[i]->getId());
			_u_l.add(IloNumVar(_env, 0, 1, ILOFLOAT, name.c_str()));
		}
		_model.add(_u_l);

		_loadUseRateRng = IloRangeArray(_env);
		for (int i = 0; i < _loadList.size(); i++)
		{
			IloExpr expr(_env);
			expr -= _u_l[i] * _loadList[i]->getTotalNum();
			string name = "C13_loadUseRateRng _F" + to_string(_mountList[i]->getId());
			_loadUseRateRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			expr.end();
		}
		_model.add(_loadUseRateRng);
		cout << "#loadUseRateRng size:" << _loadUseRateRng.getSize() << endl;
		totalRngSize += _loadUseRateRng.getSize();

		_loadBaseFairRng = IloRangeArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _loadList.size(); j++)
			{
				IloExpr expr(_env);
				string name = "C14_loadBaseFairRng_B" + to_string(_baseList[i]->getId()) + "L_" + to_string(_loadList[j]->getId());
				expr -= _u_l[j];
				_loadBaseFairRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			}
		}
		_model.add(_loadBaseFairRng);
		cout << "#loadBaseFairRng size:" << _loadBaseFairRng.getSize() << endl;
		totalRngSize += _loadBaseFairRng.getSize();

		_overLoad_bl = IloNumVarArray(_env);
		_lessLoad_bl = IloNumVarArray(_env);
		for (int i = 0; i < _baseList.size(); i++)
		{
			for (int j = 0; j < _loadList.size(); j++)
			{
				IloNumColumn colOver(_env);
				colOver = _obj(ZZUtil::costLoadBaseFair);
				colOver += _loadBaseFairRng[i * _loadList.size() + j](-1);
				string name = "OverLoad_B" + to_string(_baseList[i]->getId()) + "_L" + to_string(_loadList[j]->getId());
				_overLoad_bl.add(IloNumVar(colOver, 0, 1, ILOFLOAT, name.c_str()));

				IloNumColumn colLess(_env);
				colLess = _obj(ZZUtil::costLoadBaseFair);
				colLess += _loadBaseFairRng[i * _loadList.size() + j](1);
				name = "LessLoad_B" + to_string(_baseList[i]->getId()) + "_L" + to_string(_loadList[j]->getId());
				_lessLoad_bl.add(IloNumVar(colLess, 0, 1, ILOFLOAT, name.c_str()));
			}
		}
	}
	*/
	
	/*由于计算时间过长，尝试注释*/
	//毁伤率约束
	//if (Util::ifEvaTargetAttackRobustness)
	//{
	//	/*先注释掉松弛变量部分，毁伤率要求完全达到*/
	//	//int tmpCount = 0;
	//	//_slackRb_t = IloNumVarArray(_env);
	//	//for (int i = 0; i < _targetList.size(); i++)
	//	//{
	//	//	vector<pair<double, string>> rateObjList = _targetList[i]->getRateObjList();
	//	//	for (int j = 0; j < rateObjList.size(); j++)
	//	//	{
	//	//		IloNumColumn col(_env);
	//	//		col += _obj(Util::costTarRobust);
	//	//		string name = "Slack_T" + to_string(tmpCount);
	//	//		_slackRb_t.add(IloNumVar(col, 0, 2, ILOFLOAT, name.c_str()));
	//	//		tmpCount++;
	//	//	}
	//	//}
	//	//_model.add(_slackRb_t);
	//	//tmpCount = 0;
	//	for (int i = 0; i < _targetList.size(); i++)
	//	{
	//		double rateObj = 0.95;
	//		IloExpr expr(_env);
	//		vector<TargetPlan*> planList = _targetList[i]->getType()->getTargetPlanList();
	//		for (int p = 0; p < planList.size(); p++)
	//		{
	//			expr += _o_tm[i][p] * planList[p]->getRate(_targetList[i]);//计划p下对目标i的毁伤率
	//		}
	//		//if (rateObjList[j].second == "Weak")//如果该毁伤率是弱毁伤率
	//		//{
	//		//	expr += _slackRb_t[tmpCount];//加上松弛变量
	//		//}
	//		string name = "C11_attackRobust_R_" + to_string(_targetList[i]->getId());
	//		//对每个目标而言，选用所有计划下的毁伤率之和要大于等于95%
	//		_attackRobustRng.add(IloRange(_env, rateObj, expr, IloInfinity, name.c_str()));
	//		//tmpCount++;
	//	}
	//	_model.add(_attackRobustRng);
	//	cout << "#attackRobustRng size:" << _attackRobustRng.getSize() << endl;
	//	totalRngSize += _attackRobustRng.getSize();
	//}
	
	/*不考虑公平性，注释*/
	/*if (Util::ifEvaMainTargetSlotFair)
	{
		_u_s = IloNumVarArray(_env);
		for (int i = 0; i < _mainTargetList.size(); i++)
		{
			string name = "U_S" + to_string(_mainTargetList[i]->getId());
			_u_s.add(IloNumVar(_env, 0, IloInfinity, ILOFLOAT, name.c_str()));
		}
		_model.add(_u_s);

		_mtSlotRateRng = IloRangeArray(_env);
		for (int i = 0; i < _mainTargetList.size(); i++)
		{
			IloExpr expr(_env);
			expr -= _u_s[i] * (int)_mainTargetList[i]->getTargetList().size();
			string name = "C24_maintTargetSlotRng _MT" + to_string(_mainTargetList[i]->getId());
			_mtSlotRateRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
			expr.end();
		}
		_model.add(_mtSlotRateRng);
		cout << "#mtSlotRateRng size:" << _mtSlotRateRng.getSize() << endl;
		totalRngSize += _mtSlotRateRng.getSize();

		
		_mtSlotFairRng = IloRangeArray(_env);
		for (int i = 0; i < _targetList.size(); i++)
		{
			IloExpr expr(_env);
			string name = "C25_mtSlotFairRng_T" + to_string(_targetList[i]->getId());
			expr -= _u_s[_targetList[i]->getMainTarget()->getId()];
			_mtSlotFairRng.add(IloRange(_env, 0, expr, 0, name.c_str()));
		}
		_model.add(_mtSlotFairRng);
		cout << "#mtSlotFairRng size:" << _mtSlotFairRng.getSize() << endl;
		totalRngSize += _mtSlotFairRng.getSize();

		_overMTSlot_mt = IloNumVarArray(_env);
		_lessMTSlot_mt = IloNumVarArray(_env);
		for (int i = 0; i < _targetList.size(); i++)
		{
			IloNumColumn colOver(_env);
			colOver = _obj(200);
			colOver += _mtSlotFairRng[i](-1);
			string name = "OverMTSlot_t" + to_string(_targetList[i]->getId());
			_overMTSlot_mt.add(IloNumVar(colOver, 0, IloInfinity, ILOFLOAT, name.c_str()));

			IloNumColumn colLess(_env);
			colLess = _obj(200);
			colLess += _mtSlotFairRng[i](1);
			name = "LessMTSlot_t" + to_string(_targetList[i]->getId());
			_lessMTSlot_mt.add(IloNumVar(colLess, 0, IloInfinity, ILOFLOAT, name.c_str()));
		}
	}*/

	/*不考虑演训经验*/
	//_trainFreqPreRng = IloRangeArray(_env);
	//for (int i = 0; i < _aftTrainPairList.size(); i++)
	//{
	//	string name = "C14_trainFreqPreRng_T" + to_string(_aftTrainPairList[i]->_target->getId()) + "B" + to_string(_aftTrainPairList[i]->_baseA->getId()) + "F" + to_string(_aftTrainPairList[i]->_fleetA->getId());
	//	_trainFreqPreRng.add(IloRange(_env, 0, IloInfinity, name.c_str()));

	//	name = "C14_trainFreqPreRng_T" + to_string(_aftTrainPairList[i]->_target->getId()) + "B" + to_string(_aftTrainPairList[i]->_baseB->getId()) + "F" + to_string(_aftTrainPairList[i]->_fleetB->getId());
	//	_trainFreqPreRng.add(IloRange(_env, 0, IloInfinity, name.c_str()));
	//}
	//_model.add(_trainFreqPreRng);
	//cout << "#trainFreqPreRng size:" << _trainFreqPreRng.getSize() << endl;
	//totalRngSize += _trainFreqPreRng.getSize();

	//_q_tbbff = IloNumVarArray(_env);
	//for (int i = 0; i < _aftTrainPairList.size(); i++)
	//{
	//	IloNumColumn col(_env);
	//	col = _obj(-1 * ZZUtil::costAftTrainPrefer * _aftTrainPairList[i]->_frequency);
	//	col += _trainFreqPreRng[i * 2](-1);
	//	col += _trainFreqPreRng[i * 2 + 1](-1);
	//	string name = "Q_T" + to_string(_targetList[i]->getId()) + "B" + to_string(_aftTrainPairList[i]->_baseA->getId()) + "B" + to_string(_aftTrainPairList[i]->_baseB->getId()) + "F" + to_string(_aftTrainPairList[i]->_fleetA->getId()) + "F" + to_string(_aftTrainPairList[i]->_fleetB->getId());
	//	_q_tbbff.add(IloNumVar(col, 0, 1, ILOFLOAT, name.c_str()));
	//}
	//_model.add(_q_tbbff);

	/*时间槽内的运力数量上限*/
	//for (int i = 0; i < _slotList.size(); i++)
	//{
	//	string name = "C15_slotRng_S" + to_string(_slotList[i]->getId());
	//	_slotRng.add(IloRange(_env, -IloInfinity, _slotList[i]->getCap(), name.c_str()));
	//}
	//_model.add(_slotRng);
	//cout << "#slotRng size:" << _slotRng.getSize() << endl;
	//totalRngSize += _slotRng.getSize();

	//主目标不是所有目标都有的，先搁置
	/*
	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		for (int j = 0; j < _slotList.size(); j++)
		{
			string name = "C16A_mtSlotRng_MT" + to_string(_mainTargetList[i]->getId()) + "_S" + to_string(_slotList[j]->getId());
			_mtSlotLbRng.add(IloRange(_env, -IloInfinity, 0, name.c_str()));
		}
	}
	_model.add(_mtSlotLbRng);
	cout << "#mtSlotRng size:" << _mtSlotLbRng.getSize() << endl;
	totalRngSize += _mtSlotLbRng.getSize();

	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		for (int j = 0; j < _slotList.size(); j++)
		{
			string name = "C16B_mtSlotRng_MT" + to_string(_mainTargetList[i]->getId()) + "_S" + to_string(_slotList[j]->getId());
			_mtSlotUbRng.add(IloRange(_env, 0, IloInfinity, name.c_str()));
		}
	}
	_model.add(_mtSlotUbRng);
	cout << "#mtSlotUbRng size:" << _mtSlotUbRng.getSize() << endl;
	totalRngSize += _mtSlotUbRng.getSize();

	for (int i = 0; i < _mtPrioPairList.size(); i++)
	{
		for (int j = 0; j < _slotList.size(); j++)
		{
			string name = "C21_mtPriorityRng_MT1" + to_string(_mtPrioPairList[i].first->getId()) + "_MT2" + to_string(_mtPrioPairList[i].second->getId());
			_mtPriorityCopyRng.add(IloRange(_env, 0, IloInfinity, name.c_str()));
		}
	}
	_model.add(_mtPriorityCopyRng);
	cout << "#mtPriorityCopyRng size:" << _mtPriorityCopyRng.getSize() << endl;
	totalRngSize += _mtPriorityCopyRng.getSize();

	_q_ts = NumVar2Matrix(_env, _mainTargetList.size());//主目标有无安排在时间槽内
	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		_q_ts[i] = IloNumVarArray(_env);
		for (int j = 0; j < _slotList.size(); j++)
		{
			string name = "Q_MT" + to_string(_mainTargetList[i]->getId()) + "_S" + to_string(_slotList[j]->getId());
			IloNumColumn col(_env);
			col += _obj(-10 * ((int)_mainTargetList.size() - _mainTargetList[i]->getId()));//主目标ID越靠前，这个值越小
			col += _mtSlotLbRng[i * _slotList.size() + j](-1 * (int)(_mainTargetList[i]->getTargetList().size()) * Util::groupPertargetNumLimit * 2);
			col += _mtSlotUbRng[i * _slotList.size() + j](-1);

			for (int q = 0; q < _mtPrioPairList.size(); q++)
			{
				if (_mainTargetList[i] == _mtPrioPairList[q].first)
				{
					col += _mtPriorityCopyRng[q * _slotList.size() + j](1);
				}
				else if (_mainTargetList[i] == _mtPrioPairList[q].second)
				{
					col += _mtPriorityCopyRng[q * _slotList.size() + j](-1);
				}
			}
			_q_ts[i].add(IloNumVar(col, 0, 1, ILOINT, name.c_str()));
		}
		_model.add(_q_ts[i]);
	}
	
	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		string name = "C22_mtCoverUbRng_MT" + to_string(_mainTargetList[i]->getId());
		_mtCoverUbRng.add(IloRange(_env, -IloInfinity, 0, name.c_str()));
	}
	_model.add(_mtCoverUbRng);
	cout << "#mtCoverUbRng size:" << _mtCoverUbRng.getSize() << endl;
	totalRngSize += _mtCoverUbRng.getSize();

	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		string name = "C22_mtCoverLbRng_MT" + to_string(_mainTargetList[i]->getId());
		_mtCoverLbRng.add(IloRange(_env, (int)(1 - _mainTargetList[i]->getTargetList().size()), IloInfinity, name.c_str()));
	}
	_model.add(_mtCoverLbRng);
	cout << "#mtCoverLbRng size:" << _mtCoverLbRng.getSize() << endl;
	totalRngSize += _mtCoverLbRng.getSize();

	_q_c = IloNumVarArray(_env);
	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		string name = "Q_C" + to_string(_mainTargetList[i]->getId());
		IloNumColumn col(_env);
		for (int j = 0; j < _slotList.size(); j++)
		{
			col += _mtSlotUbRng[i * _slotList.size() + j](1);
		}

		col += _mtCoverUbRng[i]((int)(_mainTargetList[i]->getTargetList().size()));
		col += _mtCoverLbRng[i]((int)(_mainTargetList[i]->getTargetList().size()));

		_q_c.add(IloNumVar(col, 0, IloInfinity, ILOBOOL, name.c_str()));
	}
	_model.add(_q_c);
	*/
	//目标未覆盖变量
	_z_t = IloNumVarArray(_env);
	for (int t = 0; t < _targetList.size(); t++)
	{
		IloNumColumn col(_env);
		double costUncover = Util::costTargetUncover;
		if (Util::ifEvaTargetPriority)
		{
			costUncover += Util::costTargetUncover * (sqrt(Util::maxPriority) - sqrt(_targetList[t]->getPriority())) / sqrt(Util::maxPriority);
		}
		costUncover += (_targetList[t]->getPriority()) * Util::costTargetUncoverByOrder;//目标优先级越高，未覆盖的惩罚越大
		//如果在新增目标集合中找不到该目标
		if (_addedTargetSet.find(_targetList[t]) == _addedTargetSet.end())
		{
			costUncover = 200000;
		}
		col = _obj(costUncover);//设置这个变量在目标函数中的系数目标未覆盖的惩罚系数
		col += _dmdAsgRng[_targetList[t]->getId()](1);
		////相比ResModel，新增_mtCoverLbRng和_mtCoverUbRng
		//col += _mtCoverLbRng[_targetList[t]->getMainTarget()->getId()](-1);
		//col += _mtCoverUbRng[_targetList[t]->getMainTarget()->getId()](-1);

		string name = "Z_T" + to_string(_targetList[t]->getId());
		int ub = 1;
		//if (t < 40)
		//{
		//	ub = 0;
		//}
		_z_t.add(IloNumVar(col, 0, ub, ILOFLOAT, name.c_str()));
	}
	_model.add(_z_t);

	//与slot相关的约束,注释
	/*for (int i = 0; i < _mountTypeList.size(); i++)
	{
		for (int j = 0; j < _slotList.size(); j++)
		{
			string name = "C23_regionGrpUbRng_MountType" + to_string(_mountTypeList[i]->getId()) + "_S" + to_string(_slotList[j]->getId());
			_regionGrpUbRng.add(IloRange(_env, -IloInfinity, _regionGrpCapMap[_slotList[j]][_mountTypeList[i]], name.c_str()));
		}
	}
	_model.add(_regionGrpUbRng);
	cout << "#regionGrpUbRng size:" << _regionGrpUbRng.getSize() << endl;
	totalRngSize += _regionGrpUbRng.getSize();*/

	cout << "#Rng size:" << totalRngSize << endl;
}

void Model::addColsByGrp(vector<Tour* > tourList)
{
	cout << "#count:" << _colGenCount << endl;
	cout << "addCols:" << tourList.size() << endl;
	_tourList.clear();
	for (int i = 0; i < tourList.size(); i++)
	{
		tourList[i]->setId(_tourList.size());
		_tourList.push_back(tourList[i]);
	}
	// 存储每个资源g的变量，用于后续添加约束
	map<int, vector<IloNumVar>> fleetToVarsMap;  // fleetId -> 对应的变量列表

	for (int i = 0; i < tourList.size(); i++)
	{
		IloNumColumn col(_env);
		int cost = rand() % (50 - 1 + 1) + 1;
		col = _obj(tourList[i]->getCost());
		double maxtourtime= getTourTime();
		double tourtime = tourList[i]->getTotalTime();
		if (tourtime > maxtourtime) {
			setTourTime(tourtime);
		}
		//col += _assignRng[tourList[i]->getFleet()->getId()](1);
		vector<OperTarget* >operTargetList = tourList[i]->getOperTargetList();
		//mountLoad以对的形式约束
		for (int j = 0; j < operTargetList.size(); j++)
		{
			vector<pair<MountLoad*, int>> mountLoadList = operTargetList[j]->getMountLoadPlan();
			for (int p = 0; p < mountLoadList.size(); p++)
			{
				int pairIndex = mountLoadList[p].first->getId();
				int num = mountLoadList[p].second;
				col += _pairDmdRng[operTargetList[j]->getTarget()->getId() * _mountLoadList.size() + pairIndex](num);
			}
		}

		//int baseIndex = tourList[i]->getGroup()->getBase()->getId();
		map<Load*, int> loadMap = tourList[i]->getLoadMap();
		map<Load*, int >::iterator loadIter;
		map<Mount*, int> mountMap = tourList[i]->getMountMap();
		map<Mount*, int>::iterator mountIter;
		for (loadIter = loadMap.begin(); loadIter != loadMap.end(); loadIter++)
		{
			int index = loadIter->first->getId();
			int num = loadIter->second;
			col += _uCapRng[index](num);//载荷容量约束中的系数设置为计划中的载荷num
		}
		//for (loadIter = loadMap.begin(); loadIter != loadMap.end(); loadIter++)
		//{
		//	int index = loadIter->first->getId();
		//	int num = loadIter->second;
		//	col += _uCapRng[baseIndex * _loadList.size() + index](num);
		//}
		for (mountIter = mountMap.begin(); mountIter != mountMap.end(); mountIter++)
		{
			int index = mountIter->first->getId();
			int num = mountIter->second;
			col += _wCapRng[index](num);//挂载容量约束中的系数设置为num
		}
		/*由于计算时间过长，尝试注释*/
		////_tgAsgNumRng每个目标分配的飞机数量
		//for (int j = 0; j < operTargetList.size(); j++)
		//{
		//	col += _tgAsgNumRng[operTargetList[j]->getTarget()->getId()](1);//单目标最大运力数量的约束中，找得到这个operTargetList中经过的目标，给飞机数量变量设置系数为1
		//}

		string name = "y_g" + to_string(tourList[i]->getFleet()->getId()) + "_r" + to_string(tourList[i]->getId());
		IloNumVar var = IloNumVar(col, 0, 1, ILOFLOAT, name.c_str());;//变量数同tourlist的长度，设置变量的上下界
		_y_gr.add(var);
		_model.add(var);
		col.end();


		// 记录变量到对应的资源
		int fleetId = tourList[i]->getFleet()->getId();
		fleetToVarsMap[fleetId].push_back(var);

		// 添加约束：每个资源g只能选择一种Tour（r）
		addFleetSelectionConstraints(fleetToVarsMap);

	}
	cout << "AddCols Dur is " << (clock() - _startClock) / 1000 << "s." << endl;
}

void Model::addFleetSelectionConstraints(const map<int, vector<IloNumVar>>& fleetToVarsMap)
{
	for (const auto& [fleetId, vars] : fleetToVarsMap)
	{
		if (vars.size() > 1)  // 如果该资源有多个方案可选
		{
			IloExpr sumExpr(_env);
			for (const auto& var : vars)
			{
				sumExpr += var;
			}

			// 约束：该资源的所有方案之和 ≤ 1
			string constrName = "FleetSelect_g" + to_string(fleetId);
			IloRange constraint = IloRange(_env, 0, sumExpr, 1, constrName.c_str());
			_model.add(constraint);
			sumExpr.end();

			//cout << "添加资源选择约束: fleetId=" << fleetId
			//	<< ", 可选方案数=" << vars.size() << endl;
		}
	}
}
/*因为tour是没有Group的，所以这种方式不适合，注释掉*/
/*
void Model::addColsByGrpType(vector<Tour* > tourList)
{
	_solver.clear();
	cout << "#count:" << _colGenCount << endl;
	cout << "addCols:" << tourList.size() << endl;
	_tourList.clear();
	for (int i = 0; i < tourList.size(); i++)
	{
		tourList[i]->setId(_tourList.size());
		_tourList.push_back(tourList[i]);
	}
	init();
	for (int i = 0; i < tourList.size(); i++)
	{
		IloNumColumn col(_env);
		col = _obj(tourList[i]->getCost());
		vector<OperTarget* >operTargetList = tourList[i]->getOperTargetList();

		for (int j = 0; j < operTargetList.size(); j++)
		{
			vector<pair<MountLoad*, int>> mountLoadList = operTargetList[j]->getMountLoadPlan();
			for (int p = 0; p < mountLoadList.size(); p++)
			{
				int pairIndex = mountLoadList[p].first->getId();
				int num = mountLoadList[p].second;
				col += _pairDmdRng[operTargetList[j]->getTarget()->getId() * _mountLoadList.size() + pairIndex](num);
			}
		}

		//int baseIndex = tourList[i]->getGrpType()->getBase()->getId();
		map<Load*, int> loadMap = tourList[i]->getLoadMap();
		map<Load*, int >::iterator loadIter;
		map<Mount*, int> mountMap = tourList[i]->getMountMap();
		map<Mount*, int>::iterator mountIter;

		for (loadIter = loadMap.begin(); loadIter != loadMap.end(); loadIter++)
		{
			int index = loadIter->first->getId();
			int num = loadIter->second;
			col += _uCapRng[_loadList.size() + index](num);
		}
		for (mountIter = mountMap.begin(); mountIter != mountMap.end(); mountIter++)
		{
			int index = mountIter->first->getId();
			int num = mountIter->second;
			col += _wCapRng[_mountList.size() + index](num);
		}

		Fleet* fleet = tourList[i]->getFleet();
		int aftNum = tourList[i]->getGrpType()->getAftNum();
		col += _fCapRng[_fleetList.size() + fleet->getId()](aftNum);

		for (int j = 0; j < operTargetList.size(); j++)
		{
			col += _tgAsgNumRng[operTargetList[j]->getTarget()->getId()](1);
		}

		if (ZZUtil::ifEvaBaseAftUseFair)
		{
			col += _aftUseRateRng[fleet->getId()](tourList[i]->getGrpType()->getAftNum());
			col += _aftBaseFairRng[baseIndex * _fleetList.size() + fleet->getId()]((double)aftNum / (double)tourList[i]->getGrpType()->getBase()->getFleetNum(fleet));
			col += _aftBaseLbRng[baseIndex * _fleetList.size() + fleet->getId()]((double)aftNum / (double)tourList[i]->getGrpType()->getBase()->getFleetNum(fleet));
		}

		if (ZZUtil::ifEvaBaseFleetTypeUseFair)
		{
			col += _aftFUseRateRng[fleet->getType()->getId()](tourList[i]->getGrpType()->getAftNum());
			col += _aftFBaseFairRng[baseIndex * _fleetTypeList.size() + fleet->getType()->getId()]((double)aftNum / (double)tourList[i]->getGrpType()->getBase()->getFleetTypeAftNum(fleet->getType()));
			col += _aftFBaseLbRng[baseIndex * _fleetTypeList.size() + fleet->getType()->getId()]((double)aftNum / (double)tourList[i]->getGrpType()->getBase()->getFleetTypeAftNum(fleet->getType()));
		}

		if (ZZUtil::ifEvaBaseMountUseFair)
		{
			for (mountIter = mountMap.begin(); mountIter != mountMap.end(); mountIter++)
			{
				ZZMount* mount = (*mountIter).first;
				int num = (*mountIter).second;
				col += _mountUseRateRng[mount->getId()](num);
				col += _mountBaseFairRng[baseIndex * _mountList.size() + mount->getId()]((double)num / (double)tourList[i]->getGrpType()->getBase()->getMountNum(mount));
				col += _mountBaseLbRng[baseIndex * _mountList.size() + mount->getId()]((double)num / (double)tourList[i]->getGrpType()->getBase()->getMountNum(mount));
			}
		}

		if (ZZUtil::ifEvaBaseLoadUseFair)
		{
			for (loadIter = loadMap.begin(); loadIter != loadMap.end(); loadIter++)
			{
				ZZLoad* load = (*loadIter).first;
				int num = (*loadIter).second;
				col += _loadUseRateRng[load->getId()](num);
				col += _loadBaseFairRng[baseIndex * _loadList.size() + load->getId()]((double)num / (double)tourList[i]->getGrpType()->getBase()->getLoadNum(load));
				col += _loadBaseLbRng[baseIndex * _loadList.size() + load->getId()]((double)num / (double)tourList[i]->getGrpType()->getBase()->getLoadNum(load));
			}
		}

		if (ZZUtil::ifEvaMainTargetSlotFair)
		{
			for (int j = 0; j < operTargetList.size(); j++)
			{
				int earliestSlotInd = operTargetList[j]->getEarliestSlotInd();
				col += _mtSlotFairRng[operTargetList[j]->getTarget()->getId()](earliestSlotInd);
				col += _mtSlotRateRng[operTargetList[j]->getTarget()->getMainTarget()->getId()](earliestSlotInd);
			}
		}
		for (int k = 0; k < operTargetList.size(); k++)
		{
			for (int j = 0; j < _aftTrainPairList.size(); j++)
			{
				ZZBase* baseA = _aftTrainPairList[j]->_baseA;
				ZZBase* baseB = _aftTrainPairList[j]->_baseB;
				ZZFleet* fleetA = _aftTrainPairList[j]->_fleetA;
				ZZFleet* fleetB = _aftTrainPairList[j]->_fleetB;
				ZZTarget* target = operTargetList[k]->getTarget();
				if (_aftTrainPairList[j]->_target == target)
				{
					if (baseA == tourList[i]->getGrpType()->getBase() && fleet == fleetA)
					{
						col += _trainFreqPreRng[2 * j](1);
					}
					if (baseB == tourList[i]->getGrpType()->getBase() && fleet == fleetB)
					{
						col += _trainFreqPreRng[2 * j + 1](1);
					}
				}
			}
		}

		map<Slot*, int> timeMap = tourList[i]->getTimeMap();
		map<Slot*, int>::iterator timeIter = timeMap.begin();
		for (timeIter; timeIter != timeMap.end(); timeIter++)
		{
			col += _slotRng[(*timeIter).first->getId()]((*timeIter).second);
		}

		map<MainTarget*, int> mainTargetEarlyTimeMap;
		int minEarlyTime = _slotList.size();
		for (int k = 0; k < operTargetList.size(); k++)
		{
			if (mainTargetEarlyTimeMap.find(operTargetList[k]->getTarget()->getMainTarget()) != mainTargetEarlyTimeMap.end())
			{
				if (mainTargetEarlyTimeMap[operTargetList[k]->getTarget()->getMainTarget()] > operTargetList[k]->getEarliestSlotInd())
				{
					mainTargetEarlyTimeMap[operTargetList[k]->getTarget()->getMainTarget()] = operTargetList[k]->getEarliestSlotInd();
				}
			}
			else
			{
				mainTargetEarlyTimeMap[operTargetList[k]->getTarget()->getMainTarget()] = operTargetList[k]->getEarliestSlotInd();
			}
			if (minEarlyTime > operTargetList[k]->getEarliestSlotInd())
			{
				minEarlyTime = operTargetList[k]->getEarliestSlotInd();
			}
		}

		map<MainTarget*, int>::iterator iterMTT = mainTargetEarlyTimeMap.begin();
		for (iterMTT = mainTargetEarlyTimeMap.begin(); iterMTT != mainTargetEarlyTimeMap.end(); iterMTT++)
		{
			MainTarget* mainTarget = (*iterMTT).first;
			for (int k = 0; k < _slotList.size(); k++)
			{
				if (k >= (*iterMTT).second)
				{
					col += _mtSlotLbRng[mainTarget->getId() * _slotList.size() + _slotList[k]->getId()](1);
					col += _mtSlotUbRng[mainTarget->getId() * _slotList.size() + _slotList[k]->getId()](1);
				}
			}
		}

		Mount* mount = operTargetList[0]->getMountLoadPlan()[0].first->_mount;
		col += _regionGrpUbRng[mount->getMountType()->getId() * _slotList.size() + minEarlyTime](1);

		string name = "x_gt" + to_string(tourList[i]->getGrpType()->getId()) + "_r" + to_string(tourList[i]->getId());
		int lb = 0;
		IloNumVar var = IloNumVar(col, lb, IloInfinity, ILOFLOAT, name.c_str());
		_x_gtr.add(var);
		_model.add(var);
		col.end();
	}
	for (int i = 0; i < _x_gtr.getSize(); i++)
	{
		if (_tourList[i]->isFixed())
		{
			if (_tourList[i]->getValue() == 0)
			{
				cout << "check? " << endl;
				_tourList[i]->print();
				exit(0);
			}
			_x_gtr[i].setUB(_tourList[i]->getValue());
			_x_gtr[i].setLB(_tourList[i]->getValue());
		}
		else
		{
			_x_gtr[i].setUB(IloInfinity);
			_x_gtr[i].setLB(0);
		}
	}
	cout << "AddCols Dur is " << (clock() - _startClock) / 1000 << "s." << endl;
}
*/
void Model::solveLP()
{
	string name = Util::OUTPUTPATH + "m" + to_string(_colGenCount++) + ".lp";
	cout << "Col Gen Problem " << _colGenCount << endl;
	_solver.setParam(IloCplex::RootAlg, IloCplex::Barrier);
	_solver.setParam(IloCplex::EpGap, 0);

	_solver.solve();

	cout << "Solution status: " << _solver.getStatus() << endl;
	if (_solver.getStatus() == IloAlgorithm::Infeasible)
	{
		cout << "Exit with infeasible soln!" << endl;
		exit(0);
	}

	cout << "Optimal value: " << _solver.getObjValue() << endl;
	cout << "Solution status: " << _solver.getStatus() << endl;
}

void Model::solveIP()
{
	//for (int i = 0; i < _q_ts.getSize(); i++)
	//{
	//	_convertIP1 = IloConversion(_env, _q_ts[i], ILOINT);//将q_ts从连续变量到整数变量的转换
	//}
	for (int i = 0; i < _o_tm.getSize(); i++)
	{
		_convertIP2 = IloConversion(_env, _o_tm[i], ILOINT);
	}
	_convertIP = IloConversion(_env, _y_gr, ILOINT);//IloConversion将x_gr的变量类型转换为布尔型
	//_convertIP = IloConversion(_env, _x_gtr, ILOINT);

	_model.add(_convertIP);
	//_model.add(_convertIP1);
	_model.add(_convertIP2);
	//// 使用时
	//if (Util::maxThreadNumLimit > 0 && Util::isValidThreadCount(Util::maxThreadNumLimit)) {
	//	try {
	//		_solver.setParam(IloCplex::Threads, Util::maxThreadNumLimit);
	//	}
	//	catch (IloException& e) {
	//		cerr << "CPLEX Exception when setting threads: " << e << endl;
	//		cerr << "Message: " << e.getMessage() << endl;
	//		// 回退到默认设置
	//		_solver.setParam(IloCplex::Threads, 0);
	//	}
	//}
	string timeStr = to_string(clock() / 60);
	string name = Util::OUTPUTPATH + "solveIP" + to_string(_colGenCount++) + timeStr + ".lp";
	cout << "output: " << name << endl;
	//_solver.exportModel(name.c_str());

	_solver.setParam(IloCplex::TiLim, 360);
	_solver.setParam(IloCplex::EpGap, Util::IPGap);

	//_solver.solve();
	try {
		if (_solver.solve()) {
			// 处理解
		}
	}
	catch (const IloAlgorithm::Exception& e) {
		std::cerr << "CPLEX Exception: " << e.getMessage() << std::endl;
		// 获取更详细的错误信息
		if (_solver.getStatus() == IloAlgorithm::Infeasible) {
			std::cerr << "Model is infeasible" << std::endl;
		}
	}

	cout << "Solution status: " << _solver.getStatus() << endl;
	if (_solver.getStatus() == IloAlgorithm::Infeasible)
	{
		exit(0);
	}
	cout << "Optimal Value: " << _solver.getObjValue() << endl;

	double cancelTargetNum = 0.0;
	vector<Target*> uncoverTList;
	vector<Target*> coverTList;
	for (int i = 0; i < _targetList.size(); i++)
	{
		if (_solver.getValue(_z_t[i]) > EPSILON)
		{
			_targetList[i]->setLpVal(_solver.getValue(_z_t[i]));
			cout << _z_t[i].getName() << " = " << _solver.getValue(_z_t[i]) << ", ";
			_targetList[i]->print();
			uncoverTList.push_back(_targetList[i]);
			cancelTargetNum += _solver.getValue(_z_t[i]);
		}
		else
		{

			coverTList.push_back(_targetList[i]);
			cout << _z_t[i].getName() << " = " << _solver.getValue(_z_t[i]) << ", ";
			_targetList[i]->print();
		}
	}
	_schedule->setUncoverTList(uncoverTList);//设置本轮次的未覆盖目标集合
	_schedule->setCoverTList(coverTList);//设置本轮次的已覆盖目标集合
	cout << "Total cancel target num is " << cancelTargetNum << endl;
}

void Model::solveDiving()
{
	init();
	time_t bgnTime = clock();
	vector<Tour*> totalSolnTourList = _schedule->getTourList();
	int fixTargetNum = 0;
	int preFixTargetNum = -1;
	addColsByGrp(totalSolnTourList);

	while (fixTargetNum != preFixTargetNum)
	{
		solveLP();
		preFixTargetNum = fixTargetNum;
		fixTargetNum += setSolnLP();
	}

	assignTimeForSubTargets();
}

// 主求解函数：采用分层求解策略处理大规模优化问题
void Model::solve()
{
	// 1. 初始化模型（创建变量、约束、目标函数等）
	init();
	// 记录求解开始时间，用于性能监控
	time_t bgnTime = clock();
	// 获取所有可能的任务方案（Tour）列表
	vector<Tour*> totalSolnTourList = _schedule->getTourList();
	// 用于存储分批后的目标集合列表
	vector<set<Target*>> targetSetList;
	// 设置初始分批范围，默认为所有目标
	int splitRange = _schedule->getTargetList().size();

	// 2. 根据问题规模动态调整求解参数
	double splitNum = 1;
	// 如果方案数量很大且目标集很小，调整求解精度
	if (totalSolnTourList.size() > 25000 && targetSetList.size() <= 1) {
		splitNum = 1;
		targetSetList.clear();
		Util::IPGap = 0.05;  // 放宽整数规划的间隙容忍度
	}

	// 3. 根据目标数量决定是否进行分批处理
	// 如果目标数量较多（>60），进行问题分割
	if (_targetList.size() > 60) {
		splitRange = ceil(_schedule->getTargetList().size() / splitNum);
	}

	// 计算需要迭代的批次数
	int splitGrt = ceil((double)_schedule->getTargetList().size() / (double)splitRange);
	//cout << "Num of Iter is " << splitGrt << " splitRange " << splitRange
	//	<< " mainTargetNum is " << _mainTargetList.size() << "分批规模 " << splitNum << endl;

	//// 打印主目标信息用于调试
	//for (int i = 0; i < _mainTargetList.size(); i++) {
	//	cout << i << " " << _mainTargetList[i]->getName() << " "
	//		<< _mainTargetList[i]->getBatchId() << " "
	//		<< _mainTargetList[i]->getLatestBgnTime() << endl;
	//}

	// 4. 创建分批的目标集合
	set<string> preBatchIdSet;  // 记录前一批次的批次ID，用于避免重复
	for (int i = 0; i < splitGrt; i++) {
		cout << "Processing batch: " << i << endl;

		// 计算当前批次的起始和结束索引（采用80%重叠）
		int bgnId = i * ((int)splitRange * 0.8);    // 起始索引，80%重叠
		int endId = bgnId + splitRange;              // 结束索引

		// 如果是最后一批，调整结束索引到主目标列表末尾
		if (i == splitGrt - 1) {
			endId = _mainTargetList.size();
		}

		// 创建当前批次的目标集合和批次ID集合
		set<Target*> targetSet;      // 当前批次的目标集合
		set<string> batchIdSet;      // 当前批次的批次ID集合

		// 将当前批次的主目标下的子目标加入到目标集合中
		for (int j = bgnId; j < endId; j++) {
			vector<Target*> targetList = _mainTargetList[j]->getTargetList();
			targetSet.insert(begin(targetList), end(targetList));//把子目标targetList从头到尾加入到targetSet中，//begin（v） 等价于 v.begin()

			// 记录新出现的批次ID
			if (preBatchIdSet.find(_mainTargetList[j]->getBatchId()) == preBatchIdSet.end()) {
				batchIdSet.insert(_mainTargetList[j]->getBatchId());
			}
		}
		// 更新前一批次ID集合为当前批次ID集合
		preBatchIdSet = batchIdSet;
		// 将当前批次目标集合加入到总列表中
		targetSetList.push_back(targetSet);

		// 打印当前批次的目标信息（调试用）
		/*set<Target*>::iterator iterSubT = targetSet.begin();
		for (iterSubT; iterSubT != targetSet.end(); iterSubT++) {
			(*iterSubT)->print();
		}*/
	}
	////根据目标类型划分批次
	//targetSetList.clear();
	//vector<TargetType*>targetTypeList=_schedule->getTargetTypeList();
	//for (int i = 0; i < targetTypeList.size(); i++) {
	//	vector<Target*> targetList = targetTypeList[i]->getTargetList();
	//	set<Target*> targetSet(targetList.begin(), targetList.end());
	//	targetSetList.push_back(targetSet);
	//}
	//(一)分批求解每个子问题
	for (int i = 0; i < targetSetList.size(); i++) {
		set<Target*> targetSet = targetSetList[i];
		// 将当前批次目标添加到已处理目标集合中
		_addedTargetSet.insert(targetSet.begin(), targetSet.end());
		vector<Tour*> tourList;  // 当前批次的任务方案列表

		// 设置当前批次主目标的最早开始时间
		for (int j = 0; j < _mainTargetList.size(); j++) {
			vector<Target*> targetList = _mainTargetList[j]->getTargetList();
			for (int q = 0; q < targetList.size(); q++) {
				if (targetSet.find(targetList[q]) != targetSet.end()) {
					_mainTargetList[j]->setSolnEarliestBgnTime(0);  // 重置最早开始时间
					break;
				}
			}
		}

		//5. 筛选适合当前批次的任务方案
		for (int j = 0; j < totalSolnTourList.size(); j++) {
			// 如果方案已被访问且被固定，直接加入当前批次,totalSolnTourList[j]表示某个tour
			if (totalSolnTourList[j]->isVisit() && totalSolnTourList[j]->isFixed()) {//isfixed表示已经设置完路线
			//if (totalSolnTourList[j]->isFixed()) {//isfixed表示已经设置完路线
				tourList.push_back(totalSolnTourList[j]);
				vector<OperTarget*> operTargetList = totalSolnTourList[j]->getOperTargetList();
				for (int q = 0; q < operTargetList.size(); q++) {
					if (targetSet.find(operTargetList[q]->getTarget()) != targetSet.end()) {//如果targetSet中不包含某个目标
						totalSolnTourList[j]->setFixed(false);  // 取消这个tour的固定状态
					}
				}
			}
			else
			{
				// 如果方案有当前批次的目标，就加入
				vector<OperTarget*> operTargetList = totalSolnTourList[j]->getOperTargetList();
				bool ifAllFind = true;
				for (int q = 0; q < operTargetList.size(); q++) 
				{
					if (targetSet.find(operTargetList[q]->getTarget()) == targetSet.end()) {
						ifAllFind = false;
					}
				}
				// 如果方案完全覆盖当前批次，加入方案列表
				if (ifAllFind) {
					tourList.push_back(totalSolnTourList[j]);
					totalSolnTourList[j]->setFixed(false);  // 取消固定状态
				}
			}
		}

		// 打印当前批次信息
		cout << "**********" << "Iter " << i << " MaintargetNum: "
			<< targetSet.size() << " tourNum: " << tourList.size() << "************" << endl;
		cout << "start solve Model running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

		// 6. 求解当前批次的子问题
		addColsByGrp(tourList);  // 将方案添加到模型
		//目标函数添加最大tour时间
		double tourtime = getTourTime();
		_obj.setConstant(tourtime* Util::weightTourTime);
		solveIP();               // 求解整数规划问题
		cout << "solve running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
		
		//当前批次有未完成目标
		
		while (_schedule->getUncoverTList().size() > 0) {
			//setSolnInforSub();     // 保存子问题解信息
			setSolnInfor();  // 保存最终解信息
			cout << "set running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

			// 8. 清理模型，为下一批求解做准备
			_model.remove(_convertIP);
			//_model.remove(_convertIP1);
			_model.remove(_convertIP2);
			vector<Target*> uncoverTList = _schedule->getUncoverTList();
			set<Target*> uncoverTSet;
			uncoverTSet.insert(begin(uncoverTList), end(uncoverTList));
			vector<Tour*> tourList;

			// 筛选未覆盖目标的方案
			vector<Tour*>totalSolnTourListNew;
			for (int j = 0; j < totalSolnTourList.size(); j++) {
				//if (totalSolnTourList[j]->isVisit() && totalSolnTourList[j]->isFixed()){
				Tour* original_tour= totalSolnTourList[j];
				Fleet* original_fleet = original_tour->getFleet();
				FleetType* fleetType = original_fleet->getType();
				int sameTypeNum = _schedule->getFleetTypeTourNum(fleetType);
				vector<Fleet*> fleetList = fleetType->getFleetList();
				bool finded = false;
				for (int k = 0; k < fleetList.size(); k++) {
					if (fleetList[k] == original_fleet) {
						cout << "找到当前fleet,索引为:" << k << "，共有资源 ：" << fleetList.size() << "， " << sameTypeNum <<"名称为"<< fleetType->getName()<< endl;
						//更新资源，选择当前资源同类型资源的下一个资源作为同一个tour的fleet
						int idx = ((k + sameTypeNum) % fleetList.size());
						Fleet* newFleet = fleetList[idx];
						cout<<"下一个资源索引为"<<idx <<endl;
						// 创建新的Tour对象，拷贝内容
						Tour* new_tour = new Tour(*original_tour);  // 需要实现拷贝构造函数
						new_tour->setFleet(newFleet);
						totalSolnTourListNew.push_back(new_tour);
						finded= true;
						break;
					}
				}
				if (!finded) {
					cout << "未找到当前fleet，总资源数量：" << fleetList.size() << " " << sameTypeNum << "名称为" << fleetType->getName() << endl;
				}
				//if (!finded) {
				//	cout << "未找到当前fleet:" << fleetList.size() << " " << sameTypeNum << endl;
				//	}
			}
			tourList.clear();
			cout << "totalSolnTourList原始长度为" << totalSolnTourList.size() << endl;
            totalSolnTourList.clear();
            totalSolnTourList = totalSolnTourListNew;
			bool found = false;
			cout<<"totalSolnTourList长度为"<<totalSolnTourList.size()<<endl;
			for (int j = 0; j < totalSolnTourList.size(); j++) {
				// 如果方案已被访问且被固定，直接加入可选方案列表
				if (totalSolnTourList[j]->isVisit() && totalSolnTourList[j]->isFixed()) {
					tourList.push_back(totalSolnTourList[j]);
					vector<OperTarget*> operTargetList = totalSolnTourList[j]->getOperTargetList();
					totalSolnTourList[j]->setFixed(false);  // 取消这个tour的已选择状态
				}
				else {
					vector<OperTarget*> operTargetList = totalSolnTourList[j]->getOperTargetList();
					for (int q = 0; q < operTargetList.size(); q++) {
						if (operTargetList[q]->getTarget()->getName() == "target11"){
							cout << "发现目标11，类型为mount3" << endl;
							found = true;
						}

						if (uncoverTSet.find(operTargetList[q]->getTarget()) != uncoverTSet.end()) {//如果tour中包含未覆盖目标，就把这个tour放入可选tourlist中
							tourList.push_back(totalSolnTourList[j]);
							totalSolnTourList[j]->setFixed(false);  // 取消这个tour的已选择状态
							break;
						}
					}
				}
			}
			if (!found) {
				cout << "未发现目标11" << endl;
			}
			cout << "choose tours time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
			cout<<"本轮次加入的tourList长度为"<<tourList.size()<<endl;
			if (tourList.size() == 0) {
  				cout << "本轮次没有可选tour" << endl;
                break;
			}
			//打印tour信息
			for (int j = 0; j < tourList.size(); j++) {
				tourList[j]->print();
			}
			// 求解未覆盖目标的子问题
			addColsByGrp(tourList);
			//目标函数添加最大tour时间
			_obj.setConstant(getTourTime()*1000);
			solveIP();
			cout << "solve running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

			//setSolnInfor();  // 保存最终解信息
		 }
			//setSolnInforSub();    // 保存完整解信息
			////  清理模型，为下一批求解做准备
			//_model.remove(_convertIP);
			////_model.remove(_convertIP1);
			//_model.remove(_convertIP2);
			//addColsByGrp(tourList);  // 将方案添加到模型
			//solveIP();               // 求解整数规划问题
		

		// 7. 保存求解结果
		if (i == targetSetList.size() - 1) {
			// 如果是最后一批且没有未覆盖目标，保存完整解
			if (_schedule->getUncoverTList().size() == 0) {
				setSolnInfor();    // 保存完整解信息
			}
			// 如果是最后一批且有未覆盖目标，保存子问题解信息
			else {
				setSolnInforSub(); // 保存子问题解信息
			}
		}
		else {
			setSolnInforSub();     // 保存子问题解信息
		}

		// 8. 清理模型，为下一批求解做准备
		_model.remove(_convertIP);
		//_model.remove(_convertIP1);
		_model.remove(_convertIP2);
	}

	// 9. 处理未覆盖的目标（如果有）
	if (_schedule->getUncoverTList().size() > 0) {
		vector<Target*> uncoverTList = _schedule->getUncoverTList();
		set<Target*> uncoverTSet;
		uncoverTSet.insert(begin(uncoverTList), end(uncoverTList));
		vector<Tour*> tourList;

		// 筛选未覆盖目标的方案
		for (int j = 0; j < totalSolnTourList.size(); j++) {
			//更新资源，选择当前资源同类型资源的下一个资源作为同一个tour的fleet
			Tour* tour = totalSolnTourList[j];
			Fleet* fleet = tour->getFleet();
			vector<Fleet*>fleetList = fleet->getFleetType()->getFleetList();
			for (int k = 0; k < fleetList.size() - 1; k++) {
				if (fleetList[k]->getId() == fleet->getId()) {
					Fleet* newFleet = fleetList[k + 1];
					tour->setFleet(newFleet);
				}
			}
			
			// 如果方案已被访问且被固定，直接加入可选方案列表
			if (totalSolnTourList[j]->isVisit() && totalSolnTourList[j]->isFixed()) {
				tourList.push_back(totalSolnTourList[j]);
				vector<OperTarget*> operTargetList = totalSolnTourList[j]->getOperTargetList();
				totalSolnTourList[j]->setFixed(false);  // 取消这个tour的已选择状态
			}
			else {
				vector<OperTarget*> operTargetList = totalSolnTourList[j]->getOperTargetList();
				for (int q = 0; q < operTargetList.size(); q++) {
					if (uncoverTSet.find(operTargetList[q]->getTarget()) != uncoverTSet.end()) {//如果tour中包含未覆盖目标，就把这个tour放入可选tourlist中
						tourList.push_back(totalSolnTourList[j]);
						totalSolnTourList[j]->setFixed(false);  // 取消这个tour的已选择状态
						break;
					}
				}
			}
		}

		// 注释掉的备份方案处理代码
		/*
		// 原有的备份方案处理逻辑
		vector<ZZTour*> backUpTourList = _schedule->getBackUpTourList();
		for (int j = 0; j < backUpTourList.size(); j++) {
			// ... 备份方案处理代码
		}
		*/

		// 求解未覆盖目标的子问题
		addColsByGrp(tourList);
		solveIP();
		setSolnInfor();  // 保存最终解信息
	}
	// 10. 最终处理阶段
	//assignTimeForSubTargets();  // 为所有子目标分配具体执行时间
	_addedTargetSet.clear();    // 清空已处理目标集合，释放内存
}
// 函数功能总结：通过分层分批策略，将大规模优化问题分解为多个小规模子问题逐步求解，
// 既保证了解的质量，又显著提高了求解效率，特别适合处理大规模资源调度问题。

void Model::setSolnInfor()
{
	Util::isDebug = true;
	Util::tourCostStruct = { 0,0,0,0,0,0,0,0,0 };
	map<MainTarget*, int> mtTargetNumMap;
	map<MainTarget*, int> mtEBgnTMap;

	for (int i = 0; i < _targetList.size(); i++)
	{
		vector<TargetPlan* > planList = _targetList[i]->getType()->getTargetPlanList();
		for (int p = 0; p < planList.size(); p++)
		{
			if (_solver.getValue(_o_tm[i][p]) > EPSILON)
			{
				cout << _o_tm[i][p].getName() << " = " << _solver.getValue(_o_tm[i][p]);
				_targetList[i]->setSolnTargetPlan(planList[p]);
				map<MountLoad*, int>::iterator  iter = planList[p]->_mountLoadPlan.begin();
				for (iter; iter != planList[p]->_mountLoadPlan.end(); iter++)
				{
					cout << "," << (*iter).first->_mount->getName() << "-" << (*iter).first->_load->getName() << "-" << (*iter).second;
				}
				cout << endl;

				if (mtTargetNumMap.find(_targetList[i]->getMainTarget()) != mtTargetNumMap.end())
				{
					mtTargetNumMap[_targetList[i]->getMainTarget()] = mtTargetNumMap[_targetList[i]->getMainTarget()] + 1;
				}
				else
				{
					mtTargetNumMap[_targetList[i]->getMainTarget()] = 1;
				}
			}
		}
	}

	//double costTotalTour = 0;
	//double maxTourTime = 0;
	for (int i = 0; i < _tourList.size(); i++)
	{
		if (_solver.getValue(_y_gr[i]) > EPSILON)
		{
			_tourList[i]->setLpVal(_solver.getValue(_y_gr[i]));
			int numTour = (int)_solver.getValue(_y_gr[i]);
			if ((_solver.getValue(_y_gr[i]) - numTour) > 0.5)
			{
				numTour++;
			}
			for (int j = 0; j < numTour; j++)
			{
				//_tourList[i]->computeCost();
				_tourList[i]->computeTimeMap();
				_tourList[i]->computeCostNew();
				//_tourList[i]->print();
				map<Slot*, int> slotMap = _tourList[i]->getTimeMap();
				map<Slot*, int>::iterator iter = slotMap.begin();

				_solnTourList.push_back(_tourList[i]);
				_costTotalTour += _tourList[i]->getCost();
				if (_maxTourTime < _tourList[i]->getTotalTime()) {
					_maxTourTime = _tourList[i]->getTotalTime();
				}
				vector<OperTarget*> operTargetList = _tourList[i]->getOperTargetList();
				for (int j = 0; j < operTargetList.size(); j++)
				{
					Target* target = operTargetList[j]->getTarget();
					int eBgnSlotInd = operTargetList[j]->getEarliestSlotInd();
					if (mtTargetNumMap.find(target->getMainTarget()) != mtTargetNumMap.end() && mtTargetNumMap[target->getMainTarget()] == target->getMainTarget()->getTargetList().size())
					{
						if (mtEBgnTMap.find(target->getMainTarget()) != mtEBgnTMap.end())
						{
							if (mtEBgnTMap[target->getMainTarget()] > eBgnSlotInd)
							{
								mtEBgnTMap[target->getMainTarget()] = eBgnSlotInd;
							}
						}
						else
						{
							mtEBgnTMap[target->getMainTarget()] = eBgnSlotInd;
						}
					}
				}
			}
		}
	}
	sort(_solnTourList.begin(), _solnTourList.end(), Tour::cmpByOrder);
	//打印目前的tour信息
	//for (int i = 0; i < _solnTourList.size(); i++)
	//{
	//	_solnTourList[i]->print();
	//}

	cout << "CostTotalTour is " << _costTotalTour << endl;
    cout << "MaxTourTime is " << _maxTourTime << endl;
	cout << "—— costUseLoad is " << Util::tourCostStruct.costUseLoad << endl;
	cout << "—— costUseMount is " << Util::tourCostStruct.costUseMount << endl;
	cout << "—— costMountLoadMatch is " << Util::tourCostStruct.costMountLoadMatch << endl;
	cout << "—— costUseAft is " << Util::tourCostStruct.costUseAft << endl;
	cout << "—— costWait is " << Util::tourCostStruct.costWait << endl;
	cout << "—— costDst is " << Util::tourCostStruct.costDst << endl;
	cout << "—— costOil is " << Util::tourCostStruct.costOil << endl;
	cout << "—— costAftFailure is " << Util::tourCostStruct.costAftFailure << endl;
	cout << "—— costFleetPrefer is " << Util::tourCostStruct.costFleetPrefer << endl;

	double profitTargetPlanPrefer = 0;
	for (int t = 0; t < _targetList.size(); t++)
	{
		vector<TargetPlan* > planList = _targetList[t]->getType()->getTargetPlanList();
		for (int j = 0; j < planList.size(); j++)
		{
			double cost = 0.0;
			//if (Util::ifEvaDmdTargetPrefer)
			//{
			//	cost += (1 - planList[j]->getPreference(_targetList[t])) * Util::costDmdTarPref;
			//}
			if (Util::ifEvaDestroyDegree)
			{
				cost += (1 - planList[j]->getRate(_targetList[t])) * Util::costDmdTarRate;
			}
			profitTargetPlanPrefer -= cost * _solver.getValue(_o_tm[t][j]);
		}
	}
	cout << "DestroyDegreeCost is " << profitTargetPlanPrefer << endl;

	double costTargetUncover = 0;
	for (int t = 0; t < _targetList.size(); t++)
	{
		double costUncover =Util::costTargetUncover;
		//if (Util::ifEvaImpTargetCoverRate)
		//{
		//	if (_targetList[t]->isImpt())
		//	{
		//		costUncover = ZZUtil::costTargetUncover * 2;
		//	}
		//}
		if (Util::ifEvaTargetPriority)
		{
			costUncover += Util::costTargetUncover * (sqrt(Util::maxPriority) - sqrt(_targetList[t]->getPriority())) / sqrt(Util::maxPriority);
		}
		costUncover += (_mainTargetList.size() - _targetList[t]->getPriority()) * Util::costTargetUncoverByOrder;
		/*新增，没找到就设置未覆盖惩罚为20000*/
		if (_addedTargetSet.find(_targetList[t]) == _addedTargetSet.end())
		{
			costUncover = 20000;
		}
		costTargetUncover += _solver.getValue(_z_t[t]) * costUncover;
	}
	cout << "CostTargetUncover is " << costTargetUncover << endl;

	//double profitGrpNumPerTarget = 0;
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	profitGrpNumPerTarget += _solver.getValue(_p_t[t]) * Util::revLessGrpTarNum;//单目标运力数量未超过阈值的奖励
	//}
	//cout << "ProfitGrpNumPerTarget is " << profitGrpNumPerTarget << endl;

	//double costGrpNumPerTarget = 0;
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	costGrpNumPerTarget += _solver.getValue(_gn_t[t]) * Util::costOverGrpPerTarNum;//单目标运力数量超过阈值的惩罚
	//	if (_solver.getValue(_gn_t[t]) > 0)
	//	{
	//		_targetList[t]->print();
	//	}
	//}
	//cout << "CostGrpNumPerTarget is " << costGrpNumPerTarget << endl;

	double costMainTargetWait = 0;
	/*for (int i = 0; i < _mainTargetList.size(); i++)
	{
		for (int j = 0; j < _slotList.size(); j++)
		{
			double cost = -10 * ((int)_mainTargetList.size() - _mainTargetList[i]->getId());
			costMainTargetWait += _solver.getValue(_q_ts[i][j]) * cost;
		}
	}
	cout << "CostMainTargetWait is " << costMainTargetWait << endl;*/

	Util::isDebug = false;
}

void Model::setSolnInforSub()
{
	map<MainTarget*, int> mtTargetNumMap;//任务包中任务的完成数量
	map<MainTarget*, int> mtEBgnTMap;
	Util::isDebug = true;
	Util::tourCostStruct = { 0,0,0,0,0,0,0,0,0 };
	double costTotalTour = 0;

	for (int i = 0; i < _targetList.size(); i++)
	{
		vector<TargetPlan* > planList = _targetList[i]->getType()->getTargetPlanList();
		for (int p = 0; p < planList.size(); p++)
		{
			if (_solver.getValue(_o_tm[i][p]) > EPSILON)//目标i由计划p执行
			{
				cout << _o_tm[i][p].getName() << " = " << _solver.getValue(_o_tm[i][p]);
				map<MountLoad*, int>::iterator  iter = planList[p]->_mountLoadPlan.begin();
				for (iter; iter != planList[p]->_mountLoadPlan.end(); iter++)
				{
					cout << "," << (*iter).first->_mount->getName() << "-" << (*iter).first->_load->getName() << "-" << (*iter).second;
				}
				cout << endl;
			}
		}
	}
	//计算任务包中任务的完成数量
	for (int i = 0; i < _targetList.size(); i++)
	{
		vector<TargetPlan* > planList = _targetList[i]->getType()->getTargetPlanList();
		for (int p = 0; p < planList.size(); p++)
		{
			if (_solver.getValue(_o_tm[i][p]) > EPSILON)
			{
				//任务包中任务的完成数量
				if (mtTargetNumMap.find(_targetList[i]->getMainTarget()) != mtTargetNumMap.end())
				{
					mtTargetNumMap[_targetList[i]->getMainTarget()] = mtTargetNumMap[_targetList[i]->getMainTarget()] + 1;
				}
				else
				{
					mtTargetNumMap[_targetList[i]->getMainTarget()] = 1;
				}
			}
		}
	}

	for (int i = 0; i < _tourList.size(); i++)
	{
		if (_solver.getValue(_y_gr[i]) > EPSILON)//轮次i被选中
		{
			_tourList[i]->setFixed(true);
			int num = (int)_solver.getValue(_y_gr[i]);
			if ((_solver.getValue(_y_gr[i]) - num) > 0.5)
			{
				num++;
			}
			_tourList[i]->setValue(num);
			//_tourList[i]->computeCost();
			_tourList[i]->computeTimeMap();
			_tourList[i]->computeCostNew();
			costTotalTour += _tourList[i]->getCost();

			//_tourList[i]->print();
			vector<OperTarget*> operTargetList = _tourList[i]->getOperTargetList();
			for (int j = 0; j < operTargetList.size(); j++)
			{
				Target* target = operTargetList[j]->getTarget();
				int eBgnSlotInd = operTargetList[j]->getEarliestSlotInd();//目标的最早时间槽索引
				if (mtTargetNumMap.find(target->getMainTarget()) != mtTargetNumMap.end() && mtTargetNumMap[target->getMainTarget()] == target->getMainTarget()->getTargetList().size())
				{
					if (mtEBgnTMap.find(target->getMainTarget()) != mtEBgnTMap.end())
					{
						if (mtEBgnTMap[target->getMainTarget()] > eBgnSlotInd)
						{
							//更新主目标的最早开始时间为eBgnSlotInd
							mtEBgnTMap[target->getMainTarget()] = eBgnSlotInd;
						}
					}
					else
					{
						mtEBgnTMap[target->getMainTarget()] = eBgnSlotInd;
					}
				}
			}
		}
		_tourList[i]->setVisit(true);
	}

	//cout << "CostTotalTour is " << costTotalTour << endl;
	//cout << "—— costUseLoad is " << Util::tourCostStruct.costUseLoad << endl;
	//cout << "—— costUseMount is " << Util::tourCostStruct.costUseMount << endl;
	//cout << "—— costMountLoadMatch is " << Util::tourCostStruct.costMountLoadMatch << endl;
	//cout << "—— costUseAft is " << Util::tourCostStruct.costUseAft << endl;
	//cout << "—— costWait is " << Util::tourCostStruct.costWait << endl;
	//cout << "—— costDst is " << Util::tourCostStruct.costDst << endl;
	//cout << "—— costOil is " << Util::tourCostStruct.costOil << endl;
	//cout << "—— costAftFailure is " << Util::tourCostStruct.costAftFailure << endl;
	//cout << "—— costFleetPrefer is " << Util::tourCostStruct.costFleetPrefer << endl;

	//double profitTargetPlanPrefer = 0;//计划偏好收益
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	vector<TargetPlan* > planList = _targetList[t]->getType()->getTargetPlanList();
	//	for (int j = 0; j < planList.size(); j++)
	//	{
	//		double cost = 0.0;
	//		//if (Util::ifEvaDmdTargetPrefer)
	//		//{
	//		//	cost += (1 - planList[j]->getPreference(_targetList[t])) * Util::costDmdTarPref;
	//		//}
	//		if (Util::ifEvaDestroyDegree)
	//		{
	//			cost += (1 - planList[j]->getRate(_targetList[t])) * Util::costDmdTarRate;
	//		}
	//		profitTargetPlanPrefer -= cost * _solver.getValue(_o_tm[t][j]);
	//	}
	//}
	////cout << "ProfitTargetPlanPrefer is " << profitTargetPlanPrefer << endl;
	//cout << "DestroyDegreeCost is " << profitTargetPlanPrefer << endl;

	double costTargetUncover = 0;//未覆盖的惩罚
	for (int t = 0; t < _targetList.size(); t++)
	{
		//重要目标（高优先级、靠前顺序）未被覆盖时 → 惩罚很重
		double costUncover = Util::costTargetUncover;
		if (Util::ifEvaTargetPriority)
		{
			costUncover += Util::costTargetUncover * (sqrt(Util::maxPriority) - sqrt(_targetList[t]->getPriority())) / sqrt(Util::maxPriority);
		}
		costUncover += (_mainTargetList.size() - _targetList[t]->getPriority()) * Util::costTargetUncoverByOrder;
		costTargetUncover += _solver.getValue(_z_t[t]) * costUncover;
	}
	cout << "CostTargetUncover is " << costTargetUncover << endl;
	
	/*为减少计算时间，已删除对应变量*/
	//满足单目标运力数量的收益
	//double profitGrpNumPerTarget = 0;
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	profitGrpNumPerTarget += _solver.getValue(_p_t[t]) * Util::revLessGrpTarNum;
	//}
	//cout << "ProfitGrpNumPerTarget is " << profitGrpNumPerTarget << endl;

	////未满足单目标运力数量的惩罚
	//double costGrpNumPerTarget = 0;
	//for (int t = 0; t < _targetList.size(); t++)
	//{
	//	costGrpNumPerTarget += _solver.getValue(_gn_t[t]) * Util::costOverGrpPerTarNum;
	//	if (_solver.getValue(_gn_t[t]) > 0)
	//	{
	//		_targetList[t]->print();
	//	}
	//}
	//cout << "CostGrpNumPerTarget is " << costGrpNumPerTarget << endl;

	//主目标等待成本
	/*double costMainTargetWait = 0;
	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		for (int j = 0; j < _slotList.size(); j++)
		{
			double cost = -10 * ((int)_mainTargetList.size() - _mainTargetList[i]->getId());
			costMainTargetWait += _solver.getValue(_q_ts[i][j]) * cost;
		}
	}
	cout << "CostMainTargetWait is " << costMainTargetWait << endl;*/

	Util::isDebug = false;
}
//是指派时间的函数，没有用到
void Model::assignTimeForSubTargets()
{
	//givenTourList
	vector<Tour*> solnTourList;
	for (int i = 0; i < _solnTourList.size(); i++)
	{
		if (_solnTourList[i]->getTourType() != BOTHFIX_TOUR)//排除类型为BOTHFIX_TOUR的tour，tour的默认类型为NOTFIX_TOUR (4)
		{
			solnTourList.push_back(_solnTourList[i]);
		}
		else
		{
			bool ifReAssignPoint = false;
			vector<OperTarget*> operTargetList = _solnTourList[i]->getOperTargetList();
			for (int m = 0; m < operTargetList.size(); m++)
			{
				map< Slot*, map<MountLoad*, int>> solnTimeMountMap = operTargetList[m]->getSolnTimeMountMap();
				map< Slot*, map<MountLoad*, vector<int>>> solnTimeAftMap = operTargetList[m]->getSolnTimeAftMap();
				Target* target = operTargetList[m]->getTarget();
				map< Slot*, map<MountLoad*, int>>::iterator iterTimeMount = solnTimeMountMap.begin();

				for (iterTimeMount; iterTimeMount != solnTimeMountMap.end(); iterTimeMount++)
				{
					map<MountLoad*, int> mountLoadMap = (*iterTimeMount).second;
					map<MountLoad*, vector<int>> aftMap = solnTimeAftMap[(*iterTimeMount).first];
					map<MountLoad*, int>::iterator iterMLMap = mountLoadMap.begin();
					vector<int> pointList;

					for (iterMLMap; iterMLMap != mountLoadMap.end(); iterMLMap++)
					{
						//target->pushSolnTimeMount((*iterTimeMount).first,(*iterMLMap).first,(*iterMLMap).second);
						//target->pushSolnTimeAft((*iterTimeMount).first, (*iterMLMap).first, aftMap[(*iterMLMap).first]);
						//for (int n = 0; n < pointList.size(); n++)
						//{
						//	(*iterTimeMount).first->deletePoint(pointList[n]);
						//}
						pointList.insert(pointList.end(), aftMap[(*iterMLMap).first].begin(), aftMap[(*iterMLMap).first].end());
					}
					if (!(*iterTimeMount).first->checkExist(pointList))
					{
						ifReAssignPoint = true;
						break;
					}
				}
			}
			if (ifReAssignPoint)
			{
				solnTourList.push_back(_solnTourList[i]);
				_solnTourList[i]->setTourType(BOTHFIX_CP_TOUR);
			}
			else
			{
				for (int m = 0; m < operTargetList.size(); m++)
				{
					map< Slot*, map<MountLoad*, int>> solnTimeMountMap = operTargetList[m]->getSolnTimeMountMap();
					map< Slot*, map<MountLoad*, vector<int>>> solnTimeAftMap = operTargetList[m]->getSolnTimeAftMap();
					Target* target = operTargetList[m]->getTarget();
					map< Slot*, map<MountLoad*, int>>::iterator iterTimeMount = solnTimeMountMap.begin();
					for (iterTimeMount; iterTimeMount != solnTimeMountMap.end(); iterTimeMount++)
					{
						map<MountLoad*, int> mountLoadMap = (*iterTimeMount).second;
						map<MountLoad*, vector<int>> aftMap = solnTimeAftMap[(*iterTimeMount).first];
						map<MountLoad*, int>::iterator iterMLMap = mountLoadMap.begin();
						for (iterMLMap; iterMLMap != mountLoadMap.end(); iterMLMap++)
						{
							vector<int> pointList;
							target->pushSolnTimeMount((*iterTimeMount).first, (*iterMLMap).first, (*iterMLMap).second);
							target->pushSolnTimeAft((*iterTimeMount).first, (*iterMLMap).first, aftMap[(*iterMLMap).first]);
							pointList = aftMap[(*iterMLMap).first];
							for (int n = 0; n < pointList.size(); n++)
							{
								(*iterTimeMount).first->deletePoint(pointList[n]);
							}
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < _slotList.size(); i++)
	{
		_slotList[i]->setRemainCap(_slotList[i]->getCap());//设置剩余量
	}
	for (int i = 0; i < solnTourList.size(); i++)
	{
		map<Slot*, int> timeMap = solnTourList[i]->getTimeMap();//单一运力，不需要按照编队考虑释放序列及时间，所以这里的timeMap的长度均为0
		map<Slot*, int>::iterator iter = timeMap.begin();
		for (iter; iter != timeMap.end(); iter++)
		{
			vector<int > slotPointList;
			int numAft = ceil((double)(*iter).second);
			if (numAft > 0)
			{
				int bgnId = ((*iter).first->getCap() - (*iter).first->getRemainCap() + 1);
				int endId = ((*iter).first->getCap() - (*iter).first->getRemainCap()) + numAft;
				for (int j = bgnId; j <= endId; j++)
				{
					slotPointList.push_back((*iter).first->getPointList()[j - 1]);
					if (j > (*iter).first->getCap())
					{
						cout << "Error: out of slot cap " << j << endl;
						exit(-1);
					}
				}
				(*iter).first->setRemainCap((*iter).first->getRemainCap() - numAft);
			}
			solnTourList[i]->setSlotPoint((*iter).first, slotPointList);
		}
	}
	for (int i = 0; i < solnTourList.size(); i++)
	{
		Fleet* fleet = solnTourList[i]->getFleet();
		int aftNum = 1;
		vector<OperTarget*> operTargetList = solnTourList[i]->getOperTargetList();
		Mount* mount = operTargetList[0]->getMountLoadPlan()[0].first->_mount;
		int mountNumPerAft = fleet->getMountNum(mount);
		map<Slot*, int> timeMap = solnTourList[i]->getTimeMap();
		map<Slot*, int>::iterator iter = timeMap.begin();
		vector<pair<Slot*, int>> remainSlotList;
		int mountNumPerTime = mountNumPerAft;
		if (mount->getMountType()->intervalTime != 0)
		{
			mountNumPerTime = 2;
		}
		for (iter; iter != timeMap.end(); iter++)
		{
			if (mount->isOccupy())
			{
				remainSlotList.push_back(make_pair((*iter).first, (*iter).second * mountNumPerTime));
			}
			else
			{
				remainSlotList.push_back(make_pair((*iter).first, aftNum * mountNumPerTime));
			}
		}
		sort(remainSlotList.begin(), remainSlotList.end(), Slot::cmpByTime);//依然为空
		vector<pair<Slot*, int>> originSlotList = remainSlotList;

		for (int m = 0; m < operTargetList.size(); m++)
		{
		Target* target = operTargetList[m]->getTarget();
			vector<pair<MountLoad*, int>> mountLoadPlan = operTargetList[m]->getMountLoadPlan();
			for (int j = 0; j < mountLoadPlan.size(); j++)
			{
				MountLoad* mountLoad = mountLoadPlan[j].first;
				int num = mountLoadPlan[j].second;
				for (int q = 0; q < remainSlotList.size(); q++)
				{
					if (remainSlotList[q].second > 0)
					{
						if (num == 0)
						{
							break;
						}
						if (num <= remainSlotList[q].second)
						{
							target->pushSolnTimeMount(remainSlotList[q].first, mountLoad, num);
							int sizeOfPoint = ceil((double)num / (double)mountNumPerTime);
							vector<int> pointList;
							if (mountLoad->_mount->isOccupy())
							{
								for (int n = (originSlotList[q].second - remainSlotList[q].second) / mountNumPerTime; n < (originSlotList[q].second - remainSlotList[q].second) / mountNumPerTime + sizeOfPoint; n++)
								{
									pointList.push_back(solnTourList[i]->getSlotPointMap()[remainSlotList[q].first][n]);
								}
							}
							operTargetList[m]->pushSolnTimeAft(remainSlotList[q].first, mountLoad, pointList);
							target->pushSolnTimeAft(remainSlotList[q].first, mountLoad, pointList);
							remainSlotList[q].second = remainSlotList[q].second - num;
							num = 0;
							break;
						}
						else
						{
							target->pushSolnTimeMount(remainSlotList[q].first, mountLoad, remainSlotList[q].second);
							int sizeOfPoint = ceil((double)remainSlotList[q].second / (double)mountNumPerTime);
							vector<int> pointList;
							if (mountLoad->_mount->isOccupy())
							{
								for (int n = (originSlotList[q].second - remainSlotList[q].second) / mountNumPerTime; n < (originSlotList[q].second - remainSlotList[q].second) / mountNumPerTime + sizeOfPoint; n++)
								{
									pointList.push_back(solnTourList[i]->getSlotPointMap()[remainSlotList[q].first][n]);
								}
							}
							operTargetList[m]->pushSolnTimeAft(remainSlotList[q].first, mountLoad, pointList);
							target->pushSolnTimeAft(remainSlotList[q].first, mountLoad, pointList);

							num = num - remainSlotList[q].second;
							remainSlotList[q].second = 0;
						}
					}
				}
			}
		}
	}
}

//void Model::resultAnalysis()
//{
//	cout << "Uncover target: " << endl;
//	vector<Target* > uncoverTargetList = _schedule->getUncoverTList();
//	for (int i = 0; i < uncoverTargetList.size(); i++)
//	{
//		uncoverTargetList[i]->print();
//		Target* target = uncoverTargetList[i];
//
//		vector<TargetPlan*> targetPlanList = target->getType()->getTargetPlanList();
//		for (int j = 0; j < targetPlanList.size(); j++)
//		{
//			bool ifEnough = false;
//			TargetPlan* targetPlan = targetPlanList[j];
//			for (int m = 0; m < _baseList.size(); m++)
//			{
//				bool ifEnoughByBase = true;
//
//				map<Mount*, int> remainMount;
//				map<Mount*, int > mountList = _baseList[m]->getMountList();
//				map<Mount*, int > useMountList = _baseList[m]->getUsedMountMap();
//				map<Mount*, int >::iterator iterMountDmd = mountList.begin();
//				for (iterMountDmd; iterMountDmd != mountList.end(); iterMountDmd++)
//				{
//					int mountNum = 0;
//					if (useMountList.find((*iterMountDmd).first) != useMountList.end())
//					{
//						mountNum = (*iterMountDmd).second - useMountList[(*iterMountDmd).first];
//					}
//					else
//					{
//						mountNum = (*iterMountDmd).second;
//					}
//					if (remainMount.find((*iterMountDmd).first) != remainMount.end())
//					{
//						remainMount[(*iterMountDmd).first] = remainMount[(*iterMountDmd).first] + mountNum;
//					}
//					else
//					{
//						remainMount[(*iterMountDmd).first] = mountNum;
//					}
//				}
//
//				map<Load*, int> remainLoad;
//				map<Load*, int > loadList = _baseList[m]->getLoadList();
//				map<Load*, int > useLoadList = _baseList[m]->getUsedLoadMap();
//				map<Load*, int >::iterator iterLoadDmd = loadList.begin();
//				for (iterLoadDmd; iterLoadDmd != loadList.end(); iterLoadDmd++)
//				{
//					int loadNum = 0;
//					if (useLoadList.find((*iterLoadDmd).first) != useLoadList.end())
//					{
//						loadNum = (*iterLoadDmd).second - useLoadList[(*iterLoadDmd).first];
//					}
//					else
//					{
//						loadNum = (*iterLoadDmd).second;
//					}
//					if (remainLoad.find((*iterLoadDmd).first) != remainLoad.end())
//					{
//						remainLoad[(*iterLoadDmd).first] = remainLoad[(*iterLoadDmd).first] + loadNum;
//					}
//					else
//					{
//						remainLoad[(*iterLoadDmd).first] = loadNum;
//					}
//				}
//
//				map<MountLoad*, int> mountLoadPlan = targetPlanList[j]->_mountLoadPlan;
//				map<MountLoad*, int>::iterator iter = mountLoadPlan.begin();
//				map<Mount*, int> mountMap;
//				map<Load*, int> loadMap;
//
//				for (iter; iter != mountLoadPlan.end(); iter++)
//				{
//					Mount* mount = (*iter).first->_mount;
//					Load* load = (*iter).first->_load;
//					int dmdNum = (*iter).second;
//					{
//						mountMap[mount] = mountMap[mount] + dmdNum;
//					}
//
//					{
//						loadMap[load] = loadMap[load] + dmdNum;
//					}
//				}
//
//				map<Mount*, int>::iterator iterMount = mountMap.begin();
//				for (iterMount; iterMount != mountMap.end(); iterMount++)
//				{
//					Mount* mount = (*iterMount).first;
//					int mountDmdNum = (*iterMount).second;
//					if (remainMount.find(mount) == remainMount.end() || (remainMount.find(mount) != remainMount.end() && mountDmdNum > remainMount[mount]))
//					{
//						ifEnoughByBase = false;
//						if (remainMount.find(mount) == remainMount.end())
//						{
//							cout << "WQ方案" << j << " 在机场 " << _baseList[m]->getName() << " 挂载 " << mount->getName() << " 需要 " << mountDmdNum << "(0）" << endl;
//						}
//						else
//						{
//							cout << "WQ方案" << j << " 在机场 " << _baseList[m]->getName() << " 挂载 " << mount->getName() << " 需要 " << mountDmdNum << "(" << remainMount[mount] << ")" << endl;
//						}
//					}
//					else
//					{
//						cout << "WQ方案" << j << " 在机场 " << _baseList[m]->getName() << " 挂载 " << mount->getName() << " 需要 " << mountDmdNum << "(" << remainMount[mount] << ")" << endl;
//					}
//					bool ifAftEnough = false;
//					for (int q = 0; q < _fleetList.size(); q++)
//					{
//						int dmdNumPerAft = _fleetList[q]->getMountNum(mount);
//						int dmdAftNum = 0;
//						if (dmdNumPerAft > 0)
//						{
//							dmdAftNum = ceil((double)(*iterMount).second / (double)dmdNumPerAft);
//							if (remainFleet.find(_fleetList[q]) == remainFleet.end() || (remainFleet.find(_fleetList[q]) != remainFleet.end() && remainFleet[_fleetList[q]] < dmdAftNum))
//							{
//								cout << "WQ方案" << j << " 机场 " << _baseList[m]->getName() << " 挂载 " << mount->getName() << " 所需要的 " << _fleetList[q]->getName() << " " << dmdAftNum << " (0)" << endl;
//							}
//							else if ((remainFleet.find(_fleetList[q]) != remainFleet.end() && remainFleet[_fleetList[q]] < dmdAftNum))
//							{
//								cout << "WQ方案" << j << " 机场 " << _baseList[m]->getName() << " 挂载 " << mount->getName() << " 所需要的 " << _fleetList[q]->getName() << " " << dmdAftNum << " (" << remainFleet[_fleetList[q]] << ")" << endl;
//							}
//							else
//							{
//								cout << "WQ方案" << j << " 机场 " << _baseList[m]->getName() << " 挂载 " << mount->getName() << " 所需要的 " << _fleetList[q]->getName() << " " << dmdAftNum << " (" << remainFleet[_fleetList[q]] << ")" << endl;
//								ifAftEnough = true;
//							}
//						}
//					}
//					if (!ifAftEnough)
//					{
//						ifEnoughByBase = false;
//					}
//				}
//				map<Load*, int>::iterator iterLoad = loadMap.begin();
//				for (iterLoad; iterLoad != loadMap.end(); iterLoad++)
//				{
//					Load* load = (*iterLoad).first;
//					int loadDmdNum = (*iterLoad).second;
//					if (remainLoad.find(load) == remainLoad.end() || (remainLoad.find(load) != remainLoad.end() && loadDmdNum > remainLoad[load]))
//					{
//						ifEnoughByBase = false;
//						cout << "WQ方案" << j << " 在机场 " << _baseList[m]->getName() << " 载荷数量不足 " << load->getName() << " 需要 " << loadDmdNum << endl;
//					}
//				}
//				if (ifEnoughByBase)
//				{
//					ifEnough = true;
//					cout << "机场 " << _baseList[m]->getName() << " 可以提供这些资源 " << endl;
//				}
//			}
//			if (ifEnough)
//			{
//				cout << "WQ方案" << j << " 资源充足 " << endl;
//			}
//			else
//			{
//				cout << "WQ方案" << j << " 资源不足 " << endl;
//			}
//		}
//	}
//
//	int countNotPrefer = 0;
//	for (int i = 0; i < _solnTourList.size(); i++)
//	{
//		vector<OperTarget*> operTargetList = _solnTourList[i]->getOperTargetList();
//		Fleet* fleet = _solnTourList[i]->getFleet();
//		Mount* mount = operTargetList[0]->getMountLoadPlan()[0].first->_mount;
//		//不考虑运力偏好
//		/*for (int j = 0; j < operTargetList.size(); j++)
//		{
//			bool ifPrefer = true;
//			Target* target = operTargetList[j]->getTarget();
//			vector<Fleet*> fleetList = target->getPreferFleetList();
//			int fleetInd = target->getPreferFleetOrder(fleet, mount);
//			if (fleetInd != 0)
//			{
//				for (int q = 0; q < fleetInd; q++)
//				{
//					if (fleetList[q]->getMountNum(mount) > 0)
//					{
//						countNotPrefer++;
//						ifPrefer = false;
//						break;
//					}
//				}
//			}
//			if (!ifPrefer)
//			{
//				cout << target->getName() << " " << fleetInd << endl;
//			}
//		}*/
//
//	}
//	cout << "Total not prefer " << countNotPrefer << endl;
//}

bool Model::feasiblePush(Tour* tour)
{
	vector<OperTarget* > operTargetList = tour->getOperTargetList();
	for (int i = 0; i < operTargetList.size(); i++)
	{
		int mtInd = operTargetList[i]->getTarget()->getMainTarget()->getId();
		int eBgnSlotInd = operTargetList[i]->getEarliestSlotInd();
		for (int j = 0; j < mtInd; j++)
		{
			if (eBgnSlotInd < _mainTargetList[j]->getSolnEarliesBgnTime())
			{
				return false;
			}
		}
	}
	return true;
}

int Model::getMaxPrevMTBgnTime(int mtInd)
{
	int maxPrevMTBgnTime = 0;
	for (int i = 0; i < mtInd; i++)
	{
		if (_mainTargetList[i]->getSolnEarliesBgnTime() > maxPrevMTBgnTime)
		{
			maxPrevMTBgnTime = _mainTargetList[i]->getSolnEarliesBgnTime();
		}
	}
	return maxPrevMTBgnTime;
}

int Model::setSolnLP()
{
	double cancelTargetNum = 0.0;
	vector<Target*> uncoverTList;
	for (int i = 0; i < _targetList.size(); i++)
	{
		if (_solver.getValue(_z_t[i]) > EPSILON)
		{
			_targetList[i]->setLpVal(_solver.getValue(_z_t[i]));
			cout << _z_t[i].getName() << " = " << _solver.getValue(_z_t[i]) << ", ";
			_targetList[i]->print();
			uncoverTList.push_back(_targetList[i]);
			cancelTargetNum += _solver.getValue(_z_t[i]);
		}
	}
	_schedule->setUncoverTList(uncoverTList);
	cout << "Total cancel target num is " << cancelTargetNum << endl;

	for (int i = 0; i < _tourList.size(); i++)
	{
		if ((_solver.getValue(_x_gtr[i]) - (int)_solver.getValue(_x_gtr[i])) > 0.9 || (_solver.getValue(_x_gtr[i]) - (int)_solver.getValue(_x_gtr[i]) == 0 && _solver.getValue(_x_gtr[i]) > EPSILON))
		{
			cout << "check " << i << " " << _solver.getValue(_x_gtr[i]) << endl;
			_tourList[i]->setFixed(true);
			int num = (int)_solver.getValue(_x_gtr[i]);
			if ((_solver.getValue(_x_gtr[i]) - num) > 0.5)
			{
				num++;
			}
			_tourList[i]->setValue(num);
		}
	}

	int fixTargetNum = 0;
	for (int i = 0; i < _tourList.size(); i++)
	{
		if (_tourList[i]->isFixed())
		{
			_x_gtr[i].setLB(_tourList[i]->getValue());
			fixTargetNum += _tourList[i]->getOperTargetList().size();
		}
	}
	return fixTargetNum;
}