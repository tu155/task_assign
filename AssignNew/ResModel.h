#ifndef RESMODEL_H
#define RESMODEL_H
#include"Schedule.h"
typedef IloArray<IloNumVarArray> NumVar2Matrix;
typedef IloArray<IloNumArray> Num2Matrix;
class ResModel
{
private:
	Schedule* _schedule;

	vector<Tour*> _tourList;
	vector<Target*> _targetList;
	vector<Mount*> _mountList;
	vector<Fleet*> _fleetList;
	vector<Load*> _loadList;
	vector<MountLoad*> _mountLoadList;
	vector<FleetType*> _fleetTypeList;
	vector<Slot* > _slotList;
	vector<MainTarget*> _mainTargetList;
	//vector<pair<Group*, ZZTour*>> _solnList;
	vector<Tour* > _solnTourList;

	IloEnv _env;									//模型环境
	IloModel _model;								//模型
	IloCplex _solver;								//模型求解器
	IloObjective _obj;								//模型目标函数

	//主要模式二选一
	//1.编队
	IloNumVarArray _y_gr;							//编队—组合的选择变量
	IloRangeArray _assignRng;						//编队轮次限制约束
	//2.编队类型(主)
	IloNumVarArray _x_gtr;							//编队类型—组合的选择变量 new add
	IloRangeArray _fCapRng;							//基地机型数量上限约束 new add

	//公共变量
	NumVar2Matrix _o_tm;							//目标—需求模式的选择变量
	IloNumVarArray _z_t;							//目标覆盖变量
	IloNumVarArray _p_t;							//单目标编队数量尽量少的奖励松弛变量
	IloNumVarArray _gn_t;							//违反单目标编队数量的惩罚松弛变量

	//公共约束
	IloRangeArray _wDmdRng;							//目标挂载数量需求约束(在resmodel中并没用到)
	IloRangeArray _uDmdRng;							//目标载荷数量需求约束（在resmodel中并没用到）
	IloRangeArray _wCapRng;							//基地挂载数量上限约束
	IloRangeArray _uCapRng;							//基地载荷数量上限约束
	IloRangeArray _dmdAsgRng;						//目标—需求模式选择约束
	IloRangeArray _pairDmdRng;						//目标——（挂载载荷对）需求约束

	//公平性相关的变量和约束
	//机型
	IloNumVarArray _u_f;							//所有基地某机型飞机的平均剩余率
	IloNumVarArray _overAft_bf;						//某基地某机型比平均剩余率高的部分的松弛变量
	IloNumVarArray _lessAft_bf;						//某基地某机型比平均剩余率低的部分的松弛变量
	IloNumVarArray _lessLbAft_bft;					//某基地某机型比基本剩余率低的部分的松弛变量

	IloRangeArray _aftBaseLbRng;					//机型下下使用率下限约束
	IloRangeArray _aftUseRateRng;					//飞机使用率计算约束
	IloRangeArray _aftBaseFairRng;					//飞机公平性约束

	//挂载
	IloNumVarArray _u_m;							//所有基地某挂载的平均剩余率
	IloNumVarArray _overMount_bm;					//某基地某挂载比平均剩余率高的部分的松弛变量
	IloNumVarArray _lessMount_bm;					//某基地某挂载比平均剩余率低的部分的松弛变量
	IloNumVarArray _lessLbMount_bm;					//某基地某类挂载比基本剩余率低的部分的松弛变量

	IloRangeArray _mountBaseLbRng;					//挂载使用率下限约束
	IloRangeArray _mountUseRateRng;					//挂载使用率计算约束
	IloRangeArray _mountBaseFairRng;				//挂载公平性约束

	//载荷
	IloNumVarArray _u_l;							//所有基地某载荷的平均剩余率
	IloNumVarArray _overLoad_bl;					//某基地某载荷比平均剩余率高的部分的松弛变量
	IloNumVarArray _lessLoad_bl;					//某基地某载荷比平均剩余率低的部分的松弛变量
	IloNumVarArray _lessLbLoad_bl;					//某基地某类载荷比基本剩余率低的部分的松弛变量

	IloRangeArray _loadBaseLbRng;					//载荷使用率下限约束
	IloRangeArray _loadUseRateRng;					//载荷使用率计算约束
	IloRangeArray _loadBaseFairRng;					//载荷公平性约束

	//机型大类
	IloNumVarArray _u_ft;							//所有基地某类机型飞机的平均剩余率
	IloNumVarArray _overAft_bft;					//某基地某类机型比平均剩余率高的部分的松弛变量
	IloNumVarArray _lessAft_bft;					//某基地某类机型比平均剩余率低的部分的松弛变量
	IloNumVarArray _lessLbFAft_bft;					//某基地某类机型比基本剩余率低的部分的松弛变量

	IloRangeArray _aftFBaseLbRng;					//飞机大类下使用率下限约束
	IloRangeArray _aftFUseRateRng;					//飞机大类下使用率计算约束
	IloRangeArray _aftFBaseFairRng;					//飞机大类下公平性约束

	//打击目标鲁棒性 变量/约束
	IloRangeArray _attackRobustRng;					//打击目标损毁率鲁棒性
	IloNumVarArray _slackRb_t;						//弱限制的松弛变量

	////目标-同基地同机型偏好
	//IloNumVarArray _q_tbf;							//目标是否被来自基地b机型f的编队作业
	//IloRangeArray _targetGrpPreRng;					//目标是否被来自基地b机型f的编队作业的辅助约束

	////演训偏好
	//IloNumVarArray _q_tbbff;						//目标被来自某两个基地及对应机型f的编队作业的数量
	//IloRangeArray _trainFreqPreRng;					//基于演训关系的变量赋值

	//目前没有使用的约束
	IloRangeArray _tgAsgNumRng;						//给每个目标分配的编队数量限制
	IloRangeArray _ttpAsgRng;						//给每个目标选择一个任务点
	IloRangeArray _ttpLimitRng;						//分配的编队必须去所选的任务点

	IloConversion _convertIP;

	clock_t _startClock;
	clock_t _currentClock;

	int _colGenCount;
	int _solnCount;

public:
	ResModel(Schedule* sch);								//模型构造函数
	~ResModel();												//模型解构函数

	void init();											//模型初始化
	void addColsByGrp(vector<Tour*> tourList);			//添加模型主变量（列）
	//void addColsByGrpType(vector<Tour*> tourList);		//添加模型主变量（列）

	void solveIP();											//求解整数规划解
	void solveLP();
	vector<Tour*>  setSolnInfor();							//将求解器求解结果设置到类中
};

#endif // !ZZRESMODEL_H
