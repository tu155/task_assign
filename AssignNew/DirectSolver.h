// DirectSolver.h
#pragma once
#include "Schedule.h"
#include "Util.h"
#include <ilcplex/ilocplex.h>
#include <chrono>
#include <unordered_map>

ILOSTLBEGIN

class DirectSolver {
private:
    // 核心数据
    Schedule* _schedule;
    vector<Target*> _targetList;
    vector<Fleet*> _fleetList;
    vector<Mount*> _mountList;
    vector<Load*> _loadList;
    vector<MountLoad*> _mountLoadList;

    int _numTargets;
    int _numFleets;
    int _numMountLoads;

    // CPLEX相关对象
    IloEnv _env;
    IloModel _model;
    IloCplex _cplex;
    IloObjective _obj;

    // 优化后的一维存储 - 原有的
    IloNumVarArray _x_ftm_flat;      // 扁平化存储 x[fleet*target*mountLoad]

    // 新增变量：u_tm (目标类型选择) 和 v_gm (资源类型选择)
    IloNumVarArray _u_tm_flat;        // 扁平化存储 u[target*mountLoad]
    IloNumVarArray _v_gm_flat;        // 扁平化存储 v[fleet*mountLoad]

    IloNumVarArray _y_ft;              // y[fleet*target]
    IloNumVarArray _z_t;                // z[target]

    // 时间权重（可以从Util获取）
    double _weightTourTime;

// 新增变量
    IloNumVar _maxArrival;                    // 最晚到达时间
    IloArray<IloNumVarArray> _arrival_ft;     // arrival[f][t] 到达时间

    // 路径弧变量：z_f[s][t]  (s,t 包括基地索引0)
    IloArray<IloArray<IloBoolVarArray>> _z_fst;   // [f][s][t]

    // 每个资源的总飞行距离
    IloNumVarArray _dist_f;

    // MTZ 顺序变量 (可选，用于子回路消除)
    IloArray<IloNumVarArray> _order_ft;   // [f][t]

    //是否出动某个飞机
    IloBoolVarArray _w_f;


    // 约束数组
    IloRangeArray _targetCoverRng;          // 目标覆盖约束
    IloRangeArray _fleetSingleRng;          // 资源唯一分配约束
    IloRangeArray _mountCapacityRng;        // 挂载容量约束
    IloRangeArray _loadCapacityRng;         // 载荷容量约束
    IloRangeArray _fleetCapacityRng;        // 飞机数量约束

    // 新增约束数组
    IloRangeArray _demandSatisfyRng;         // 需求满足约束
    IloRangeArray _capacityRng;              // 资源装载能力约束
    IloRangeArray _resourceTypeRng;          // 资源类型选择约束
    IloRangeArray _targetTypeRng;            // 目标类型选择约束
    IloRangeArray _linkXVRng;                 // x与v的关联约束
    IloRangeArray _linkXURng;                 // x与u的关联约束
    // 在约束数组部分添加
    IloRangeArray _totalDemandCapRng;        // 总需求容量约束（新增）
    // 参数配置
    double _timeLimit;
    double _gapTolerance;
    double _costUncoverPenalty= Util::costTargetUncover;
    double _bigM;                              // 新增：大M参数
    double _bigM_x;         // 用于 x 变量关联约束（上界 = x 的上限 100）
    //double _bigM_time;      // 用于时间累积约束（基于最大可能航程）
    //double _bigM_sumY;      // 用于固定成本关联（上界 = 最大目标数）
    bool _verbose;
    int _threads;
    int _workMem;

    // 性能统计
    std::chrono::steady_clock::time_point _startTime;
    double _solveTime;
    double _objValue;
    IloAlgorithm::Status _solveStatus;

    // 辅助映射（预计算，用于加速约束创建）
    std::unordered_map<Mount*, std::vector<int>> _mountToMLIndices;  // 挂载 -> 挂载-载荷对索引
    std::unordered_map<Load*, std::vector<int>> _loadToMLIndices;    // 载荷 -> 挂载-载荷对索引
    std::vector<std::vector<int>> _targetToMLIndices;                // 目标 -> 可选挂载-载荷对索引
    std::vector<std::vector<int>> _fleetToMLIndices;                 // 资源 -> 可提供挂载-载荷对索引
    std::vector<std::vector<int>> _targetDemands;                    // 目标 -> 每个挂载-载荷对的需求量

    /**
     * @brief 计算资源g以需求类型m服务任务t的总成本
     */
    double computeCost(Fleet* fleet, Target* target, MountLoad* mountLoad, int num);

    /**
     * @brief 预计算所有需要的映射关系
     */
    void precomputeMappings();

    /**
     * @brief 创建决策变量
     */
    void createVariables();

    /**
     * @brief 创建约束条件
     */
    void createConstraints();

    /**
     * @brief 设置求解器参数
     */
    void setSolverParameters();

    // 索引计算函数
    inline int xIdx(int f, int t, int m) const {
        return f * _numTargets * _numMountLoads + t * _numMountLoads + m;
    }

    inline int yIdx(int f, int t) const {
        return f * _numTargets + t;
    }

    inline int uIdx(int t, int m) const {
        return t * _numMountLoads + m;
    }

    inline int vIdx(int f, int m) const {
        return f * _numMountLoads + m;
    }

    struct FleetTargetKey {
        int fleetIdx;
        int targetIdx;

        FleetTargetKey(int f, int t) : fleetIdx(f), targetIdx(t) {}

        bool operator==(const FleetTargetKey& other) const {
            return fleetIdx == other.fleetIdx && targetIdx == other.targetIdx;
        }
    };

    // 自定义哈希函数
    struct FleetTargetHash {
        std::size_t operator()(const FleetTargetKey& key) const {
            return std::hash<int>()(key.fleetIdx) ^ (std::hash<int>()(key.targetIdx) << 1);
        }
    };

    // 存储每个(f,t)组合可行的m索引
    std::unordered_map<FleetTargetKey, std::vector<int>, FleetTargetHash> _feasibleMTMap;

    // 存储每个目标的可行m索引（需求量>0）
    std::vector<std::vector<int>> _targetFeasibleM;

    // 存储每个资源的可行m索引（有挂载能力）
    std::vector<std::vector<int>> _fleetFeasibleM;

    // 存储x变量的索引映射
    std::map<std::tuple<int, int, int>, int> _xIndexMap;

    // 获取x变量索引的辅助函数
    int getXIndex(int f, int t, int m) const {
        auto it = _xIndexMap.find(std::make_tuple(f, t, m));
        if (it != _xIndexMap.end()) {
            return it->second;
        }
        return -1;  // 不存在
    }
    // 辅助函数：获取节点索引（0为基地，1..numTargets为目标）
    inline int nodeIdx(int t) const { return t + 1; }  // t从0开始，目标索引0..numTargets-1

    // 基地到目标、目标之间的距离（预计算）
    vector<vector<double>> _distMatrix;   // [numNodes][numNodes], 节点0为基地


public:
    DirectSolver(Schedule* schedule, double timeLimit = 300.0, bool verbose = false);
    ~DirectSolver();

    // 参数设置方法
    void setTimeLimit(double timeLimit) { _timeLimit = timeLimit; }
    void setGapTolerance(double gap) { _gapTolerance = gap; }
    void setUncoverPenalty(double penalty) { _costUncoverPenalty = penalty; }
    void setBigM(double bigM) { _bigM = bigM; }
    void setThreads(int threads) { _threads = threads; }
    void setWorkMem(int memMB) { _workMem = memMB; }
    void setVerbose(bool verbose) { _verbose = verbose; }

    // 求解方法
    void initModel();
    bool solve();
    vector<Tour*> extractSolution();
    vector<Tour*> run();
    void exportModel(const string& filename);
    // 多轮求解（资源可重复出动，直到所有目标被覆盖）
    vector<Tour*> runMultiRound();

    // 获取求解信息
    double getSolveTime() const { return _solveTime; }
    double getObjValue() const { return _objValue; }
    IloAlgorithm::Status getStatus() const { return _solveStatus; }
    int getVariableCount() const;
    int getConstraintCount() const;
};