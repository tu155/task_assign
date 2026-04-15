#include "Util.h"
#include "Target.h"
#include "MainTarget.h"
#include <chrono>
#include <thread>  // 需要这个头文件
#include <algorithm> // 需要 min 函数
inline time_t makeTime(int yr, int mon, int day, int hr = 0, int min = 0, int sec = 0)
{
    struct tm timeinfo;
    timeinfo.tm_year = yr - 1900;
    timeinfo.tm_mon = mon - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hr;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;
    return mktime(&timeinfo);
}
string Util::getUUID()
{
    static std::atomic<long long> counter(0);
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now
        .time_since_epoch()).count();

    std
        ::stringstream ss;
    ss
        << std::hex << nanos << "-" << counter.fetch_add(1);
    return ss.str();
}
// 全局或静态变量，避免重复初始化
std::mt19937& Util::get_generator() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

// ============= NSGA-II 算法参数初始化 =============
int Util::nsgaii_population_size = 10;                  // 种群大小（建议增大到50-200）
int Util::nsgaii_max_generations = 10;                  // 最大迭代次数（建议增大到100-500）
double Util::nsgaii_crossover_rate = 0.8;              // 交叉概率
double Util::nsgaii_mutation_rate = 0.01;               // 变异概率
double Util::nsgaii_init_timeShortest_ratio = 0.3;     // 时间最短策略初始化比例
double Util::nsgaii_init_costLowest_ratio = 0.2;       // 成本最低策略初始化比例
double Util::nsgaii_init_normal_ratio = 0.5;           // 随机策略初始化比例
int Util::nsgaii_objective_count = 3;                  // 优化目标数量 (2或3)

string Util::INPUTPATH = ".\\input\\";
string Util::OUTPUTPATH = ".\\output\\";
time_t Util::startRunningClock = 0;
double Util::weightTourTime = 1000;
double Util::costTargetUncover = 2000;
double Util::costTargetUncoverByOrder = 2500;
double Util::costBaseLessThanRatio = 2000;
double Util::costDistWeight = 0.2;
double Util::costAftOil = 40;
double Util::costGrpUnLoad = 50000;
//double ZZUtil::costBaseTarPref = 100;
double Util::costOverGrpPerTarNum = 20000;
double Util::revLessGrpTarNum = -10;
double Util::costDmdTarPref = 10000;
double Util::costDmdTarRate = 100;
//double Util::costFleetTypeBaseFair = 0;
//double Util::costFleetBaseFair = 200;
//double Util::costLoadBaseFair = 200;
//double Util::costMountBaseFair = 200;
double Util::costAftFailure = 200;
double Util::costTarRobust = 200;
double Util::costOilWeight = 0.3;
double Util::costUseWeight = 5;
double Util::costSameBaseFleetPrefer = 0;
double Util::costAftTrainPrefer = 0;
double Util::costTargetFleetPrefer = 5000;

bool Util::ifAutoGeneTours = true;
bool Util::isDebug = false;
bool Util::ifContiOrderSameTime = true;

int Util::groupRoundNumLimit = 1;
int Util::groupPertargetNumLimit = 5;
int Util::targetTPAsgNumLimit = 1;
int Util::targetPerGroupNumLimit = 2;
int Util::maxThreadNumLimit = 0;

bool Util::isValidThreadCount(int threads) {
    if (threads <= 0) return false;

    // CPLEX 支持的线程数通常为 1-64
    if (threads > 64) {
        cerr
            << "Warning: Thread count " <<
            threads
            << " exceeds recommended maximum 64" << endl;
    }

    unsigned int sysCores = thread::hardware_concurrency();
    if (sysCores > 0 && threads > sysCores) {
        cerr
            << "Warning: Thread count " <<
            threads
            << " exceeds system cores " << sysCores << endl;
    }

    return true;
}
double Util::maxPriority = 100;
int Util::mainTargetNum = 0;
double Util::targetMaxDistance = 20;
double Util::maxFreqNum = 1;
pair<int, int> Util::generalSlotLimit = make_pair(2, 300);//cap, slotTime每个slot的可容纳飞机数量？每个slot的时间
double Util::IPGap = 0.01;//??

bool Util::ifEvaDestroyDegree = false;
//bool ZZUtil::ifEvaImpTargetCoverRate = false;
//bool ZZUtil::ifEvaBaseTargetPrefer = false;
//bool ZZUtil::ifEvaDmdTargetPrefer = false;
bool Util::ifEvaTargetPriority = true;
bool Util::ifEvaTargetAttackRobustness = false;
bool Util::ifEvaTargetNumPerGrpUb = false;
bool Util::ifEvaGrpNumPerTargetUb = false;
//bool Util::ifEvaBaseAftUseFair = true;
//bool ZZUtil::ifEvaBaseFleetTypeUseFair = false;
//bool ZZUtil::ifEvaBaseLoadUseFair = false;
//bool ZZUtil::ifEvaBaseMountUseFair = false;
//bool ZZUtil::ifEvaTargetNumPerGrpFair = false;
bool Util::ifEvaTotalLoadNum = false;
bool Util::ifEvaGrpLoadFactor = false;
//bool ZZUtil::ifEvaMainTargetSlotFair = false;

TourCost Util::tourCostStruct = { 0,0,0,0,0,0,0,0,0 };

bool Util::cmp(pair<int, double> pair1, pair<int, double> pair2)
{
    return pair1.second < pair2.second;
}

bool Util::cmpBatchByOrder(pair<string, vector<Target*>> batch1, pair<string, vector<Target*>> batch2)
{
    if (batch1.second[0]->getMainTarget()->getOrder() != batch2.second[0]->getMainTarget()->getOrder())
    {
        return batch1.second[0]->getMainTarget()->getOrder() < batch2.second[0]->getMainTarget()->getOrder();
    }
    else
    {
        return batch1.second[batch1.second.size() - 1]->getMainTarget()->getOrder() < batch2.second[batch2.second.size() - 1]->getMainTarget()->getOrder();
    }
}