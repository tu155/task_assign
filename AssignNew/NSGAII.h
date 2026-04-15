#ifndef NSGAII_H
#define NSGAII_H
#include "Util.h"
#include "Individual.h"
#include <vector>
#include <memory>
#include <random>

class NSGAII {
private:
    Schedule* _schedule;
    int population_size;
    int max_generations;
    double crossover_rate;
    double mutation_rate;

    std::vector<Individual> population;

    //std::random_device rd;
    //std::mt19937 gen;
    std::mt19937 gen = Util::get_generator();
    std::uniform_real_distribution<double> dis;

    //result
    map<string, vector<Tour*>> _result;
    map<string, map<string, float>> _objectives;
public:
    // 构造函数
    NSGAII(
        Schedule* schedule,
        int pop_size = -1,// 种群大小，-1表示使用Util::nsgaii_population_size
        int max_gen = -1,// 最大迭代次数，-1表示使用Util::nsgaii_max_generations
        double cross_rate = -1,// 交叉概率，-1.0表示使用Util::nsgaii_crossover_rate
        double mut_rate = -1);// 变异概率，-1.0表示使用Util::nsgaii_mutation_rate


    // 主运行函数
    void run();

    // 获取结果
    std::vector<Individual> getParetoFront() const;
    std::vector<std::vector<double>> getObjectiveValues() const;
    void printStatistics() const;
    void printParetoFront(int max_display = 10) const;
    void saveMultiResult();
    map<string, vector<Tour*>>getResult() {return _result;};
    map<string, map<string, float>>getObjectives() { return _objectives; };
private:
    // 初始化种群
    void initializePopulation();

    // 评估种群
    void evaluatePopulation(std::vector<Individual>& pop);
    void evaluatePopulationFast(std::vector<Individual>& pop);
    
    // NSGA-II核心操作
    void fastNonDominatedSort(std::vector<Individual>& pop);                // 快速非支配排序
    void calculateCrowdingDistance(std::vector<Individual>& pop,            // 计算拥挤度距离
        const std::vector<int>& front_indices);
    std::vector<Individual> selection(const std::vector<Individual>& pop);          // 选择操作
    void twoPointCrossover(Individual& parent1, Individual& parent2);            // 两点交叉
    void capacityAwareMutation(Individual& ind);                       // 容量感知变异
    std::vector<Individual> generateOffspring(std::vector<Individual>& parents);                // 生成子代

    void check(Individual ind);           //检查最终优化结果是否满足约束
    // 辅助函数
    std::vector<int> getFrontIndices(const std::vector<Individual>& pop, int rank) const;           // 获取某一等级的非支配解的索引
    int getMaxRank(const std::vector<Individual>& pop) const;                   // 获取种群中最大等级
};

#endif // NSGAII_H