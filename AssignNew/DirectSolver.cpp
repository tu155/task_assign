// DirectSolver.cpp
#include "DirectSolver.h"
#include "Util.h"
#include <fstream>
#include <iomanip>
#include <numeric>

using namespace std;

// ==================== 构造函数和析构函数 ====================

DirectSolver::DirectSolver(Schedule* schedule, double timeLimit, bool verbose)
    : _schedule(schedule)
    , _timeLimit(timeLimit)
    , _verbose(verbose)
    , _gapTolerance(0.05)
    , _costUncoverPenalty(10000.0)
    , _bigM(10000.0)
    , _threads(0)
    , _workMem(2048)
    , _solveTime(0.0)
    , _objValue(0.0)
    , _solveStatus(IloAlgorithm::Unknown) {

    // 从Util获取时间权重
    _weightTourTime = Util::weightTourTime;  // 假设Util中有这个变量

    // 获取必要的数据列表
    _targetList = schedule->getTargetList();
    _fleetList = schedule->getFleetList();
    _mountList = schedule->getMountList();
    _loadList = schedule->getLoadList();
    _mountLoadList = schedule->getMountLoadList();

    _numTargets = _targetList.size();
    _numFleets = _fleetList.size();
    _numMountLoads = _mountLoadList.size();

    // 预计算映射关系
    precomputeMappings();

    // 构建距离矩阵 (节点0=基地, 节点1..numTargets为目标)
    int numNodes = _numTargets + 1;
    _distMatrix.assign(numNodes, vector<double>(numNodes, 0.0));

    // 获取基地坐标（假设 Schedule 提供 getBasePosition）
    Base* base = _schedule->getBase();
    GeoPoint basePos(base->getLongitude(), base->getLatitude());

    for (int t = 0; t < _numTargets; ++t) {
        auto pos = _targetList[t]->getPosition();
        GeoPoint target(pos.first, pos.second);
        double d = TSPSolver::calculateDistance(basePos, target);
        _distMatrix[0][t + 1] = _distMatrix[t + 1][0] = d;
    }

    for (int i = 0; i < _numTargets; ++i) {
        for (int j = i + 1; j < _numTargets; ++j) {
            auto pos1 = _targetList[i]->getPosition();
            auto pos2 = _targetList[j]->getPosition();
            GeoPoint p1(pos1.first, pos1.second);
            GeoPoint p2(pos2.first, pos2.second);
            double d = TSPSolver::calculateDistance(p1, p2);
            _distMatrix[i + 1][j + 1] = _distMatrix[j + 1][i + 1] = d;
        }
    }
}

DirectSolver::~DirectSolver() {
    _env.end();
}

// ==================== 预计算映射关系 ====================

void DirectSolver::precomputeMappings() {
    // 1. 挂载 -> 挂载-载荷对索引
    for (int ml = 0; ml < _numMountLoads; ml++) {
        MountLoad* mountLoad = _mountLoadList[ml];
        if (mountLoad && mountLoad->_mount) {
            _mountToMLIndices[mountLoad->_mount].push_back(ml);
        }
        if (mountLoad && mountLoad->_load) {
            _loadToMLIndices[mountLoad->_load].push_back(ml);
        }
        if (_verbose) {
            cout << "挂载-载荷对[" << ml << "]: "
                << mountLoad->_mount->getName()
                << " -> " << mountLoad->_load->getName() << endl;
        }
    }

    // 2. 目标 -> 可选挂载-载荷对索引及需求量
    _targetToMLIndices.resize(_numTargets);
    _targetDemands.resize(_numTargets);

    for (int t = 0; t < _numTargets; t++) {
        Target* target = _targetList[t];
        TargetType* targetType = target->getType();

        vector<TargetPlan*> planList = targetType->getTargetPlanList();
        _targetDemands[t].resize(_numMountLoads, 0);

        for (TargetPlan* plan : planList) {
            for (const auto& mlPair : plan->_mountLoadPlan) {
                MountLoad* ml = mlPair.first;
                int requiredNum = mlPair.second;

                for (int m = 0; m < _numMountLoads; m++) {
                    if (_mountLoadList[m] == ml) {
                        _targetToMLIndices[t].push_back(m);
                        _targetDemands[t][m] = requiredNum;
                        break;
                    }
                }
            }
        }
    }

    // 3. 资源 -> 可提供挂载-载荷对索引
    _fleetToMLIndices.resize(_numFleets);

    for (int f = 0; f < _numFleets; f++) {
        Fleet* fleet = _fleetList[f];

        for (int m = 0; m < _numMountLoads; m++) {
            MountLoad* ml = _mountLoadList[m];
            if (ml && ml->_mount) {
                // 检查fleet是否配备这个挂载
                int mountNum = fleet->getMountNum(ml->_mount);
                if (mountNum > 0) {
                    _fleetToMLIndices[f].push_back(m);
                }
            }
        }
    }

    // 构建目标的可行m（需求量>0）
    _targetFeasibleM.resize(_numTargets);
    for (int t = 0; t < _numTargets; t++) {
        for (int m = 0; m < _numMountLoads; m++) {
            if (_targetDemands[t][m] > 0) {
                _targetFeasibleM[t].push_back(m);
            }
        }
    }

    // 构建资源的可行m（有挂载能力）
    _fleetFeasibleM.resize(_numFleets);
    for (int f = 0; f < _numFleets; f++) {
        Fleet* fleet = _fleetList[f];
        for (int m = 0; m < _numMountLoads; m++) {
            MountLoad* ml = _mountLoadList[m];
            if (ml && ml->_mount) {
                // 检查fleet是否配备这个挂载
                int mountNum = fleet->getMountNum(ml->_mount);
                if (mountNum > 0) {
                    _fleetFeasibleM[f].push_back(m);
                }
            }
        }
    }

    // 构建(f,t)的可行m组合（同时满足资源和目标）
    for (int f = 0; f < _numFleets; f++) {
        for (int t = 0; t < _numTargets; t++) {
            std::vector<int> feasibleM;

            // 求资源的可行m和目标可行m的交集
            std::set_intersection(
                _fleetFeasibleM[f].begin(), _fleetFeasibleM[f].end(),
                _targetFeasibleM[t].begin(), _targetFeasibleM[t].end(),
                std::back_inserter(feasibleM)
            );

            if (!feasibleM.empty()) {
                _feasibleMTMap[FleetTargetKey(f, t)] = feasibleM;
            }
        }
    }

    if (_verbose) {
        cout << "\n可行组合统计:" << endl;
        int totalCombos = 0;
        for (const auto& [key, vec] : _feasibleMTMap) {
            totalCombos += vec.size();
        }
        cout << "  总可行(f,t,m)组合数: " << totalCombos << endl;
        cout << "  理论上最大组合数: " << _numFleets * _numTargets * _numMountLoads << endl;
    }
}



// ==================== 核心计算方法 ====================

double DirectSolver::computeCost(Fleet* fleet, Target* target, MountLoad* mountLoad, int num) {
    double cost = 0.0;

    // 运输成本：距离 * 单位运输成本
    double distance = getDistance(
        fleet->getPos().first, fleet->getPos().second,
        target->getPosition().first, target->getPosition().second
    );
    double speed = fleet->getOptSpeed();  // 获取飞机速度
    cost += distance/speed * fleet->getOilPerMin();

    // 资源使用固定成本
    cost += fleet->getCostUse();

    // 挂载使用成本
    if (mountLoad && mountLoad->_mount) {
        cost += mountLoad->_mount->getCostUse() * num;
    }

    return cost;
}

// ==================== 模型构建方法 ====================

void DirectSolver::createVariables() {
    auto startTime = chrono::steady_clock::now();
    int numNodes = _numTargets + 1;   // 包含基地

    // 不再使用总变量数，而是只创建可行的
    int totalXVars = 0;
    for (const auto& [key, vec] : _feasibleMTMap) {
        totalXVars += vec.size();
    }

    if (_verbose) {
        cout << "\n创建变量:" << endl;
        cout << "  - x可行变量: " << totalXVars << endl;
        cout << "  - y变量: " << _numFleets * _numTargets << endl;
        cout << "  - z变量: " << _numTargets << endl;
        cout << "  - u变量: " << _numTargets * _numMountLoads << " (包含不可行的，但会被约束限制)" << endl;
        cout << "  - v变量: " << _numFleets * _numMountLoads << " (包含不可行的，但会被约束限制)" << endl;
    }

    // 初始化变量数组
    _x_ftm_flat = IloNumVarArray(_env);
    _y_ft = IloNumVarArray(_env);
    _z_t = IloNumVarArray(_env);
    _u_tm_flat = IloNumVarArray(_env);
    _v_gm_flat = IloNumVarArray(_env);
    // 创建 _maxArrival 变量
    _maxArrival = IloNumVar(_env, 0, IloInfinity, ILOFLOAT, "maxArrival");
    // 1. 批量创建x变量 - 只创建可行的
    // 需要维护一个从(f,t,m)到扁平索引的映射
    std::map<std::tuple<int, int, int>, int> xIndexMap;

    for (const auto& [key, mList] : _feasibleMTMap) {
        int f = key.fleetIdx;
        int t = key.targetIdx;

        for (int m : mList) {
            int idx = _x_ftm_flat.getSize();
            _x_ftm_flat.add(IloNumVar(_env, 0, 100, ILOINT));
            xIndexMap[std::make_tuple(f, t, m)] = idx;

            // 设置目标函数系数
            //double cost = computeCost(_fleetList[f], _targetList[t], _mountLoadList[m], 1);
            //_obj.setLinearCoef(_x_ftm_flat[idx], cost);
            // 在 createVariables 中，设置 x 系数时：
            double mountCost = 0.0;
            if (_mountLoadList[m] && _mountLoadList[m]->_mount) {
                mountCost = _mountLoadList[m]->_mount->getCostUse() * 1.0; // num=1
            }
            //_obj.setLinearCoef(_x_ftm_flat[idx], mountCost);

        }
    }

    // 保存映射供后续使用（需要在类中添加成员变量）
    _xIndexMap = xIndexMap;

    // 2. 批量创建y变量
    for (int i = 0; i < _numFleets * _numTargets; i++) {
        _y_ft.add(IloNumVar(_env, 0, 1, ILOBOOL));
    }

    // 3. 批量创建z变量
    for (int i = 0; i < _numTargets; i++) {
        _z_t.add(IloNumVar(_env, 0, 1, ILOBOOL));
        _obj.setLinearCoef(_z_t[i], _costUncoverPenalty);
    }

    // 4. 批量创建u变量
    for (int i = 0; i < _numTargets * _numMountLoads; i++) {
        _u_tm_flat.add(IloNumVar(_env, 0, 1, ILOBOOL));
    }

    // 5. 批量创建v变量
    for (int i = 0; i < _numFleets * _numMountLoads; i++) {
        _v_gm_flat.add(IloNumVar(_env, 0, 1, ILOBOOL));
    }

    // 6. 创建 _arrival_ft 二维数组，并添加到模型
    _arrival_ft = IloArray<IloNumVarArray>(_env, _numFleets);
    for (int f = 0; f < _numFleets; f++) {
        _arrival_ft[f] = IloNumVarArray(_env, _numTargets);
        for (int t = 0; t < _numTargets; t++) {
            string name = "arrival_f" + to_string(f) + "_t" + to_string(t);
            _arrival_ft[f][t] = IloNumVar(_env, 0, IloInfinity, ILOFLOAT, name.c_str());
        }
        _model.add(_arrival_ft[f]);
    }
    // 7. 添加到模型
    _model.add(_x_ftm_flat);
    _model.add(_y_ft);
    _model.add(_z_t);
    _model.add(_u_tm_flat);
    _model.add(_v_gm_flat);
    _model.add(_maxArrival);

    // 为每个资源创建弧变量 z_f[s][t] 和距离变量 dist_f
    _z_fst = IloArray<IloArray<IloBoolVarArray>>(_env, _numFleets);
    _dist_f = IloNumVarArray(_env, _numFleets, 0, IloInfinity);
    _order_ft = IloArray<IloNumVarArray>(_env, _numFleets);

    for (int f = 0; f < _numFleets; ++f) {
        _z_fst[f] = IloArray<IloBoolVarArray>(_env, numNodes);
        for (int s = 0; s < numNodes; ++s) {
            _z_fst[f][s] = IloBoolVarArray(_env, numNodes);
            for (int t = 0; t < numNodes; ++t) {
                if (s == t) continue;
                string name = "z_f" + to_string(f) + "_s" + to_string(s) + "_t" + to_string(t);
                _z_fst[f][s][t] = IloBoolVar(_env, name.c_str());
                _model.add(_z_fst[f][s][t]);
            }
        }
        _dist_f[f] = IloNumVar(_env, 0, IloInfinity, ILOFLOAT, ("dist_f" + to_string(f)).c_str());
        _order_ft[f] = IloNumVarArray(_env, _numTargets, 0, _numTargets, ILOFLOAT); // 连续即可
        _model.add(_order_ft[f]);
    }
    _model.add(_dist_f);

    // 在 createVariables() 中，其他变量创建之后
    _w_f = IloBoolVarArray(_env, _numFleets);
    for (int f = 0; f < _numFleets; ++f) {
        _w_f[f] = IloBoolVar(_env, ("w_f" + to_string(f)).c_str());
        _model.add(_w_f[f]);   // 重要：添加到模型
        _obj.setLinearCoef(_w_f[f], _fleetList[f]->getCostUse());
    }
}
void DirectSolver::createConstraints() {
    auto startTime = chrono::steady_clock::now();

    if (_verbose) {
        cout << "\n创建约束..." << endl;
    }

    // ========== 约束1：目标覆盖约束（使用u_tm）==========
    // 每个目标要么选择一种需求类型被覆盖，要么不被覆盖
    // sum_{m∈M_t} u_tm + z_t = 1

    auto c1Start = chrono::steady_clock::now();

    for (int t = 0; t < _numTargets; t++) {
        IloExpr expr(_env);

        // 只对目标可行的需求类型求和
        for (int m : _targetFeasibleM[t]) {
            expr += _u_tm_flat[uIdx(t, m)];
        }
        expr += _z_t[t];

        string name = "cover_t" + to_string(t);
        _targetCoverRng.add(IloRange(_env, 1, expr, 1, name.c_str()));
        expr.end();
    }
    _model.add(_targetCoverRng);

    if (_verbose) {
        auto c1End = chrono::steady_clock::now();
        cout << "  - 覆盖约束: " << _numTargets << " 个, 用时: "
            << chrono::duration<double>(c1End - c1Start).count() << "秒" << endl;
    }

    // ========== 约束2：需求满足约束 ==========
    // sum_f x_ftm >= demand_tm * u_tm, ∀t∈T, ∀m∈M_t

    auto c2Start = chrono::steady_clock::now();
    int demandCount = 0;

    for (int t = 0; t < _numTargets; t++) {
        for (int m : _targetFeasibleM[t]) {
            int demand = _targetDemands[t][m];

            IloExpr expr(_env);

            // 只对可行的(f,t,m)组合求和
            for (int f = 0; f < _numFleets; f++) {
                int idx = getXIndex(f, t, m);
                if (idx >= 0) {
					expr += _x_ftm_flat[idx];
                }
            }

            // sum_f x_ftm >= demand * u_tm
            // 转换为：sum_f x_ftm - demand * u_tm >= 0
            IloExpr expr2 = expr - demand * _u_tm_flat[uIdx(t, m)];

            string name = "demand_t" + to_string(t) + "_m" + to_string(m);
            _demandSatisfyRng.add(IloRange(_env, 0, expr2, IloInfinity, name.c_str()));
            expr.end();
            expr2.end();
            demandCount++;
        }
    }
    _model.add(_demandSatisfyRng);

    if (_verbose) {
        auto c2End = chrono::steady_clock::now();
        cout << "  - 需求满足约束: " << demandCount << " 个, 用时: "
            << chrono::duration<double>(c2End - c2Start).count() << "秒" << endl;
    }

    // ========== 约束3：资源装载能力约束 ==========
    // sum_t x_ftm ≤ capacity_fm * v_fm, ∀f∈F, ∀m∈M_f

    auto c3Start = chrono::steady_clock::now();
    int capacityCount = 0;

    for (int f = 0; f < _numFleets; f++) {
        Fleet* fleet = _fleetList[f];

        for (int m : _fleetFeasibleM[f]) {
            // 计算资源f对需求类型m的装载能力
            int capacity = 0;
            MountLoad* ml = _mountLoadList[m];
            if (ml && ml->_mount) {
                capacity = fleet->getMountNum(ml->_mount);
            }

            IloExpr expr(_env);

            // 只对可行的(f,t,m)组合求和
            for (int t = 0; t < _numTargets; t++) {
                int idx = getXIndex(f, t, m);
                if (idx >= 0) {
                    expr += _x_ftm_flat[idx];
                }
            }

            // sum_t x_ftm <= capacity * v_fm
            IloExpr expr2 = expr - capacity * _v_gm_flat[vIdx(f, m)];

            string name = "capacity_f" + to_string(f) + "_m" + to_string(m);
            _capacityRng.add(IloRange(_env, -IloInfinity, expr2, 0, name.c_str()));
            expr.end();
            expr2.end();
            capacityCount++;
        }
    }
    _model.add(_capacityRng);

    if (_verbose) {
        auto c3End = chrono::steady_clock::now();
        cout << "  - 装载能力约束: " << capacityCount << " 个, 用时: "
            << chrono::duration<double>(c3End - c3Start).count() << "秒" << endl;
    }

    // ========== 约束4：资源类型选择约束 ==========
    // 每个资源只能选择一个需求类型提供服务
    // sum_{m∈M_f} v_fm ≤ 1

    auto c4Start = chrono::steady_clock::now();

    for (int f = 0; f < _numFleets; f++) {
        IloExpr expr(_env);

        // 只对资源可行的需求类型求和
        for (int m : _fleetFeasibleM[f]) {
            expr += _v_gm_flat[vIdx(f, m)];
        }

        string name = "resource_type_f" + to_string(f);
        _resourceTypeRng.add(IloRange(_env, 0, expr, 1, name.c_str()));
        expr.end();
    }
    _model.add(_resourceTypeRng);

    if (_verbose) {
        auto c4End = chrono::steady_clock::now();
        cout << "  - 资源类型选择约束: " << _numFleets << " 个, 用时: "
            << chrono::duration<double>(c4End - c4Start).count() << "秒" << endl;
    }

    // ========== 约束5：目标类型选择约束（可选，已由约束1隐含）==========
    // 如果保留，应该设为 sum_{m∈M_t} u_tm ≤ 1，但约束1已经是 =1 当z_t=0时

    auto c5Start = chrono::steady_clock::now();

    for (int t = 0; t < _numTargets; t++) {
        IloExpr expr(_env);

        // 只对目标可行的需求类型求和
        for (int m : _targetFeasibleM[t]) {
            expr += _u_tm_flat[uIdx(t, m)];
        }

        string name = "target_type_t" + to_string(t);
        // 注意：这里应该用 ≤1，而不是 =1，因为当z_t=1时，sum=0
        _targetTypeRng.add(IloRange(_env, 0, expr, 1, name.c_str()));
        expr.end();
    }
    _model.add(_targetTypeRng);

    if (_verbose) {
        auto c5End = chrono::steady_clock::now();
        cout << "  - 目标类型选择约束: " << _numTargets << " 个, 用时: "
            << chrono::duration<double>(c5End - c5Start).count() << "秒" << endl;
    }

    // ========== 约束6：资源唯一分配约束 ==========
    // 6a: x <= y 关联 (x_ftm ≤ bigM * y_ft)

    auto c6Start = chrono::steady_clock::now();
    int linkXYCount = 0;

    for (int f = 0; f < _numFleets; f++) {
        for (int t = 0; t < _numTargets; t++) {
            // 检查这个(f,t)是否有任何可行的m
            auto ftIt = _feasibleMTMap.find(FleetTargetKey(f, t));
            if (ftIt != _feasibleMTMap.end()) {
                int yi = yIdx(f, t);

                for (int m : ftIt->second) {
                    int idx = getXIndex(f, t, m);
                    if (idx >= 0) {
                        IloExpr expr(_env);
                        expr = _x_ftm_flat[idx] - _bigM * _y_ft[yi];

                        string name = "link_xy_f" + to_string(f) + "_t" + to_string(t) + "_m" + to_string(m);
                        IloRange rng = IloRange(_env, -IloInfinity, expr, 0, name.c_str());
                        _model.add(rng);
                        expr.end();
                        linkXYCount++;
                    }
                }
            }
        }
    }

    //// 6b: 每个资源最多服务一个目标 (sum_t y_ft ≤ 1)
    //for (int f = 0; f < _numFleets; f++) {
    //    IloExpr expr(_env);
    //    for (int t = 0; t < _numTargets; t++) {
    //        expr += _y_ft[yIdx(f, t)];
    //    }

    //    string name = "exclusive_f" + to_string(f);
    //    _fleetSingleRng.add(IloRange(_env, 0, expr, 1, name.c_str()));
    //    expr.end();
    //}
    //_model.add(_fleetSingleRng);

    //if (_verbose) {
    //    auto c6End = chrono::steady_clock::now();
    //    cout << "  - 资源独占约束: " << _numFleets << " 个, 用时: "
    //        << chrono::duration<double>(c6End - c6Start).count() << "秒" << endl;
    //}

    // ========== 约束7：x与v的关联约束 ==========
    // sum_t x_ftm ≤ bigM * v_fm, ∀f∈F, ∀m∈M_f

    auto c7Start = chrono::steady_clock::now();
    int linkXVCount = 0;

    for (int f = 0; f < _numFleets; f++) {
        for (int m : _fleetFeasibleM[f]) {
            IloExpr expr(_env);

            for (int t = 0; t < _numTargets; t++) {
                int idx = getXIndex(f, t, m);
                if (idx >= 0) {
                    expr += _x_ftm_flat[idx];
                }
            }

            IloExpr expr2 = expr - _bigM * _v_gm_flat[vIdx(f, m)];

            string name = "link_xv_f" + to_string(f) + "_m" + to_string(m);
            _linkXVRng.add(IloRange(_env, -IloInfinity, expr2, 0, name.c_str()));
            expr.end();
            expr2.end();
            linkXVCount++;
        }
    }
    _model.add(_linkXVRng);

    if (_verbose) {
        auto c7End = chrono::steady_clock::now();
        cout << "  - x-v关联约束: " << linkXVCount << " 个, 用时: "
            << chrono::duration<double>(c7End - c7Start).count() << "秒" << endl;
    }

    // ========== 约束8：x与u的关联约束 ==========
    // sum_f x_ftm ≤ bigM * u_tm, ∀t∈T, ∀m∈M_t

    auto c8Start = chrono::steady_clock::now();
    int linkXUCount = 0;

    for (int t = 0; t < _numTargets; t++) {
        for (int m : _targetFeasibleM[t]) {
            IloExpr expr(_env);

            for (int f = 0; f < _numFleets; f++) {
                int idx = getXIndex(f, t, m);
                if (idx >= 0) {
                    expr += _x_ftm_flat[idx];
                }
            }

            IloExpr expr2 = expr - _bigM * _u_tm_flat[uIdx(t, m)];

            string name = "link_xu_t" + to_string(t) + "_m" + to_string(m);
            _linkXURng.add(IloRange(_env, -IloInfinity, expr2, 0, name.c_str()));
            expr.end();
            expr2.end();
            linkXUCount++;
        }
    }
    _model.add(_linkXURng);

    if (_verbose) {
        auto c8End = chrono::steady_clock::now();
        cout << "  - x-u关联约束: " << linkXUCount << " 个, 用时: "
            << chrono::duration<double>(c8End - c8Start).count() << "秒" << endl;
    }

    // ========== 约束9：总需求容量约束（新增）==========
    // 每种需求类型m的总需求量不超过可供量
    // sum_{t∈T} sum_{f∈F} x_ftm ≤ sum_{f∈F} capacity_fm, ∀m∈M

    auto c9Start = chrono::steady_clock::now();
    int totalDemandCount = 0;


    // 对每种需求类型创建约束
    for (int m = 0; m < _numMountLoads; m++) {

        IloExpr expr(_env);

        // 对所有目标和资源求和 x_ftm
        for (int t = 0; t < _numTargets; t++) {
            for (int f = 0; f < _numFleets; f++) {
                int idx = getXIndex(f, t, m);
                if (idx >= 0) {
                    expr += _x_ftm_flat[idx];
                }
            }
        }

        string name = "total_demand_cap_m" + to_string(m);
        _totalDemandCapRng.add(IloRange(_env, 0, expr, 10000, name.c_str()));
        expr.end();
        totalDemandCount++;
    }
    _model.add(_totalDemandCapRng);

    if (_verbose) {
        auto c9End = chrono::steady_clock::now();
        cout << "  - 总需求容量约束: " << totalDemandCount << " 个, 用时: "
            << chrono::duration<double>(c9End - c9Start).count() << "秒" << endl;
    }

    auto endTime = chrono::steady_clock::now();

        // ========== 新增约束：到达时间定义 ==========
    // arrival_ft = (d_ft / v_f) * y_ft

    auto cArrivalStart = chrono::steady_clock::now();
    int arrivalCount = 0;

    for (int f = 0; f < _numFleets; f++) {
        Fleet* fleet = _fleetList[f];
        double speed = fleet->getOptSpeed();  // 获取飞机速度
        
        for (int t = 0; t < _numTargets; t++) {
            // 计算距离
            double distance = getDistance(
                fleet->getPos().first, fleet->getPos().second,
                _targetList[t]->getPosition().first, _targetList[t]->getPosition().second
            );
            
            // 计算到达时间 = 距离 / 速度
            double arrivalTime = distance / speed;
            
            int yi = yIdx(f, t);
            if (yi >= 0 && yi < _y_ft.getSize()) {
                IloExpr expr(_env);
                expr = _arrival_ft[f][t] - arrivalTime * _y_ft[yi];
                
                string name = "arrival_def_f" + to_string(f) + "_t" + to_string(t);
                IloRange rng = IloRange(_env, 0, expr, 0, name.c_str());
                _model.add(rng);
                expr.end();
                arrivalCount++;
            }
        }
    }

    if (_verbose) {
        auto cArrivalEnd = chrono::steady_clock::now();
        cout << "  - 到达时间定义约束: " << arrivalCount << " 个, 用时: "
             << chrono::duration<double>(cArrivalEnd - cArrivalStart).count() << "秒" << endl;
    }

    // ========== 新：路径流守恒与时间累积约束 ==========
    int numNodes = _numTargets + 1;  // 0:基地, 1..numTargets

    for (int f = 0; f < _numFleets; ++f) {
        Fleet* fleet = _fleetList[f];
        double speed = fleet->getOptSpeed();           // 公里/分钟
        double costPerKm = fleet->getOilPerMin() / speed;  // 元/公里

        // (1) 流守恒：从基地出发的次数 = 服务的总目标数
        IloExpr outFromBase(_env);
        for (int t = 1; t < numNodes; ++t) {
            outFromBase += _z_fst[f][0][t];
        }
        IloExpr sumY(_env);
        for (int t = 0; t < _numTargets; ++t) {
            sumY += _y_ft[yIdx(f, t)];
        }
        _model.add(outFromBase == sumY);

        // (2) 每个目标入度 = y
        for (int t = 1; t < numNodes; ++t) {
            IloExpr inExpr(_env), outExpr(_env);
            for (int s = 0; s < numNodes; ++s) {
                if (s == t) continue;
                inExpr += _z_fst[f][s][t];
                outExpr += _z_fst[f][t][s];
            }
            _model.add(inExpr == _y_ft[yIdx(f, t - 1)]);
            _model.add(outExpr == _y_ft[yIdx(f, t - 1)]);
        }

        // (3) 返回基地的流等于出发流
        IloExpr backToBase(_env);
        for (int t = 1; t < numNodes; ++t) {
            backToBase += _z_fst[f][t][0];
        }
        _model.add(backToBase == sumY);

        // (4) MTZ 子回路消除
        for (int i = 1; i < numNodes; ++i) {  // 目标节点 i
            int tIdx = i - 1;
            _model.add(_order_ft[f][tIdx] >= 1 * _y_ft[yIdx(f, tIdx)]);
            _model.add(_order_ft[f][tIdx] <= _numTargets * _y_ft[yIdx(f, tIdx)]);
        }
        for (int i = 1; i < numNodes; ++i) {
            for (int j = 1; j < numNodes; ++j) {
                if (i == j) continue;
                int sIdx = i - 1, tIdx = j - 1;
                IloExpr expr(_env);
                expr = _order_ft[f][sIdx] - _order_ft[f][tIdx] + 1;
                _model.add(expr <= (_numTargets + 1) * (1 - _z_fst[f][i][j]));
                expr.end();
            }
        }

        // (5) 到达时间累积
        // 从基地到第一个目标
        for (int t = 1; t < numNodes; ++t) {
            int tIdx = t - 1;
            IloExpr expr(_env);
            expr = _arrival_ft[f][tIdx] - (_distMatrix[0][t] / speed) * _y_ft[yIdx(f, tIdx)];
            _model.add(expr >= 0);
            expr.end();
        }
        // 目标之间的传递
        for (int s = 1; s < numNodes; ++s) {
            for (int t = 1; t < numNodes; ++t) {
                if (s == t) continue;
                int sIdx = s - 1, tIdx = t - 1;
                IloExpr expr(_env);
                expr = _arrival_ft[f][tIdx] - _arrival_ft[f][sIdx] - (_distMatrix[s][t] / speed);
                _model.add(expr >= -_bigM * (1 - _z_fst[f][s][t]));
                expr.end();
            }
        }

        // (6) 总飞行距离累积
        IloExpr totalDist(_env);
        for (int s = 0; s < numNodes; ++s) {
            for (int t = 0; t < numNodes; ++t) {
                if (s == t) continue;
                totalDist += _distMatrix[s][t] * _z_fst[f][s][t];
            }
        }
        _model.add(_dist_f[f] == totalDist);
        totalDist.end();

        // (7) 目标函数中的运输成本部分：costPerKm * dist_f
        // 将在目标函数中设置系数
        _obj.setLinearCoef(_dist_f[f], costPerKm);
    }

    // ========== 最晚到达时间约束 ==========
    for (int f = 0; f < _numFleets; ++f) {
        for (int t = 0; t < _numTargets; ++t) {
            IloExpr expr(_env);
            expr = _maxArrival - _arrival_ft[f][t];
            _model.add(expr >= 0);
            expr.end();
        }
    }
    // 目标函数中加上时间权重
    _obj.setLinearCoef(_maxArrival, _weightTourTime);

    for (int f = 0; f < _numFleets; ++f) {
        IloExpr sumY(_env);
        for (int t = 0; t < _numTargets; ++t) {
            sumY += _y_ft[yIdx(f, t)];
        }
        _model.add(_w_f[f] <= sumY);
        _model.add(sumY <= _bigM * _w_f[f]);   // 使用 bigM 代替除法
        sumY.end();
    }

    if (_verbose) {
        cout << "约束创建完成，总用时: " << chrono::duration<double>(endTime - startTime).count() << "秒" << endl;
    }
}

void DirectSolver::setSolverParameters() {
    _cplex.setParam(IloCplex::TiLim, _timeLimit);
    _cplex.setParam(IloCplex::EpGap, _gapTolerance);
    _cplex.setParam(IloCplex::Threads, _threads);
    _cplex.setParam(IloCplex::WorkMem, _workMem);
    _cplex.setParam(IloCplex::NodeFileInd, 2);
    _cplex.setParam(IloCplex::MIPEmphasis, IloCplex::MIPEmphasisBalanced);

    if (_verbose) {
        cout << "\n求解器参数:" << endl;
        cout << "  - 时间上限: " << _timeLimit << "秒" << endl;
        cout << "  - 间隙容忍度: " << _gapTolerance * 100 << "%" << endl;
        cout << "  - 线程数: " << (_threads == 0 ? "自动" : to_string(_threads)) << endl;
        cout << "  - 内存限制: " << _workMem << "MB" << endl;
    }
}

// ==================== 求解方法 ====================

void DirectSolver::initModel() {
    _startTime = chrono::steady_clock::now();

    _model = IloModel(_env);
    _obj = IloAdd(_model, IloMinimize(_env));

    // 初始化所有约束数组
    _targetCoverRng = IloRangeArray(_env);
    _fleetSingleRng = IloRangeArray(_env);
    _mountCapacityRng = IloRangeArray(_env);
    _loadCapacityRng = IloRangeArray(_env);
    _fleetCapacityRng = IloRangeArray(_env);

    _demandSatisfyRng = IloRangeArray(_env);
    _capacityRng = IloRangeArray(_env);
    _resourceTypeRng = IloRangeArray(_env);
    _targetTypeRng = IloRangeArray(_env);
    _linkXVRng = IloRangeArray(_env);
    _linkXURng = IloRangeArray(_env);
    _totalDemandCapRng = IloRangeArray(_env);  // 新增

    createVariables();
    createConstraints();
}

bool DirectSolver::solve() {
    _cplex = IloCplex(_model);
    setSolverParameters();

    if (_verbose) {
        cout << "\n开始求解..." << endl;
    }

    auto solveStart = chrono::steady_clock::now();

    bool solved = false;
    try {
        solved = _cplex.solve();
        _solveStatus = _cplex.getStatus();
    }
    catch (const IloException& e) {
        cerr << "CPLEX异常: " << e.getMessage() << endl;
        _solveStatus = IloAlgorithm::Error;
        return false;
    }
    catch (const exception& e) {
        cerr << "标准异常: " << e.what() << endl;
        _solveStatus = IloAlgorithm::Error;
        return false;
    }

    auto solveEnd = chrono::steady_clock::now();
    _solveTime = chrono::duration<double>(solveEnd - solveStart).count();

    if (solved) {
        _objValue = _cplex.getObjValue();

        if (_verbose) {
            cout << "\n===== 变量值诊断 =====" << endl;

            // 检查前几个x变量的值
            for (int i = 0; i < min(10, (int)_x_ftm_flat.getSize()); i++) {
                double val = _cplex.getValue(_x_ftm_flat[i]);
                if (val > 1e-6) {
                    cout << "x[" << i << "] = " << val << endl;
                }
            }

            // 检查z变量的值
            for (int t = 0; t < min(10, _numTargets); t++) {
                double zVal = _cplex.getValue(_z_t[t]);
                cout << "z_t" << t << " = " << zVal << endl;
            }

            // 检查y变量的值
            for (int i = 0; i < min(10, (int)_y_ft.getSize()); i++) {
                double yVal = _cplex.getValue(_y_ft[i]);
                if (yVal > 1e-6) {
                    cout << "y[" << i << "] = " << yVal << endl;
                }
            }

            // 输出最晚到达时间
            double maxArrivalVal = _cplex.getValue(_maxArrival);
            cout << "maxArrival = " << fixed << setprecision(2) << maxArrivalVal << endl;

            // 输出前几个到达时间
            for (int f = 0; f < min(3, _numFleets); f++) {
                for (int t = 0; t < min(3, _numTargets); t++) {
                    if (_cplex.isExtracted(_arrival_ft[f][t])) {
                        double arrivalVal = _cplex.getValue(_arrival_ft[f][t]);
                        if (arrivalVal > 1e-6) {
                            cout << "arrival_f" << f << "_t" << t << " = " << arrivalVal << endl;
                        }
                    }
                }
            }

            cout << "\n求解成功！" << endl;
            cout << "  - 目标值: " << fixed << setprecision(2) << _objValue << endl;
            cout << "最晚到达时间: " << fixed << setprecision(2) << maxArrivalVal << " 秒" << endl;
            cout << "  - 求解时间: " << fixed << setprecision(2) << _solveTime << "秒" << endl;
            cout << "  - 求解状态: " << _solveStatus << endl;
            cout << "  - 间隙: " << _cplex.getMIPRelativeGap() * 100 << "%" << endl;
        }
        return true;
    }
    else {
        if (_verbose) {
            cout << "\n求解失败！" << endl;
            cout << "  - 求解状态: " << _solveStatus << endl;
        }
        return false;
    }
}

vector<Tour*> DirectSolver::extractSolution() {
    vector<Tour*> tours;

    if (_solveStatus != IloAlgorithm::Optimal &&
        _solveStatus != IloAlgorithm::Feasible &&
        !(_cplex.isPrimalFeasible())) {
        cerr << "没有可行解可提取" << endl;
        return tours;
    }

    if (_verbose) {
        cout << "\n===== 提取求解结果 =====" << endl;
        // 输出最晚到达时间
        double maxArrivalVal = _cplex.getValue(_maxArrival);
        cout << "最晚到达时间: " << fixed << setprecision(2) << maxArrivalVal << "秒" << endl;
    }

    // 记录未覆盖目标
    vector<Target*> uncoverTargets;

    // 输出每个目标的选择和分配情况
    for (int t = 0; t < _numTargets; t++) {
        double zVal = _cplex.getValue(_z_t[t]);
        bool covered = (zVal < 0.5);

        if (covered) {
            if (_verbose) {
                cout << "\n目标" << _targetList[t]->getName() << " 被覆盖 (z=" << zVal << ")" << endl;

                // 输出目标选择的类型
                for (int m : _targetFeasibleM[t]) {
                    double uVal = _cplex.getValue(_u_tm_flat[uIdx(t, m)]);
                    if (uVal > 0.5) {
                        cout << "  选择类型: " << _mountLoadList[m]->_mount->getName()
                            << "-" << _mountLoadList[m]->_load->getName() << endl;

                        // ========== 修改点：使用 getXIndex 替代 xIdx ==========
                        // 输出分配详情
                        for (int f = 0; f < _numFleets; f++) {
                            int idx = getXIndex(f, t, m);
                            if (idx >= 0) {
                                double val = _cplex.getValue(_x_ftm_flat[idx]);
                                if (val > 0.01) {  // 考虑浮点误差，用0.01而不是0.5
                                    cout << "    从资源" << _fleetList[f]->getName()
                                        << " 获得 " << val << " 个" << endl;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        else {
            uncoverTargets.push_back(_targetList[t]);
            if (_verbose) {
                cout << "目标" << _targetList[t]->getName() << " 未覆盖 (z=" << zVal << ")" << endl;
            }
        }
    }

    // 输出每个资源的选择情况
    if (_verbose) {
        cout << "\n资源选择情况:" << endl;
        for (int f = 0; f < _numFleets; f++) {
            for (int m : _fleetFeasibleM[f]) {
                double vVal = _cplex.getValue(_v_gm_flat[vIdx(f, m)]);
                if (vVal > 0.5) {
                    cout << "资源" << _fleetList[f]->getName()
                        << " 选择类型: " << _mountLoadList[m]->_mount->getName()
                        << "-" << _mountLoadList[m]->_load->getName() << endl;
                    break;
                }
            }
        }
    }
    // 计算并输出飞行总成本（固定出动成本 + 距离运输成本）
    double totalFlightCost = 0.0;
    cout << "\n===== 飞行成本明细（固定+运输）=====" << endl;
    for (int f = 0; f < _numFleets; ++f) {
        double wVal = _cplex.getValue(_w_f[f]);                // 是否出动
        double fixedCost = wVal * _fleetList[f]->getCostUse(); // 固定出动成本
        double distVal = _cplex.getValue(_dist_f[f]);          // 总飞行距离（公里）
        double speed = _fleetList[f]->getOptSpeed();           // 公里/分钟
        double costPerKm = _fleetList[f]->getOilPerMin() / speed; // 元/公里
        double transportCost = distVal * costPerKm;            // 运输成本
        totalFlightCost += fixedCost + transportCost;

        if (_verbose && (fixedCost > 1e-6 || transportCost > 1e-6)) {
            cout << "  飞机 " << _fleetList[f]->getName()
                << ": 出动=" << wVal
                << ", 固定成本=" << fixedCost
                << ", 距离=" << distVal << " km"
                << ", 运输成本=" << transportCost << endl;
        }
    }
    cout << "飞行总成本（固定+运输）: " << totalFlightCost << endl;
    cout << "=================================" << endl;
    
    // 输出每个资源的访问顺序（基于弧变量）
    cout << "\n===== 资源访问顺序 =====" << endl;
    int numNodes = _numTargets + 1;
    for (int f = 0; f < _numFleets; ++f) {
        // 检查该飞机是否有任务（通过 w_f 或检查是否存在出弧）
        double wVal = _cplex.getValue(_w_f[f]);
        if (wVal < 0.5) continue;

        // 构建从基地出发的路径
        vector<int> path;          // 存储节点索引，0=基地，1..numTargets=目标
        vector<double> segDist;    // 分段距离（公里）
        vector<double> segTime;    // 分段时间（分钟）

        // 找到第一条从基地出发的弧
        int currentNode = 0;       // 从基地开始
        path.push_back(currentNode);
        bool hasNext = true;
        while (hasNext) {
            int nextNode = -1;
            for (int t = 1; t < numNodes; ++t) {
                if (t != currentNode) {
                    double val = _cplex.getValue(_z_fst[f][currentNode][t]);
                    if (val > 0.5) {
                        nextNode = t;
                        break;
                    }
                }
            }
            if (nextNode == -1) {
                // 如果没有出弧，可能已经回到基地或路径结束
                break;
            }
            // 记录距离和时间
            double dist = _distMatrix[currentNode][nextNode];
            double speed = _fleetList[f]->getOptSpeed();
            double time = dist / speed;
            segDist.push_back(dist);
            segTime.push_back(time);

            path.push_back(nextNode);
            currentNode = nextNode;
        }

        // 最后应该有一条回到基地的弧（除非服务目标数为0，但已被过滤）
        if (currentNode != 0) {
            // 寻找回到基地的弧
            double backVal = _cplex.getValue(_z_fst[f][currentNode][0]);
            if (backVal > 0.5) {
                double dist = _distMatrix[currentNode][0];
                double speed = _fleetList[f]->getOptSpeed();
                double time = dist / speed;
                segDist.push_back(dist);
                segTime.push_back(time);
                path.push_back(0);
            }
        }

        // 输出路径
        cout << "飞机 " << _fleetList[f]->getName() << " 的访问顺序: ";
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == 0) {
                cout << "基地";
            }
            else {
                int tIdx = path[i] - 1;
                cout << _targetList[tIdx]->getName();
            }
            if (i < path.size() - 1) cout << " -> ";
        }
        cout << endl;

        // 输出累计距离和时间
        double totalDist = 0.0, totalTime = 0.0;
        for (size_t i = 0; i < segDist.size(); ++i) {
            totalDist += segDist[i];
            totalTime += segTime[i];
        }
        cout << "   总距离: " << fixed << setprecision(2) << totalDist << " km, "
            << "总时间: " << totalTime << " 分钟" << endl;
    }
    cout << "=========================" << endl;
    // 设置未覆盖目标列表
    _schedule->setUncoverTList(uncoverTargets);

    return tours;
}

vector<Tour*> DirectSolver::run() {
    initModel();
    if (solve()) {
        // 补充输出最终结果的总成本和最晚到达时间
        double maxArrivalVal = _cplex.getValue(_maxArrival);
        cout << "\n===== 最终结果 =====" << endl;
        cout << "总成本 (目标值): " << fixed << setprecision(2) << _objValue << endl;
        cout << "最晚到达时间: " << fixed << setprecision(2) << maxArrivalVal << " 秒" << endl;
        cout << "===================" << endl;
        return extractSolution();
    }
    return vector<Tour*>();
}

vector<Tour*> DirectSolver::runMultiRound() {
    vector<Tour*> allTours;               // 合并所有轮次的 tours
    double totalCost = 0.0;              // 总成本
    double totalMaxArrival = 0.0;        // 总最晚到达时间（累加）
    int round = 0;

    // 当前待覆盖的目标列表（初始为全部目标）
    vector<Target*> remainingTargets = _targetList;

    while (!remainingTargets.empty()) {
        round++;
        if (_verbose) {
            cout << "\n========== 第 " << round << " 轮求解 ==========" << endl;
            cout << "本轮剩余目标数: " << remainingTargets.size() << endl;
        }

        // 1. 临时替换目标列表为当前剩余目标
        vector<Target*> originalTargets = _targetList;
        int originalNumTargets = _numTargets;
        _targetList = remainingTargets;
        _numTargets = _targetList.size();

        // 2. 重新预计算映射和距离矩阵
        precomputeMappings();

        int numNodes = _numTargets + 1;
        _distMatrix.assign(numNodes, vector<double>(numNodes, 0.0));
        Base* base = _schedule->getBase();
        GeoPoint basePos(base->getLongitude(), base->getLatitude());
        for (int t = 0; t < _numTargets; ++t) {
            auto pos = _targetList[t]->getPosition();
            GeoPoint target(pos.first, pos.second);
            double d = TSPSolver::calculateDistance(basePos, target);
            _distMatrix[0][t + 1] = _distMatrix[t + 1][0] = d;
        }
        for (int i = 0; i < _numTargets; ++i) {
            for (int j = i + 1; j < _numTargets; ++j) {
                auto pos1 = _targetList[i]->getPosition();
                auto pos2 = _targetList[j]->getPosition();
                GeoPoint p1(pos1.first, pos1.second);
                GeoPoint p2(pos2.first, pos2.second);
                double d = TSPSolver::calculateDistance(p1, p2);
                _distMatrix[i + 1][j + 1] = _distMatrix[j + 1][i + 1] = d;
            }
        }

        // 3. 重建模型（释放旧模型，创建新模型）
        _model.end();               // 释放原有模型资源
        _env.end();                 // 注意：env 不能完全结束，后面还要用，这里重新初始化
        _env = IloEnv();            // 重新创建环境
        _model = IloModel(_env);
        _obj = IloAdd(_model, IloMinimize(_env));

        // 重新初始化约束数组（需要重新分配）
        _targetCoverRng = IloRangeArray(_env);
        _fleetSingleRng = IloRangeArray(_env);
        _mountCapacityRng = IloRangeArray(_env);
        _loadCapacityRng = IloRangeArray(_env);
        _fleetCapacityRng = IloRangeArray(_env);
        _demandSatisfyRng = IloRangeArray(_env);
        _capacityRng = IloRangeArray(_env);
        _resourceTypeRng = IloRangeArray(_env);
        _targetTypeRng = IloRangeArray(_env);
        _linkXVRng = IloRangeArray(_env);
        _linkXURng = IloRangeArray(_env);
        _totalDemandCapRng = IloRangeArray(_env);

        // 重新创建变量和约束
        createVariables();
        createConstraints();

        // 4. 求解本轮模型
        if (!solve()) {
            cerr << "第 " << round << " 轮求解失败！" << endl;
            break;
        }

        // 5. 累加成本和最晚到达时间
        totalCost += _objValue;
        double roundMaxArrival = _cplex.getValue(_maxArrival);
        totalMaxArrival += roundMaxArrival;

        // 6. 提取本轮解（tours 和未覆盖目标）
        vector<Tour*> roundTours = extractSolution();
        allTours.insert(allTours.end(), roundTours.begin(), roundTours.end());

        // 获取本轮未覆盖的目标
        vector<Target*> uncovered = _schedule->getUncoverTList();

        // 7. 恢复原始目标列表（为下一轮循环做准备）
        _targetList = originalTargets;
        _numTargets = originalNumTargets;

        // 更新剩余目标为未覆盖目标
        remainingTargets = uncovered;

        // 可选：清理本轮 CPLEX 对象，避免内存泄漏
        _cplex.end();
    }

    if (_verbose) {
        cout << "\n========== 多轮求解完成 ==========" << endl;
        cout << "总轮数: " << round << endl;
        cout << "总成本: " << fixed << setprecision(2) << totalCost << endl;
        cout << "总最晚到达时间: " << fixed << setprecision(2) << totalMaxArrival << " 秒" << endl;
    }

    // 恢复最终模型环境（可选，如果后续不再使用）
    // 注意：此时 _env 已被重建，原 schedule 中的目标列表恢复，但模型已变。
    // 如果需要在外部继续使用，请谨慎。

    return allTours;
}


void DirectSolver::exportModel(const string& filename) {
    _cplex.exportModel(filename.c_str());
    if (_verbose) {
        cout << "模型已导出到: " << filename << endl;
    }
}

int DirectSolver::getVariableCount() const {
    return _cplex.getNcols();
}

int DirectSolver::getConstraintCount() const {
    return _cplex.getNrows();
}