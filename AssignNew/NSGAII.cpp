#include "NSGAII.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>

// 修改构造函数，使用Util中的参数
NSGAII::NSGAII(Schedule* schedule,
    int pop_size, int max_gen, double cross_rate, double mut_rate)
    :_schedule(schedule),
    population_size(pop_size == -1 ? Util::nsgaii_population_size : pop_size),
    max_generations(max_gen == -1 ? Util::nsgaii_max_generations : max_gen),
    crossover_rate(cross_rate == -1.0 ? Util::nsgaii_crossover_rate : cross_rate),
    mutation_rate(mut_rate == -1.0 ? Util::nsgaii_mutation_rate : mut_rate) {


    initializePopulation();
}

// 修改初始化种群函数
void NSGAII::initializePopulation() {
    population.clear();

    for (int i = 0; i < population_size; ++i) {
        // 群体生成策略：30%可能性时间最短、20%消耗最少或50%随机选择
        uniform_real_distribution<double> dis(0.0, 1.0);
        double randomValue = dis(gen);
        if (randomValue < Util::nsgaii_init_timeShortest_ratio) {
            Individual ind("timeShortest", _schedule);
            ind.encode();
            ind.getAllocation();
            for (auto& [key, value] : ind._fleetAllTargetNum) {
                Fleet*fleet = key;
                map<Target*, int> TargetNum = value;
                //if (value.size() > 1) {
                //    cout << "资源分配给多个目标" << endl;
                //}
            }
            population.push_back(ind);
        }
        else if (randomValue < Util::nsgaii_init_timeShortest_ratio + Util::nsgaii_init_costLowest_ratio) {
            Individual ind("costLowest", _schedule);
            ind.encode();
            population.push_back(ind);
        }
        else {
            Individual ind("Normal", _schedule);
            ind.encode();
            population.push_back(ind);
        }
    }

    evaluatePopulationFast(population);
    //evaluatePopulation(population);
}
//原串行方法已弃用
void NSGAII::evaluatePopulation(std::vector<Individual>& pop) {
    for (auto& ind : pop) {
        ind.decode();
        check(ind);
        ind.cocuSumCostTime();
        cout<< "SumCost" << ind._SumCost << endl;
        //ind.cocuSumTime();
        cout<< "SumTime" << ind._SumTime << endl;
        ind.cocuSumDifference();
        ind._objectives.push_back(ind._SumTime);
        ind._objectives.push_back(ind._SumCost);
        if (ind._objCount == 3) {
            ind._objectives.push_back(ind._SumDifference);
        }
    }
    cout << "计算初始种群的适应度时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
}

// 2. 并行计算适应度
void NSGAII::evaluatePopulationFast(std::vector<Individual>& pop) {
#pragma omp parallel for
    for (auto& ind : pop) {

        ind.decode();
        ind.computeObjectives();
    }
    cout << "并行计算初始种群的适应度时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
}
void NSGAII::fastNonDominatedSort(std::vector<Individual>& pop) {
    std::vector<std::vector<int>> fronts;
    fronts.push_back(std::vector<int>());

    // 第一遍：计算支配关系
    for (int i = 0; i < pop.size(); ++i) {
        pop[i]._dominatedSet.clear();
        pop[i]._dominatedCount = 0;

        for (int j = 0; j < pop.size(); ++j) {
            if (i == j) continue;

            if (pop[i].dominates(pop[j])) {
                pop[i]._dominatedSet.push_back(j);
            }
            else if (pop[j].dominates(pop[i])) {
                pop[i]._dominatedCount++;
            }
        }

        if (pop[i]._dominatedCount == 0) {
            pop[i]._rank = 1;
            fronts[0].push_back(i);
        }
    }

    // 构建后续前沿
    int current_front = 0;
    while (!fronts[current_front].empty()) {
        std::vector<int> next_front;

        for (int p_idx : fronts[current_front]) {
            for (int q_idx : pop[p_idx]._dominatedSet) {
                pop[q_idx]._dominatedCount--;
                if (pop[q_idx]._dominatedCount == 0) {
                    pop[q_idx]._rank = current_front + 2;
                    next_front.push_back(q_idx);
                }
            }
        }

        current_front++;
        fronts.push_back(next_front);
    }
}

void NSGAII::calculateCrowdingDistance(std::vector<Individual>& pop,
    const std::vector<int>& front_indices) {
    if (front_indices.empty()) return;

    int front_size = front_indices.size();
    int obj_count = population[0]._objCount;

    // 初始化拥挤度
    for (int idx : front_indices) {
        pop[idx]._crowdingDistance = 0.0;
    }

    // 对每个目标计算拥挤度
    for (int obj_idx = 0; obj_idx < obj_count; ++obj_idx) {
        // 按当前目标排序
        std::vector<int> sorted_front = front_indices;
        std::sort(sorted_front.begin(), sorted_front.end(),
            [&](int a, int b) {
                return pop[a]._objectives[obj_idx] < pop[b]._objectives[obj_idx];
            });

        // 边界个体拥挤度为无穷大
        pop[sorted_front[0]]._crowdingDistance = std::numeric_limits<double>::max();
        pop[sorted_front[front_size - 1]]._crowdingDistance = std::numeric_limits<double>::max();

        // 计算中间个体的拥挤度
        double min_obj = pop[sorted_front[0]]._objectives[obj_idx];
        double max_obj = pop[sorted_front[front_size - 1]]._objectives[obj_idx];

        if (max_obj - min_obj < nsgaii::GA_EPSILON) continue;

        for (int i = 1; i < front_size - 1; ++i) {
            double distance = (pop[sorted_front[i + 1]]._objectives[obj_idx] -
                pop[sorted_front[i - 1]]._objectives[obj_idx]) / (max_obj - min_obj);
            pop[sorted_front[i]]._crowdingDistance += distance;
        }
    }
}

std::vector<Individual> NSGAII::selection(const std::vector<Individual>& pop) {
    std::vector<Individual> selected;
    selected.reserve(pop.size());  // 预分配内存，避免重复扩容
    std::uniform_int_distribution<size_t> dist(0, pop.size() - 1);
    while (selected.size() < pop.size()) {
        //int idx1 = rand() % pop.size();
        //int idx2 = rand() % pop.size();
        // 一次生成两个随机索引
        size_t idx1 = dist(gen);
        size_t idx2 = dist(gen);

        const Individual& ind1 = pop[idx1];
        const Individual& ind2 = pop[idx2];

        // 锦标赛选择
        if (ind1._rank < ind2._rank) {
            selected.push_back(ind1);
        }
        else if (ind1._rank > ind2._rank) {
            selected.push_back(ind2);
        }
        else {
            if (ind1._crowdingDistance > ind2._crowdingDistance) {
                selected.push_back(ind1);
            }
            else {
                selected.push_back(ind2);
            }
        }
    }

    return selected;
}

/**
 * 模拟两点交叉操作，用于生成两个新的子代个体
 * @param parent1 第一个父代个体
 * @param parent2 第二个父代个体
 */
void NSGAII::twoPointCrossover(Individual& parent1, Individual& parent2) {
    
    if (dis(gen) > crossover_rate) return;
    std::uniform_int_distribution<size_t> dist(0, _schedule->getTargetTypeList().size() - 1);
    
    //int chromosome_length = parent1.getVariableCount();

    //// 如果染色体长度小于2，无法进行两点交叉
    //if (chromosome_length < 2) return;

    //// 随机选择两个交叉点
    //int point1 = rand() % chromosome_length;
    //int point2 = rand() % chromosome_length;

    //随机选择两个目标进行交叉，从而确保交叉后，目标需求满足并且飞机装载量不超过最大载重
    //选择同一类型的目标进行交叉
    //int targetTypeIdx=rand() % _schedule->getTargetTypeList().size();
    int targetTypeIdx= dist(gen);
    std::vector<Target*> targetList = _schedule->getTargetTypeList()[targetTypeIdx]->getTargetList();
    std::uniform_int_distribution<size_t> dist1(0, targetList.size() - 1);
    int target1Idx = dist1(gen);
    int target2Idx = dist1(gen);
    //int target1Idx= rand() % targetList.size();
    //int target2Idx = rand() % targetList.size();
    //// 确保 target1Idx <= target2Idx
    //if (target1Idx > target2Idx) {
    //    std::swap(target1Idx, target2Idx);
    //}
    // 计算两个交叉点之间的基因段
    int point1;
    int point2;
    if (target1Idx == 0) {
        point1 = 0;
    }
    else {
        point1 = parent1._partPosition[target1Idx - 1];
    }
    point2 = parent1._partPosition[target1Idx];
    
    int point3;
    int point4;
    if (target2Idx == 0) {
        point3 = 0;
    }
    else {
        point3 = parent1._partPosition[target2Idx - 1];
    }
    point4 = parent1._partPosition[target2Idx];

    // 交换两个点之间的基因段
    for (int i = point1; i <= point2; ++i) {
        std::swap(parent1._variables[i], parent2._variables[i]);
    }
    // 交换两个点之间的基因段
    for (int i = point3; i <= point4; ++i) {
        std::swap(parent1._variables[i], parent2._variables[i]);
    }
    //cout << "检查交叉后的染色体" << endl;
    //check(parent1);
    //check(parent2);
    // 可行性检查（确保交叉后的值在合理范围内）
    //parent1.getAllocation();
    //parent2.getAllocation();//先计算分配数量，再检查
    parent1.repairFeasibility();
    parent2.repairFeasibility();

}

void NSGAII::capacityAwareMutation(Individual& ind) {
    //double mutation_prob = 1.0 / ind._variables.size();
    ////将遍历所有基因位修改为遍历个体
    //for (int i = 0; i < population[0]._variables.size(); ++i) {
    //    if (dis(gen) < mutation_rate) {
    //随机生成变异位置
    int mutation_position = rand() % ind._variables.size();
    //计算这个位置的飞机的最大货物数量
    Target* target = get<0>(ind._idxMeaning[mutation_position]);
    Mount* mount = get<1>(ind._idxMeaning[mutation_position]);
    Fleet* fleet = get<2>(ind._idxMeaning[mutation_position]);
    int fleetCapacity = fleet->getMountNum(mount);
    //计算这个飞机的累计货物数量
    int currentCargoNum = ind._fleetAllocation[make_pair(fleet, mount)];
    int beforeNum = currentCargoNum - ind._variables[mutation_position];
    int maxNum = fleetCapacity - beforeNum;
    //重新生成这个位置上的飞机货物数量，范围为0-maxNum，左右边界都包含
    int newNum = rand() % (maxNum + 1);
    //cout << "变异前货物数量为 " << ind._variables[mutation_position] << " 变异后货物数量为 " << newNum << endl;
    ind._variables[mutation_position] = newNum;
    //更新累计货物数量
    ind._fleetAllocation[make_pair(fleet, mount)] = beforeNum + newNum;
    
    //可行性检查（确保变异后的值在合理范围内）
    //ind.getAllocation();
    ind.repairFeasibility();
}

std::vector<Individual> NSGAII::generateOffspring(std::vector<Individual>& parents) {
    std::vector<Individual> offspring;
    offspring.reserve(parents.size());  // 预分配内存
    // 交叉和变异
    for (size_t i = 0; i < parents.size(); i += 2) {
        if (i + 1 >= parents.size()) break;

        Individual child1 = parents[i];
        Individual child2 = parents[i + 1];
        //cout << "父母代数："<<i << endl;
        twoPointCrossover(child1, child2);
        //cout << "交叉修复后的检查" << endl;
        //check(child1);
        //check(child2);
        
        if (dis(gen) < mutation_rate){
            capacityAwareMutation(child1);
            //cout << "子代1容量满足下变异的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
        }
        if (dis(gen) < mutation_rate) {
            capacityAwareMutation(child2);
            //cout << "子代2容量满足下变异的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
        }
        //cout << "变异后的检查" << endl;
        //check(child1);
        //check(child2);
        //更新子代的目标函数值
        child1.decode();
        child1.computeObjectives();
        child2.decode();
        child2.computeObjectives();

        offspring.push_back(child1);
        offspring.push_back(child2);
    }
    //如果小于交叉概率，那么直接返回父代，所以群体的数量是不变的
    //// 补充不足的个体
    //while (offspring.size() < population_size) {
    //    offspring.push_back(parents[rand() % parents.size()]);
    //}
    //cout << "补充个体的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

    //evaluatePopulation(offspring);
    //cout << "群体完成交叉变异的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

    return offspring;
}

void NSGAII::run() {
    std::cout << "开始NSGA-II优化..." << std::endl;
    std::cout << "目标数量: " << population[0]._objCount << std::endl;
    std::cout << "变量数量: " << population[0]._variables.size() << std::endl;
    std::cout << "种群大小: " << population_size << std::endl;
    std::cout << "最大代数: " << max_generations << std::endl;

    for (int gen = 0; gen < max_generations; ++gen) {
        // 选择父代
        auto parents = selection(population);
        //cout << "选择过程的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
        
        // 生成子代
        auto offspring = generateOffspring(parents);
        //cout << "生成子代的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

        // 合并种群
        std::vector<Individual> combined_population = population;
        combined_population.insert(combined_population.end(),offspring.begin(), offspring.end());
        //cout << "合并种群的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

        // 非支配排序
        fastNonDominatedSort(combined_population);
        //cout << "非支配排序的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

        // 环境选择
        population.clear();
        int current_rank = 1;

        while (population.size() < population_size) {
            auto current_front = getFrontIndices(combined_population, current_rank);

            if (population.size() + current_front.size() <= population_size) {
                // 加入整个前沿
                for (int idx : current_front) {
                    population.push_back(combined_population[idx]);
                }
            }
            else {
                // 计算拥挤度并选择部分个体
                calculateCrowdingDistance(combined_population, current_front);

                std::sort(current_front.begin(), current_front.end(),
                    [&](int a, int b) {
                        return combined_population[a]._crowdingDistance >
                            combined_population[b]._crowdingDistance;
                    });

                int remaining = population_size - population.size();
                for (int i = 0; i < remaining; ++i) {
                    population.push_back(combined_population[current_front[i]]);
                }
                break;
            }
            current_rank++;
        }
        //cout<< "计算拥挤度构建新种群的时间为 " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
        // 输出进度
        //if (gen % 10 == 0) {
        //    std::cout << "第 " << gen << " 代完成" << std::endl;
        //    printStatistics();
        //    cout << "Running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
        //}
    }

    std::cout << "优化完成！" << std::endl;
    cout << "累计运行时间为s " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

    printParetoFront();

    //记录不同决策偏好下的方案
    saveMultiResult();
}

// 辅助函数实现
std::vector<int> NSGAII::getFrontIndices(const std::vector<Individual>& pop, int rank) const {
    std::vector<int> indices;
    for (size_t i = 0; i < pop.size(); ++i) {
        if (pop[i]._rank == rank) {
            indices.push_back(i);
        }
    }
    return indices;
}

int NSGAII::getMaxRank(const std::vector<Individual>& pop) const {
    int max_rank = 0;
    for (const auto& ind : pop) {
        max_rank = std::max(max_rank, ind._rank);
    }
    return max_rank;
}

std::vector<Individual> NSGAII::getParetoFront() const {
    std::vector<Individual> pareto_front;
    for (const auto& ind : population) {
        if (ind._rank == 1) {
            pareto_front.push_back(ind);
        }
    }
    return pareto_front;
}

std::vector<std::vector<double>> NSGAII::getObjectiveValues() const {
    std::vector<std::vector<double>> objectives;
    for (const auto& ind : getParetoFront()) {
        objectives.push_back(ind._objectives);
    }
    return objectives;
}

void NSGAII::printStatistics() const {
    auto pareto_front = getParetoFront();
    std::cout << "第一层非支配解数量: " << pareto_front.size()
        << "/" << population_size << std::endl;
}

void NSGAII::printParetoFront(int max_display) const {
    auto pareto_front = getParetoFront();
    std::cout << "\n最终帕累托前沿包含 " << pareto_front.size() << " 个解:" << std::endl;

    int display_count = std::min(max_display, (int)pareto_front.size());
    for (int i = 0; i < display_count; ++i) {
        std::cout << "解 " << i + 1 << ": ";
        pareto_front[i].printObjectives();
        std::cout << std::endl;
    }

    if (pareto_front.size() > display_count) {
        std::cout << "... 还有 " << pareto_front.size() - display_count << " 个解" << std::endl;
    }
}

void NSGAII::saveMultiResult() {
    auto pareto_front = getParetoFront();
    double minCost = INFINITY;
    int minCostIdx=0;
    double minTime = INFINITY;
    int minTimeIdx=0;
    double minDifference = INFINITY;
    int minDifferenceIdx=0;
    for (int i = 0; i < pareto_front.size(); ++i) {
        pareto_front[i].computeObjectivesTotal();
        if (pareto_front[i]._objectives[1] < minCost) {
            minCost = pareto_front[i]._objectives[0];
            minCostIdx = i;
        }
        if (pareto_front[i]._objectives[0] < minTime) {
            minTime = pareto_front[i]._objectives[1];
            minTimeIdx = i;
        }
        if (Util::nsgaii_objective_count == 3 && pareto_front[i]._objectives.size() == 3) {
            if (pareto_front[i]._objectives[2] < minDifference) {
                minDifference = pareto_front[i]._objectives[2];
                minDifferenceIdx = i;
            }
        }
    }
    //记录不同偏好下的最优方案
    
    Individual ind1= pareto_front[minCostIdx];
    //ind1.decode();
    //ind1.computeObjectives();
    check(ind1);
    vector<Tour* >soluTourList1 =ind1.getsolnTourList();

    _result["minCost"] = soluTourList1;
    cout<<"-------------minCost-----------------"<<endl;
    ind1.printObjectives();
    _objectives["minCost"]["SumTime"] = ind1._objectives[0];
    _objectives["minCost"]["SumCost"] = ind1._objectives[1];
    _objectives["minCost"]["minDifference"] = ind1._objectives[2];

    Individual ind2 = pareto_front[minTimeIdx];
    //ind2.decode();
    //ind2.computeObjectives();
    vector<Tour* >soluTourList2 = ind2.getsolnTourList();
    check(ind2);
    _result["minTime"] = soluTourList2;
    cout << "-------------minTime-----------------" << endl;
    ind2.printObjectives();
    _objectives["minTime"]["SumTime"] = ind1._objectives[0];
    _objectives["minTime"]["SumCost"] = ind1._objectives[1];
    _objectives["minTime"]["minDifference"] = ind1._objectives[2];

    if (pareto_front[0]._objectives.size() == 3) {
        Individual ind3 = pareto_front[minDifferenceIdx];
        //ind3.decode();
        //ind3.computeObjectives();
        vector<Tour* >soluTourList3 = ind3.getsolnTourList();
        check(ind3);
        _result["minDifference"] = soluTourList3;
        cout << "-------------minDifference-----------------" << endl;
        ind3.printObjectives();
        _objectives["minDifference"]["SumTime"] = ind1._objectives[0];
        _objectives["minDifference"]["SumCost"] = ind1._objectives[1];
        _objectives["minDifference"]["minDifference"] = ind1._objectives[2];

    }
    
}

void NSGAII::check(Individual ind) {
    ind.getAllocation();
    //清空覆盖目标集合和未覆盖目标集合
    _schedule->clearCoverT();
    _schedule->clearUncoverT();
    //检查目标是否覆盖，如果覆盖，计算总需求量是否超出可供量，并记录
    for (Target* target : _schedule->getTargetList()) {
        Mount* mount=ind._targetMount[target];
        ind._mountNeedTotalNum[mount] += ind._targetAllocation[make_pair(target, mount)];
        if (ind._targetNeedNum[target] <= ind._targetAllocation[make_pair(target,mount)]) {
            target->setLpVal(1.0);
            _schedule->pushCoverT(target);//记录覆盖的目标
        }
        else {
            target->setLpVal(0.0);
            _schedule->pushUncoverT(target);//记录未覆盖的目标
            ind._targetUncoverNum[target] = ind._targetNeedNum[target] - ind._targetAllocation[make_pair(target, mount)];
        }
    };
    bool needContinue = false;
    if (ind._targetUncoverNum.size() > 0) {
        needContinue = true;
        cout << "error : 有目标未完全覆盖" << endl;
    }
    bool error=false;
    //如果目标未覆盖，检查剩余资源能否满足
    for (auto it = ind._targetUncoverNum.begin(); it != ind._targetUncoverNum.end(); it++) {
        if (it->second > 0) {
            Mount* mount = ind._targetMount[it->first];
            if (_schedule->getBase()->getMountList()[mount]- ind._mountNeedTotalNum[mount] > it->second) {
                error = true;
                cout<<"error : 有充足的剩余资源，但是未分配给目标" << endl;
                break;
            }
        }
    }
    //检查已经安排的飞机的装载量是否超出最大载重
    bool overweight= false;
    for (Fleet* fleet : _schedule->getFleetList()) {
        Mount* mount = ind._fleetMount[fleet];
        int mountAssignment = ind._fleetAllocation[make_pair(fleet, mount)];
        int mountNum = fleet->getMountNum(mount);
        if ( mountAssignment> mountNum) {
            overweight = true;
            cout << "error : 飞机的装载量超出最大载重" << endl;
            break;
        }
    }
}