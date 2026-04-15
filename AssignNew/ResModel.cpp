#include "ResModel.h"
using std::cout;

ResModel::ResModel(Schedule* sch)
	:_schedule(sch)
{
	_targetList = _schedule->getTargetList();
	_mountList = _schedule->getMountList();
	_loadList = _schedule->getLoadList();
	_mountLoadList = _schedule->getMountLoadList();
	_fleetList = _schedule->getFleetList();
	_fleetTypeList = _schedule->getFleetTypeList();
	_slotList = _schedule->getSlotList();
	_mainTargetList = _schedule->getMainTargetList();

	for (int i = 0; i < _targetList.size(); i++)
	{
		_targetList[i]->setId(i);
	}

	for (int i = 0; i < _mountList.size(); i++)
	{
		_mountList[i]->setId(i);
	}
	for (int i = 0; i < _loadList.size(); i++)
	{
		_loadList[i]->setId(i);
	}

	for (int i = 0; i < _mountLoadList.size(); i++)
	{
		_mountLoadList[i]->setId(i);
		//_mountLoadList[i]->print();
	}
	//for (int i = 0; i < _taskPointList.size(); i++)
	//{
	//	_taskPointList[i]->setId(i);
	//}
	for (int i = 0; i < _fleetList.size(); i++)
	{
		_fleetList[i]->setId(i);
	}
	for (int i = 0; i < _fleetTypeList.size(); i++)
	{
		_fleetTypeList[i]->setId(i);
	}

	for (int i = 0; i < _slotList.size(); i++)
	{
		_slotList[i]->setId(i);
	}
	for (int i = 0; i < _mainTargetList.size(); i++)
	{
		_mainTargetList[i]->setId(i);
	}

	_startClock = clock();
	_colGenCount = 0;
	_solnCount = 0;
}

ResModel::~ResModel()
{
}

void ResModel::init()
{
	_model = IloModel(_env);
	_obj = IloAdd(_model, IloMinimize(_env));// 创建目标函数，最小化
	_solver = IloCplex(_model);
	_y_gr = IloNumVarArray(_env);//编队—组合的选择变量
	_x_gtr = IloNumVarArray(_env);//new add，//编队类型—组合的选择变量

	_assignRng = IloRangeArray(_env);
	//_wDmdRng = IloRangeArray(_env);
	//_uDmdRng = IloRangeArray(_env);
	_wCapRng = IloRangeArray(_env);
	_uCapRng = IloRangeArray(_env);
	
	_fCapRng = IloRangeArray(_env);//new add
	
	_dmdAsgRng = IloRangeArray(_env);
	_pairDmdRng = IloRangeArray(_env);
	_tgAsgNumRng = IloRangeArray(_env);
	_ttpAsgRng = IloRangeArray(_env);
	_ttpLimitRng = IloRangeArray(_env);
	_attackRobustRng = IloRangeArray(_env);

	int totalRngSize = 0;

	//for (int i = 0; i < _groupList.size(); i++)
	//{
	//	string name = "C1_Assign_G" + to_string(_groupList[i]->getId());
	//	_assignRng.add(IloRange(_env, 0, ZZUtil::groupRoundNumLimit, name.c_str()));
	//}
	//_model.add(_assignRng);

	_o_tm = NumVar2Matrix(_env, _targetList.size());
	for (int t = 0; t < _targetList.size(); t++)
	{
		_o_tm[t] = IloNumVarArray(_env);
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
			string name = "O_T" + to_string(_targetList[t]->getId()) + "_M" + to_string(j);
			_o_tm[t].add(IloNumVar(_env, 0, 1, ILOBOOL, name.c_str()));
			//_o_tm[t].add(IloNumVar(_env, 0, 1, ILOFLOAT, name.c_str()));
			_obj.setLinearCoef(_o_tm[t][j], -cost);//设置目标函数中o_tj的系数
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
		double perturb = 0;// = ((double)(rand() % 1000)) / 1000000;
		_dmdAsgRng.add(IloRange(_env, 1 - perturb, expr, 1 + perturb, name.c_str()));//每个目标只能选择唯一的计划
	}
	_model.add(_dmdAsgRng);
	cout << "#dmdAsgRng size:" << _dmdAsgRng.getSize() << endl;
	totalRngSize += _dmdAsgRng.getSize();

	//目标未覆盖变量
	_z_t = IloNumVarArray(_env);
	for (int t = 0; t < _targetList.size(); t++)
	{
		IloNumColumn col(_env);//列对象，为z_t变量添加目标系数和约束
		double costUncover = Util::costTargetUncover;

		if (Util::ifEvaTargetPriority)
		{
			costUncover += Util::costTargetUncover * (sqrt(Util::maxPriority) - sqrt(_targetList[t]->getPriority())) / sqrt(Util::maxPriority);
		}
		costUncover += _targetList[t]->getPriority() * Util::costTargetUncoverByOrder;
		col = _obj(costUncover);//设置这个变量在目标函数中的系数目标未覆盖的惩罚系数
		col += _dmdAsgRng[_targetList[t]->getId()](1);//设置这个目标未覆盖变量在对应约束（_dmdAsgRng）中的系数
		string name = "Z_T" + to_string(_targetList[t]->getId());
		int ub = 1;
		_z_t.add(IloNumVar(col, 0, ub, ILOFLOAT, name.c_str()));//变量的上下界，并与刚刚的列对象关联
	}
	_model.add(_z_t);
	/*
	约束2：目标的挂载-载荷对的需求满足
	*/
	//目标在不同的计划中，对挂载-载荷对的需求量限制
	for (int t = 0; t < _targetList.size(); t++)
	{
		vector<TargetPlan*> planList = _targetList[t]->getType()->getTargetPlanList();
		for (int j = 0; j < _mountLoadList.size(); j++)
		{
			IloExpr expr(_env);
			for (int p = 0; p < planList.size(); p++)
			{
				expr += (-1) * planList[p]->getMountLoadNum(_mountLoadList[j]) * _o_tm[t][p];//目标t在计划p中，分配到的挂载-载荷对j的数量
			}
			string name = "C3_pairDmd_P" + to_string(_mountLoadList[j]->getId()) + "_T" + to_string(_targetList[t]->getId());
			_pairDmdRng.add(IloRange(_env, 0, expr, IloInfinity, name.c_str()));
		}
	}
	_model.add(_pairDmdRng);
	cout << "#pairDmdRng size:" << _pairDmdRng.getSize() << endl;
	totalRngSize += _pairDmdRng.getSize();
	
	//约束3：载荷数量上限，先设置为2000（只有约束范围，缺少变量和系数设置）
	for (int j = 0; j < _loadList.size(); j++)
	{
		string name = "C5_uCap_U" + to_string(_loadList[j]->getId());
		_uCapRng.add(IloRange(_env, 0,2000, name.c_str()));
	}
	_model.add(_uCapRng);
	cout << "#uCapRng size:" << _uCapRng.getSize() << endl;
	totalRngSize += _uCapRng.getSize();
	//约束4：挂载数量上限，默认1000（只有约束范围，缺少变量和系数设置）
	for (int j = 0; j < _mountList.size(); j++)
	{
		string name = "C6_wCap_W" + to_string(_mountList[j]->getId());
		_wCapRng.add(IloRange(_env, 0,1000, name.c_str()));
	}
	_model.add(_wCapRng);
	cout << "#wCapRng size:" << _wCapRng.getSize() << endl;
	totalRngSize += _wCapRng.getSize();

	//约束：飞机数量上限（只有约束范围，缺少变量和系数设置）
	for (int j = 0; j < _fleetList.size(); j++)
	{
		string name = "C7_fCap_F" + to_string(_fleetList[j]->getId());
		_fCapRng.add(IloRange(_env, 0, _schedule->getFleetList().size()* Util::groupRoundNumLimit, name.c_str()));
	}
	_model.add(_fCapRng);
	cout << "#fCapRng size:" << _fCapRng.getSize() << endl;
	totalRngSize += _fCapRng.getSize();

	//约束：给每个目标分配的最大运力数量
	for (int i = 0; i < _targetList.size(); i++)
	{
		string name = "C8_tgAsgNumRng_T" + to_string(_targetList[i]->getId());
		int grpPerTarNumUb = Util::groupPertargetNumLimit;
		_tgAsgNumRng.add(IloRange(_env, 0, grpPerTarNumUb, name.c_str()));
	}
	_model.add(_tgAsgNumRng);
	cout << "#tgAsgNumRng size:" << _tgAsgNumRng.getSize() << endl;
	totalRngSize += _tgAsgNumRng.getSize();

	//设置单目标编队数量少于阈值的奖励松弛变量、关联最大运力数量的范围约束
	_p_t = IloNumVarArray(_env);
	for (int t = 0; t < _targetList.size(); t++)
	{
		IloNumColumn col(_env);
		col = _obj(Util::revLessGrpTarNum);//少于最大运力数量的奖励系数
		col += _tgAsgNumRng[_targetList[t]->getId()](1);//设置这个变量在对应约束中的系数为1
		string name = "P_T" + to_string(_targetList[t]->getId());
		_p_t.add(IloNumVar(col, 0, IloInfinity, ILOFLOAT, name.c_str()));
	}
	_model.add(_p_t);
	//设置单目标编队数量超过阈值的惩罚松弛变量、关联最大运力数量的范围约束
	_gn_t = IloNumVarArray(_env);
	for (int t = 0; t < _targetList.size(); t++)
	{
		IloNumColumn col(_env);
		col = _obj(Util::costOverGrpPerTarNum);
		col += _tgAsgNumRng[_targetList[t]->getId()](-1);//在对应约束中的系数为-1
		string name = "GN_T" + to_string(_targetList[t]->getId());
		_gn_t.add(IloNumVar(col, 0, IloInfinity, ILOFLOAT, name.c_str()));
	}
	_model.add(_gn_t);

	cout << "#Rng size:" << totalRngSize << endl;
}

void ResModel::addColsByGrp(vector<Tour* > tourList)
{
	//打印当前列生成计数器_colGenCount
	cout << "#count:" << _colGenCount << endl;
	//打印要添加的列的数量
	cout << "addCols:" << tourList.size() << endl;
	for (int i = 0; i < tourList.size(); i++)
	{
		tourList[i]->setId(_tourList.size());
		_tourList.push_back(tourList[i]);
	}
	for (int i = 0; i < tourList.size(); i++)
	{
		IloNumColumn col(_env);//创建一个列对象
		int cost = rand() % (50 - 1 + 1) + 1;
		//cout << cost << endl;
		col = _obj(tourList[i]->getCost()/*cost*/);//设置这个变量在目标函数中的系数，这个tour的成本
		//assignRng(2)
		//col += _assignRng[tourList[i]->getFleet()->getId()](1);
		
		vector<OperTarget* >operTargetList = tourList[i]->getOperTargetList();
		//mountLoad以对的形式约束
		for (int j = 0; j < operTargetList.size(); j++)
		{
			vector<pair<MountLoad*, int>> mountLoadList = operTargetList[j]->getMountLoadPlan();
			for (int p = 0; p < mountLoadList.size(); p++)
			{
				int pairIndex = mountLoadList[p].first->getId();
				//mountLoadList[p].first->print();
				int num = mountLoadList[p].second;
				//cout << "targetId-" << operTargetList[j]->getTarget()->getId() << " mountLoadSize-" << _mountLoadList.size() << " pairIndex-" << pairIndex << endl;
				col += _pairDmdRng[operTargetList[j]->getTarget()->getId() * _mountLoadList.size() + pairIndex](num);//设置这个变量在对应约束中的系数为num
			}
		}

		map<Load*, int> loadMap = tourList[i]->getLoadMap();
		map<Load*, int >::iterator loadIter;
		map<Mount*, int> mountMap = tourList[i]->getMountMap();
		map<Mount*, int>::iterator mountIter;
		for (loadIter = loadMap.begin(); loadIter != loadMap.end(); loadIter++)
		{
			int index = loadIter->first->getId();
			int num = loadIter->second;
			col += _uCapRng[index](num);//载荷容量约束中的系数设置为num
		}
		for (mountIter = mountMap.begin(); mountIter != mountMap.end(); mountIter++)
		{
			int index = mountIter->first->getId();
			int num = mountIter->second;
			col += _wCapRng[index](num);//挂载容量约束中的系数设置为num
		}

		//_tgAsgNumRng每个目标分配的飞机数量
		for (int j = 0; j < operTargetList.size(); j++)
		{
			col += _tgAsgNumRng[operTargetList[j]->getTarget()->getId()](1);//单目标最大运力数量的约束中，找得到这个operTargetList中经过的目标，给飞机数量变量设置系数为1
		}

		string name = "y_g" + to_string(tourList[i]->getFleet()->getId()) + "_r" + to_string(tourList[i]->getId());
		IloNumVar var = IloNumVar(col, 0, 1, ILOFLOAT, name.c_str());//变量数同tourlist的长度，设置变量的上下界
		_y_gr.add(var);
		_model.add(var);
		col.end();
	}
	//输出添加列所花费的时间
	cout << "AddCols Dur is " << (clock() - _startClock) / 1000 << "s." << endl;
}

void ResModel::solveIP()
{
	//string name = Util::OUTPUTPATH + "solveIP.lp";
	//cout << "output: " << name << endl;
	_convertIP = IloConversion(_env, _y_gr, ILOBOOL);//IloConversion将x_gr的变量类型转换为布尔型
	//_convertIP = IloConversion(_env, _x_gtr, ILOINT);//IloConversion将x_gtr的变量类型从浮点转换为整数
	_model.add(_convertIP);//变量添加到模型中
	if (Util::maxThreadNumLimit != 0)
	{
		_solver.setParam(IloCplex::Threads, Util::maxThreadNumLimit);//设置最大线程数
	}
	string timeStr = to_string(clock() / 60);
	string name = Util::OUTPUTPATH + "solveIP" + to_string(_colGenCount++) + timeStr + ".lp";
	cout << "output: " << name << endl;
	//_solver.setParam(IloCplex::RootAlg, IloCplex::Algorithm::Dual);
	//_solver.setParam(IloCplex::RootAlg, IloCplex::Algorithm::Barrier);
	//_solver.setParam(IloCplex::RootAlg, IloCplex::Algorithm::Sifting);
	//_solver.exportModel(name.c_str());
	_solver.setParam(IloCplex::TiLim, 200);
	_solver.setParam(IloCplex::EpGap, 0.015);

	_solver.solve();

	cout << "Solution status: " << _solver.getStatus() << endl;
	if (_solver.getStatus() == IloAlgorithm::Infeasible)
	{
		exit(0);
	}
	cout << "Optimal Value: " << _solver.getObjValue() << endl;
}

//将求解器求解结果（目标函数计算等）设置到类中
vector<Tour*> ResModel::setSolnInfor()
{
	Util::isDebug = true;
	Util::tourCostStruct = { 0,0,0,0,0,0,0,0,0 };
	vector<Tour*> tourList;//保存选中的轮次

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

	double cancelTargetNum = 0.0;
	for (int i = 0; i < _targetList.size(); i++)
	{
		if (_solver.getValue(_z_t[i]) > EPSILON)//目标i没有被覆盖
		{
			_targetList[i]->setLpVal(_solver.getValue(_z_t[i]));
			cout << _z_t[i].getName() << " = " << _solver.getValue(_z_t[i]) << ", ";
			_targetList[i]->print();
			cancelTargetNum += _solver.getValue(_z_t[i]);
		}
	}
	cout << "Total cancel target num is " << cancelTargetNum << endl;

	for (int i = 0; i < _targetList.size(); i++)
	{
		vector<TargetPlan* > planList = _targetList[i]->getType()->getTargetPlanList();
		int maxValue = 0;
		TargetPlan* prePlan = NULL;
		int maxInd = 0;


		double costTotalTour = 0;
		for (int i = 0; i < _tourList.size(); i++)
		{
			//if (_solver.getValue(_x_gtr[i]) > EPSILON)//轮次i的x_gtr被选中
			if (_solver.getValue(_y_gr[i]) > EPSILON)//轮次i被选中
			{
				//对小数解进行四舍五入，确保得到整数解
				int numTour = (int)_solver.getValue(_y_gr[i]);
				if ((_solver.getValue(_y_gr[i]) - numTour) > 0.5)
				{
					numTour++;
				}
				for (int j = 0; j < numTour; j++)
				{
					_tourList[i]->computeCost();
					_tourList[i]->print();
					costTotalTour += _tourList[i]->getCost();
					tourList.push_back(_tourList[i]);//保存选中的轮次，如果numTour>1,那么保存多次
				}
			}
		}
		//输出每个目标超出目标最大可接受运力的数量（如果有超出）
		for (int i = 0; i < _targetList.size(); i++)
		{
			if (_solver.getValue(_gn_t[i]) > EPSILON)
			{
				cout << _solver.getValue(_gn_t[i]) << endl;
				_targetList[i]->print();
			}
		}

		cout << "CostTotalTour is " << costTotalTour << endl;
		//未找到相关设置
		cout << "—— costUseLoad is " << Util::tourCostStruct.costUseLoad << endl;
		cout << "—— costUseMount is " << Util::tourCostStruct.costUseMount << endl;
		cout << "—— costMountLoadMatch is " << Util::tourCostStruct.costMountLoadMatch << endl;
		cout << "—— costUseAft is " << Util::tourCostStruct.costUseAft << endl;
		cout << "—— costWait is " << Util::tourCostStruct.costWait << endl;
		cout << "—— costDst is " << Util::tourCostStruct.costDst << endl;
		cout << "—— costOil is " << Util::tourCostStruct.costOil << endl;
		cout << "—— costAftFailure is " << Util::tourCostStruct.costAftFailure << endl;
		cout << "—— costFleetPrefer is " << Util::tourCostStruct.costFleetPrefer << endl;

		double profitTargetPlanPrefer = 0;//计划偏好收益
		for (int t = 0; t < _targetList.size(); t++)
		{
			vector<TargetPlan* > planList = _targetList[t]->getType()->getTargetPlanList();
			for (int j = 0; j < planList.size(); j++)
			{
				double cost = 0.0;
				//if (Util::ifEvaDmdTargetPrefer)
				//{
				//	cost += (1 - planList[j]->getPreference(_targetList[t])) * Util::costDmdTarPref;//每个目标都有不同的偏好系数，再乘以目标需求偏好的成本
				//}
				if (Util::ifEvaDestroyDegree)
				{
					cost += (1 - planList[j]->getRate(_targetList[t])) * Util::costDmdTarRate;
				}
				profitTargetPlanPrefer -= cost * _solver.getValue(_o_tm[t][j]);
			}
		}
		//cout << "ProfitTargetPlanPrefer is " << profitTargetPlanPrefer << endl;
		cout << "DestroyDegreeCost is " << profitTargetPlanPrefer << endl;

		double costTargetUncover = 0;//未覆盖的惩罚
		for (int t = 0; t < _targetList.size(); t++)
		{
			//重要目标（高优先级、靠前顺序）未被覆盖时 → 惩罚很重
			double costUncover = Util::costTargetUncover;
			if (Util::ifEvaTargetPriority)
			{
				costUncover += Util::costTargetUncover * (sqrt(Util::maxPriority) - sqrt(_targetList[t]->getPriority())) / sqrt(Util::maxPriority);//优先级越高，惩罚值越低
			}
			costUncover += (_mainTargetList.size() - _targetList[t]->getPriority()) * Util::costTargetUncoverByOrder;//ID越靠前，惩罚值越高
			costTargetUncover += _solver.getValue(_z_t[t]) * costUncover;
		}
		cout << "CostTargetUncover is " << costTargetUncover << endl;

		//满足单目标运力数量的收益
		double profitGrpNumPerTarget = 0;
		for (int t = 0; t < _targetList.size(); t++)
		{
			profitGrpNumPerTarget += _solver.getValue(_p_t[t]) * Util::revLessGrpTarNum;
		}
		cout << "ProfitGrpNumPerTarget is " << profitGrpNumPerTarget << endl;

		//未满足单目标运力数量的惩罚
		double costGrpNumPerTarget = 0;
		for (int t = 0; t < _targetList.size(); t++)
		{
			costGrpNumPerTarget += _solver.getValue(_gn_t[t]) * Util::costOverGrpPerTarNum;
		}
		cout << "CostGrpNumPerTarget is " << costGrpNumPerTarget << endl;

		Util::isDebug = false;

		return tourList;//返回轮次列表
	}
}
void ResModel::solveLP()
{
	/*if (_colGenCount != 0)
	{
		warmStart();
	}*/

	string name = Util::OUTPUTPATH + "m" + to_string(_colGenCount++) + ".lp";
	//_solver.exportModel(name.c_str());
	cout << "Col Gen Problem " << _colGenCount << endl;
	_solver.setParam(IloCplex::RootAlg, IloCplex::Barrier);
	//_solver.setParam(IloCplex::IntParam::AdvInd, 1);
	_solver.setParam(IloCplex::EpGap, 0.05);

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