#ifndef ZZUTIL_H
#define ZZUTIL_H

// 首先包含 CPLEX 配置
#include "CPLEXConfig.h"

// 5. 然后包含标准库
#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <iostream>
#include <ctime>
#include <random>
#include <array>
#include <map>
#include <set>
#include <unordered_map>

// 最后包含 Windows 特定头文件
#ifdef _WIN32
#include <process.h>
#include <codecvt>
#endif
#pragma warning(disable:4244 4267 4334)

using namespace std;


#define THREADSIZE 32
#define ADAYTIME 24 * 60 * 60
#define HOUR 3600
#define EPSILON 0.0001
#define PI 3.141592654
#define EARTH_RADIUS 6378.137// 地球半径，单位：千米(km)

namespace nsgaii {
    // 算法参数默认值
    constexpr int DEFAULT_POPULATION_SIZE = 100;                //种群大小
    constexpr int DEFAULT_MAX_GENERATIONS = 500;                //最大迭代次数
    constexpr double DEFAULT_CROSSOVER_RATE = 0.9;              //交叉概率
    constexpr double DEFAULT_MUTATION_RATE = 0.1;               //变异概率
    constexpr double DISTRIBUTION_INDEX = 20.0;                 //分布指数

    // 变量边界
    constexpr double DEFAULT_LOWER_BOUND = 0.0;
    constexpr double DEFAULT_UPPER_BOUND = 1.0;

    // 数值稳定性
    constexpr double GA_EPSILON = 1e-10;
}

enum ScenarioType
{
    RevenueBased,
    HistoryBased,
    BreakdownBased,
    EquityBased
};

enum TourType
{
    BOTHFIX_TOUR,    //频点时刻/WQ/行动均给定（频点时刻与给定的一致）
    BOTHFIX_HALF_TOUR,   //频点时刻/WQ/行动均给定（频点时刻不保证和给定的一致）
    BOTHFIX_CP_TOUR, //频点时刻/WQ/行动均给定（频点时刻不保证和给定的一致）
    GROUPFIX_TOUR,   //除了频点和时刻以外均给定
    NOTFIX_TOUR      //普通
};

enum TargetStatus
{
    NORMAL,          //普通目标
    BOTHFIX,         //频点时刻/WQ/行动均给定（频点时刻可违反,目前先不做）
    GROUPFIX,        //WQ和行动被给定的目标
    PARTIAL_FIX      //WQ方案中只有一种挂载被给定（目前先不做）
};

struct TourCost
{
    double costUseLoad;
    double costUseMount;
    double costMountLoadMatch;
    double costDst;
    double costOil;
    double costUseAft;
    double costWait;
    double costAftFailure;
    double costFleetPrefer;
};

class Target;
class Util
{
public:
    static string INPUTPATH;
    static string OUTPUTPATH;
    static time_t startRunningClock;

    // ============= NSGA-II 算法参数 =============
    static int nsgaii_population_size;              // 种群大小
    static int nsgaii_max_generations;              // 最大迭代次数
    static double nsgaii_crossover_rate;            // 交叉概率
    static double nsgaii_mutation_rate;             // 变异概率
    static double nsgaii_init_timeShortest_ratio;   // 时间最短策略初始化比例
    static double nsgaii_init_costLowest_ratio;     // 成本最低策略初始化比例
    static double nsgaii_init_normal_ratio;         // 随机策略初始化比例
    static int nsgaii_objective_count;              // 优化目标数量 (2或3)

    static string getTimeStr(time_t t);
    static string getUUID();
    static std::mt19937& get_generator();
    static double targetMaxDistance;
    static bool ifAutoGeneTours;
    static int groupRoundNumLimit;//??什么含义，在这个模型中没有考虑
    static int groupPertargetNumLimit;
    static int targetTPAsgNumLimit;
    static int targetPerGroupNumLimit;
    static int maxThreadNumLimit;
    static bool isValidThreadCount(int threads);
    static double maxPriority;
    static int mainTargetNum;
    static double IPGap;

    static double weightTourTime;
    void setweightTourTime(double weightTourTime_) { weightTourTime = weightTourTime_; };
    static double costTargetUncover;
    static double costTargetUncoverByOrder;
    static double costBaseLessThanRatio;
    static double costDistWeight;
    static double costAftOil;
    static double costGrpUnLoad;
    static double costBaseTarPref;
    static double costOverGrpPerTarNum;
    static double revLessGrpTarNum;
    static double costDmdTarPref;//偏好目标的需求成本？
    static double costDmdTarRate;
    static double costFleetTypeBaseFair;
    static double costFleetBaseFair;
    static double costLoadBaseFair;
    static double costMountBaseFair;
    static double costAftFailure;
    static double costTarRobust;
    static double costOilWeight;//到所有目标的平均距离/机型速度 * 该机型燃油费（单位：/每分钟）* 飞机数量 * 燃油费系数
    static double costUseWeight;
    static double costSameBaseFleetPrefer;
    static double costAftTrainPrefer;
    static double costTargetFleetPrefer;

    static pair<int, int> generalSlotLimit;
    static double maxFreqNum;
    static bool cmp(pair<int, double> pair1, pair<int, double> pair2);
    static bool cmpBatchByOrder(pair<string, vector<Target*>> batch1, pair<string, vector<Target*>> batch2);//比较主目标顺序
    static bool isDebug;
    static bool ifContiOrderSameTime;           //?是否连续订单要求同一时间？满足则计算目标的"最早可开始时间"

    static bool ifEvaDestroyDegree;
    //static bool ifEvaImpTargetCoverRate;
    //static bool ifEvaBaseTargetPrefer;
    //static bool ifEvaDmdTargetPrefer;
    static bool ifEvaTargetPriority;
    static bool ifEvaTargetAttackRobustness;
    static bool ifEvaTargetNumPerGrpUb;
    static bool ifEvaGrpNumPerTargetUb;
    //static bool ifEvaBaseAftUseFair;
    //static bool ifEvaBaseFleetTypeUseFair;
    //static bool ifEvaBaseLoadUseFair;
    //static bool ifEvaBaseMountUseFair;
    //static bool ifEvaTargetNumPerGrpFair;
    static bool ifEvaTotalLoadNum;
    static bool ifEvaGrpLoadFactor;
    //static bool ifEvaMainTargetSlotFair;

    static TourCost tourCostStruct;
};

//inline time_t makeTime(int yr, int mon, int day, int hr = 0, int min = 0, int sec = 0);
inline time_t makeTime(int yr, int mon, int day, int hr, int min, int sec);
inline double getDistance(double lng0, double lat0, double lng1, double lat1)
{
    //计算地球上任意两点之间的球面距离
    double radLat0 = lat0 * PI / 180.0;
    double radLat1 = lat1 * PI / 180.0;
    double a = radLat0 - radLat1;
    double b = lng0 * PI / 180.0 - lng1 * PI / 180.0;
    double dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat0) * cos(radLat1) * pow(sin(b / 2), 2))));
    dst = dst * EARTH_RADIUS;
    dst = round(dst * 10000) / 10000;
    return dst;
}

#endif