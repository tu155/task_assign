// main_multi_directsolver.cpp
#define _CRT_SECURE_NO_WARNINGS  // 添加在文件最开头
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include "DataIO.h"
#include "Util.h"
#include "DirectSolver.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
namespace fs = std::filesystem;

// 结果记录结构
struct SolveResult {
    string filename;
    int numTargets;
    int numFleets;
    int numMountLoads;
    double objValue;
    double solveTime;
    double totalTime;
    int variableCount;
    int constraintCount;
    int uncoveredTargets;
    double targetCoverage;
    IloAlgorithm::Status status;
    string statusStr;
    bool success;

    // 默认构造函数
    SolveResult() : numTargets(0), numFleets(0), numMountLoads(0),
        objValue(0.0), solveTime(0.0), totalTime(0.0),
        variableCount(0), constraintCount(0), uncoveredTargets(0),
        targetCoverage(0.0), status(IloAlgorithm::Unknown),
        statusStr("Unknown"), success(false) {}
};

// 获取状态字符串
string getStatusString(IloAlgorithm::Status status) {
    switch (status) {
    case IloAlgorithm::Optimal: return "Optimal";
    case IloAlgorithm::Feasible: return "Feasible";
    case IloAlgorithm::Infeasible: return "Infeasible";
    case IloAlgorithm::Unbounded: return "Unbounded";
    case IloAlgorithm::InfeasibleOrUnbounded: return "InfeasibleOrUnbounded";
    case IloAlgorithm::Error: return "Error";
    default: return "Unknown";
    }
}

// 将单个结果追加到CSV文件
void appendResultToCSV(const SolveResult& result, const string& outputFile, bool isFirst) {
    std::ofstream csv;

    if (isFirst) {
        // 如果是第一个结果，创建新文件并写入表头
        csv.open(outputFile);
        if (!csv.is_open()) {
            cerr << "无法创建CSV文件: " << outputFile << endl;
            return;
        }
        csv << "文件名,目标数,资源数,挂载-载荷对数,目标值,求解时间(秒),总时间(秒),"
            << "变量数,约束数,未覆盖目标数,目标覆盖率(%),求解状态,是否成功\n";
    }
    else {
        // 否则追加到文件末尾
        csv.open(outputFile, std::ios::app);
        if (!csv.is_open()) {
            cerr << "无法打开CSV文件追加: " << outputFile << endl;
            return;
        }
    }

    // 写入当前结果
    csv << result.filename << ","
        << result.numTargets << ","
        << result.numFleets << ","
        << result.numMountLoads << ","
        << std::fixed << std::setprecision(2) << result.objValue << ","
        << std::fixed << std::setprecision(2) << result.solveTime << ","
        << std::fixed << std::setprecision(2) << result.totalTime << ","
        << result.variableCount << ","
        << result.constraintCount << ","
        << result.uncoveredTargets << ","
        << std::fixed << std::setprecision(1) << result.targetCoverage << ","
        << result.statusStr << ","
        << (result.success ? "是" : "否") << "\n";

    csv.close();
    cout << "  结果已追加到: " << outputFile << endl;
}

// 更新摘要文件（重写整个文件）
void updateSummaryFile(const vector<SolveResult>& allResults, const string& summaryFile) {
    std::ofstream summary(summaryFile);
    if (!summary.is_open()) {
        cerr << "无法创建摘要文件: " << summaryFile << endl;
        return;
    }

    summary << "直接求解器批量测试结果\n";
    summary << "======================\n\n";

    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    summary << "最后更新时间: " << std::ctime(&now_time);

    summary << "\n参数设置:\n";
    // 注意：这里无法获取原始参数，需要在主函数中传递

    summary << "\n已处理文件数: " << allResults.size() << "\n";

    int successCount = 0;
    double totalSolveTime = 0;
    double totalObjValue = 0;

    for (const auto& r : allResults) {
        if (r.success) {
            successCount++;
            totalSolveTime += r.solveTime;
            totalObjValue += r.objValue;
        }
    }

    summary << "成功求解数: " << successCount << "\n";
    summary << "失败数: " << allResults.size() - successCount << "\n";

    if (successCount > 0) {
        summary << "平均求解时间: " << std::fixed << std::setprecision(2)
            << totalSolveTime / successCount << "秒\n";
        summary << "平均目标值: " << std::fixed << std::setprecision(2)
            << totalObjValue / successCount << "\n";
    }

    summary << "\n详细结果:\n";
    summary << "----------------------------------------------------------------\n";

    for (const auto& r : allResults) {
        summary << r.filename << "\t"
            << r.numTargets << "\t"
            << r.numFleets << "\t"
            << std::fixed << std::setprecision(2) << r.solveTime << "\t"
            << std::fixed << std::setprecision(2) << r.objValue << "\t"
            << std::fixed << std::setprecision(1) << r.targetCoverage << "%\t"
            << r.statusStr << "\t"
            << (r.success ? "成功" : "失败") << "\n";
    }

    summary.close();
    cout << "摘要文件已更新: " << summaryFile << endl;
}

//int main() {
//    try {
//        // ==================== 参数配置 ====================
//
//        // 数据文件夹路径
//        string dataFolder = "../data/scenarios/";
//
//        // 输出文件
//        string resultCsvFile = "../data/direct_solver_results.csv";
//        string summaryFile = "../data/direct_solver_summary.txt";
//
//        // 直接求解器参数
//        double timeLimit = 300.0;      // 每个实例求解时间上限（秒）
//        double gapTolerance = 0.05;     // MIP间隙容忍度（5%）
//        double uncoverPenalty = 10000.0; // 未覆盖目标惩罚系数
//        bool verbose = false;            // 是否输出详细信息（批量运行时建议false）
//
//        // 文件扩展名过滤
//        string extension = ".json";
//
//        // ==================== 获取所有数据文件 ====================
//
//        cout << "========== 直接求解器批量测试 ==========" << endl;
//        cout << "数据文件夹: " << dataFolder << endl;
//        cout << "时间上限: " << timeLimit << "秒/实例" << endl;
//        cout << "间隙容忍度: " << gapTolerance * 100 << "%" << endl;
//        cout << "未覆盖惩罚系数: " << uncoverPenalty << endl;
//        cout << endl;
//
//        // 检查文件夹是否存在
//        if (!fs::exists(dataFolder)) {
//            cerr << "错误: 文件夹不存在 - " << dataFolder << endl;
//            return 1;
//        }
//
//        // 遍历文件夹，收集所有JSON文件
//        vector<string> dataFiles;
//        for (const auto& entry : fs::directory_iterator(dataFolder)) {
//            if (entry.is_regular_file()) {
//                string path = entry.path().string();
//                if (path.size() >= extension.size() &&
//                    path.substr(path.size() - extension.size()) == extension) {
//                    dataFiles.push_back(path);
//                }
//            }
//        }
//
//        // 按文件名排序，保证结果顺序一致
//        std::sort(dataFiles.begin(), dataFiles.end());
//
//        cout << "找到 " << dataFiles.size() << " 个数据文件" << endl;
//        cout << string(80, '=') << endl;
//
//        // ==================== 批量求解（实时保存）====================
//
//        vector<SolveResult> allResults;  // 保存所有已处理的结果
//        int fileCount = 0;
//        int totalFiles = dataFiles.size();
//        bool isFirstResult = true;        // 标记是否是第一个结果（用于CSV表头）
//
//        for (const string& filePath : dataFiles) {
//            fileCount++;
//
//            // 获取文件名（不含路径）
//            string filename = fs::path(filePath).filename().string();
//
//            cout << "\n[" << fileCount << "/" << totalFiles << "] 处理: " << filename << endl;
//
//            // 记录开始时间
//            auto totalStart = std::chrono::steady_clock::now();
//
//            // 创建Schedule和数据加载器
//            Schedule* schedule = nullptr;
//            DataLoader_Nlohmann* dataLoader = nullptr;
//            DirectSolver* solver = nullptr;
//
//            SolveResult result;
//            result.filename = filename;
//            result.success = false;
//
//            try {
//                // 创建对象
//                schedule = new Schedule();
//                dataLoader = new DataLoader_Nlohmann(schedule);
//
//                // 读取数据
//                dataLoader->loadFromFile(filePath);
//
//                // 获取问题规模信息
//                result.numTargets = schedule->getTargetList().size();
//                result.numFleets = schedule->getFleetList().size();
//                result.numMountLoads = schedule->getMountLoadList().size();
//
//                if (verbose) {
//                    cout << "  目标数: " << result.numTargets
//                        << ", 资源数: " << result.numFleets
//                        << ", 挂载-载荷对数: " << result.numMountLoads << endl;
//                }
//
//                // 创建直接求解器
//                solver = new DirectSolver(schedule, timeLimit, verbose);
//
//                // 设置参数
//                solver->setGapTolerance(gapTolerance);
//                solver->setUncoverPenalty(uncoverPenalty);
//                solver->setThreads(0);      // 自动线程
//                solver->setWorkMem(4096);    // 内存限制4GB
//
//                // 运行求解
//                vector<Tour*> tours = solver->run();
//
//                // 记录求解结果
//                result.solveTime = solver->getSolveTime();
//                result.objValue = solver->getObjValue();
//                result.variableCount = solver->getVariableCount();
//                result.constraintCount = solver->getConstraintCount();
//                result.status = solver->getStatus();
//                result.statusStr = getStatusString(solver->getStatus());
//
//                // 计算未覆盖目标数
//                vector<Target*> uncoverTargets = schedule->getUncoverTList();
//                result.uncoveredTargets = uncoverTargets.size();
//                result.targetCoverage = (result.numTargets - result.uncoveredTargets) * 100.0 / result.numTargets;
//
//                // 判断是否成功
//                result.success = (solver->getStatus() == IloAlgorithm::Optimal ||
//                    solver->getStatus() == IloAlgorithm::Feasible);
//
//                // 输出求解方案（可选）
//                if (verbose && result.success && !tours.empty()) {
//                    string solnOutput = dataLoader->writeJsonOutput(tours, result.solveTime);
//                    string outputFile = "../data/output_direct_" + filename;
//                    std::ofstream outFile(outputFile);
//                    if (outFile.is_open()) {
//                        outFile << solnOutput;
//                        outFile.close();
//                        cout << "  方案已写入: " << outputFile << endl;
//                    }
//                }
//
//            }
//            catch (const std::exception& e) {
//                cerr << "  错误: " << e.what() << endl;
//                result.statusStr = "Exception: " + string(e.what());
//                result.success = false;
//            }
//            catch (const IloException& e) {
//                cerr << "  CPLEX错误: " << e.getMessage() << endl;
//                result.statusStr = "CPLEX Exception: " + string(e.getMessage());
//                result.success = false;
//            }
//            catch (...) {
//                cerr << "  未知错误" << endl;
//                result.statusStr = "Unknown Error";
//                result.success = false;
//            }
//
//            // 计算总时间
//            auto totalEnd = std::chrono::steady_clock::now();
//            result.totalTime = std::chrono::duration<double>(totalEnd - totalStart).count();
//
//            // 无论成功失败，都将结果添加到总列表
//            allResults.push_back(result);
//
//            // 输出当前结果摘要
//            cout << "  结果: " << (result.success ? "成功" : "失败")
//                << ", 求解时间: " << std::fixed << std::setprecision(2) << result.solveTime << "秒"
//                << ", 目标值: " << std::fixed << std::setprecision(2) << result.objValue
//                << ", 覆盖率: " << std::fixed << std::setprecision(1) << result.targetCoverage << "%" << endl;
//
//            // ========== 实时保存结果到文件 ==========
//
//            // 追加到CSV文件
//            appendResultToCSV(result, resultCsvFile, isFirstResult);
//            isFirstResult = false;  // 之后的结果都是追加模式
//
//            // 更新摘要文件（重写整个文件，包含所有已处理的结果）
//            updateSummaryFile(allResults, summaryFile);
//
//            // 清理资源
//            if (schedule) {
//                schedule->clearSoln();
//                delete schedule;
//            }
//            if (dataLoader) delete dataLoader;
//            if (solver) delete solver;
//        }
//
//        // ==================== 最终输出 ====================
//
//        cout << "\n" << string(80, '=') << endl;
//        cout << "所有文件处理完成！" << endl;
//        cout << "结果已保存到: " << resultCsvFile << endl;
//        cout << "摘要已保存到: " << summaryFile << endl;
//        cout << "\n========== 批量测试完成 ==========" << endl;
//
//    }
//    catch (const std::exception& e) {
//        cerr << "\n致命错误: " << e.what() << endl;
//        return 1;
//    }
//
//    return 0;
//}



