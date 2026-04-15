#pragma once
#include "Tour.h"
#include "Fleet.h"
#include "Target.h"
#include "Schedule.h"

class Individual {
public:
    static int _countId;
    int _id;                //个体编号
    string _flag;            //个体基因偏好：成本最低costLowest、时间最短timeShortest、随机Normal
    int _objCount;                //目标函数个数
    
   vector<Tour* > _solnTourList;
    
    double  _SumTime;                //记录配送计划完成的时间
    double _SumCost;                //记录配送计划完成的成本
    double _SumDifference;                //记录配送计划完成的时间差

    vector<int> _variables;    // 决策变量
    vector<double> _objectives;   // 目标函数值（支持2个或3个）
    int _rank;                         // 非支配排序等级
    double _crowdingDistance;         // 拥挤度
    vector<int> _dominatedSet;   // 支配的个体集合
    int _dominatedCount;              // 支配该个体的数量

    static Schedule* _schedule;
    vector< tuple<Target*, Mount*, Fleet*>> _idxMeaning;    //记录索引对应的含义
    map <tuple<Target*, Mount*, Fleet*>,int>_meaningIdx;    //记录含义对应的索引
    map<Target*,Mount*> _targetMount;                //记录目标分配的mount
    map<pair<Target*, Mount*>, int> _targetAllocation;          //记录目标分配的mount数量
    map<Target*, int> _targetNeedNum;                                   //目标需要的货物数量
    
    map<Target*, map<Fleet*, int>> _targetAllFleet;             //记录每个目标分配的飞机提供的货物数量
    map<Fleet*,Mount*> _fleetMount;                //记录飞机分配的mount
    map<pair<Fleet*, Mount*>, int> _fleetAllocation;//注意Fleet*是飞机个体，而不是飞机类型
    map<Fleet*, vector<Target*>> _fleetTargetList; //记录每个飞机分配的目标
    map<Fleet*, map<Target*, int>> _fleetAllTargetNum;             //记录每个飞机为途径目标提供的货物数量

    map<Mount*, int> _mountNeedTotalNum;      //存储所有mount的需求总量
    map<Target*, int> _targetUncoverNum;         //存储未覆盖target的mount需求数量
    vector<int> _partPosition;                //记录染色体的各段位置

    std::mt19937 gen = Util::get_generator();
    std::uniform_real_distribution<double> dis;

    Individual(string flag, Schedule* schedule);                //个体构造函数
    ~Individual();

    vector<Tour* > getsolnTourList() { return _solnTourList; };
    void pushsolnTourList(Tour* _tour) { _solnTourList.push_back(_tour); };

    void set_objCount(int objCount) { _objCount = objCount; };
    void setsumCost(double cost) { _SumCost = cost; };                //计算总成本
    void setsumTime(double time) { _SumTime = time; };                //计算总时间
    void setSize();                //设置染色体的各段大小

    int getVariableCount() { return _variables.size(); };
    int getObjectiveCount() { return _objectives.size(); };
    void encode();                //编码函数

    void decode();        //解码函数

    static Fleet* findTimeShortestFleet(Target*, vector<Fleet*>);                            //找到前往目标时间最短的资源

    static Fleet* findCostLowestFleet(Target*, vector<Fleet*>);                            //找到前往目标成本最低的资源

    static Fleet* findDifferenceMinFleet(Target*, vector<Fleet*>);                            //找到前往目标成本最低的资源
    void cocuSumCostTime();                //计算总成本和总时间
    void cocuSumTime();                //计算总时间
    void cocuSumDifference();                //计算总时间差

    void computeObjectives();    // 计算目标函数值

    //加入随机选择之后，目标函数的计算分成部分与全部
    void cocuSumCostTimeTotal();                //计算总成本和总时间
    void cocuSumDifferenceTotal();                //计算总时间差
    void computeObjectivesTotal();
    vector<Tour*> getSolnTourList() { return _solnTourList; };

    // 支配关系判断
    bool dominates(const Individual& other) const;

    // 打印目标函数值
    void printObjectives() const;

    // 获取问题信息
    int getVariableCount() const { return _variables.size(); }
    int getObjectiveCount() const { return _objectives.size(); }

    // 修复相关方法
    void repairFeasibility();
    bool checkDemandSatisfaction();
    bool checkCapacityConstraints();
    void repairDemandViolation();
    void repairCapacityViolation();
    void repairDemandViolationforCapacity(vector <Target*>,Mount*mount);
    // 分配相关方法
    void getAllocation();
    map<pair<Fleet*, Mount*>, int> getFleetAllocation() { return _fleetAllocation; };
    //计算存储目标总需求
    void computeTargetDemand();


};