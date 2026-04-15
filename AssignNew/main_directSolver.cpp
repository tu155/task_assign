// main.cpp
#include <iostream>
#include <chrono>
#include "DataIO.h"
#include "Util.h"
#include "Model.h"
#include "NSGAII.h"
#include "Greedy.h"
#include "DirectSolver.h"  // 新增：直接求解器头文件

using std::cout;
using std::cerr;
using std::endl;

// 求解器类型枚举
enum SolverType {
    SOLVER_MODEL,        // 原Model求解器（列生成+IP）
    SOLVER_NSGAII,       // NSGA-II多目标进化算法
    SOLVER_GREEDY,       // 多策略贪心算法
    SOLVER_DIRECT        // 直接求解器（基准对比）
};

int main() {
    try {
        // ==================== 参数配置（可直接修改） ====================

        // 选择求解器类型 - 在这里切换不同的算法
        SolverType solverType = SOLVER_NSGAII;  // 当前使用直接求解器

        // 数据文件路径
        string path = "../data/scenarios/scenario_16_T4_R47.json";
        //scenario_16_T4_R47 scenario_2_T75_R800

        // 直接求解器参数（可直接修改）
        double directSolverTimeLimit = 300.0;   // 求解时间上限（秒）
        double directSolverGap = 0.05;           // MIP间隙容忍度（5%）
        double directUncoverPenalty = Util::costTargetUncover;    // 未覆盖目标惩罚系数

        // 是否输出详细信息
        bool verbose = true;

        // ==================== 读取数据 ====================

        cout << "\n========= 读取运力数据 ===========" << endl;
        cout << "数据文件: " << path << endl;

        Schedule* schedule = new Schedule();
        DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);

        // 记录数据读取开始时间
        auto readStart = std::chrono::steady_clock::now();

        string outputname(path.begin() + 18, path.end());
        dataLoader->loadFromFile(path);

        auto readEnd = std::chrono::steady_clock::now();
        double readTime = std::chrono::duration<double>(readEnd - readStart).count();
        cout << "数据读取时间: " << readTime << "秒" << endl;

        // 算法开始时间
        Util::startRunningClock = clock();
        auto algoStart = std::chrono::steady_clock::now();

        // ==================== 根据求解器类型选择算法 ====================

        cout << "\n========= 开始求解 ===========" << endl;
        cout << "求解器类型: ";
        switch (solverType) {
        case SOLVER_MODEL: cout << "原Model求解器（列生成+IP）" << endl; break;
        case SOLVER_NSGAII: cout << "NSGA-II多目标进化算法" << endl; break;
        case SOLVER_GREEDY: cout << "多策略贪心算法" << endl; break;
        case SOLVER_DIRECT: cout << "直接求解器（基准对比）" << endl; break;
        }

        vector<Tour*> resultTours;
        map<string, vector<Tour*>> multiResult;  // 用于NSGA-II和贪心的多解返回
        double runningTime = 0.0;

        switch (solverType) {
        case SOLVER_MODEL: {
            // ===== 原Model求解器 =====
            cout << "\n--- 预生成可用方案 ---" << endl;
            schedule->generateSchemes();
            cout << "方案生成时间: " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << "秒" << endl;

            cout << "\n--- 模型求解 ---" << endl;
            Model* model = new Model(schedule);
            model->solve();
            resultTours = model->getSolnTourList();
            delete model;
            break;
        }

        case SOLVER_NSGAII: {
            // ===== NSGA-II求解器 =====
            cout << "\n--- NSGA-II求解 ---" << endl;
            NSGAII* nsgaii = new NSGAII(schedule);
            nsgaii->run();
            multiResult = nsgaii->getResult();

            // 默认取第一个解（可根据需要选择）
            if (!multiResult.empty()) {
                resultTours = multiResult.begin()->second;
            }
            delete nsgaii;
            break;
        }

        case SOLVER_GREEDY: {
            // ===== 贪心求解器 =====
            cout << "\n--- 贪心算法求解 ---" << endl;
            Greedy* greedy = new Greedy(schedule);
            multiResult = greedy->getResult();

            // 默认取第一个解（可根据需要选择）
            if (!multiResult.empty()) {
                resultTours = multiResult.begin()->second;
            }
            delete greedy;
            break;
        }

        case SOLVER_DIRECT: {
            // ===== 直接求解器（基准对比） =====
            cout << "\n--- 直接求解器（基准对比）---" << endl;
            cout << "时间上限: " << directSolverTimeLimit << "秒" << endl;
            cout << "间隙容忍度: " << directSolverGap * 100 << "%" << endl;
            cout << "未覆盖惩罚系数: " << directUncoverPenalty << endl;

            // 创建直接求解器
            DirectSolver* directSolver = new DirectSolver(schedule, directSolverTimeLimit, verbose);

            // 设置参数
            directSolver->setGapTolerance(directSolverGap);
            directSolver->setUncoverPenalty(directUncoverPenalty);
            directSolver->setThreads(0);          // 0=自动线程
            directSolver->setWorkMem(2048);       // 内存限制2048MB

            // 运行求解
            //resultTours = directSolver->run();
            resultTours = directSolver->runMultiRound();

            // 输出求解统计
            cout << "\n--- 直接求解器统计 ---" << endl;
            cout << "求解状态: " << directSolver->getStatus() << endl;
            cout << "目标值: " << directSolver->getObjValue() << endl;
            double solveTime = directSolver->getSolveTime();
            cout <<  "CPLEX求解时间: " << std::fixed << std::setprecision(6) << solveTime << "秒" << endl;
            cout << "变量数: " << directSolver->getVariableCount() << endl;
            cout << "约束数: " << directSolver->getConstraintCount() << endl;

            // 可选：导出模型用于调试
            // directSolver->exportModel("../data/direct_model.lp");

            delete directSolver;
            break;
        }
        }

        // 计算算法运行时间
        auto algoEnd = std::chrono::steady_clock::now();
        runningTime = std::chrono::duration<double>(algoEnd - algoStart).count();
        cout << "\n算法运行时间: " << runningTime << "秒" << endl;

        // ==================== 输出结果 ====================

        cout << "\n========= 输出方案 ===========" << endl;

        // 生成输出文件名
        string solverName;
        switch (solverType) {
        case SOLVER_MODEL: solverName = "Model"; break;
        case SOLVER_NSGAII: solverName = "NSGAII"; break;
        case SOLVER_GREEDY: solverName = "Greedy"; break;
        case SOLVER_DIRECT: solverName = "Direct"; break;
        }

        // 处理多目标返回的情况（NSGA-II和贪心可能返回多个解）
        if (solverType == SOLVER_NSGAII || solverType == SOLVER_GREEDY) {
            for (auto it = multiResult.begin(); it != multiResult.end(); it++) {
                string solnOutput = dataLoader->writeJsonOutput(it->second, runningTime);
                string outputFile = "../data/" + solverName + "_" + it->first + "_" + outputname;

                std::ofstream outputFileStream(outputFile);
                if (outputFileStream.is_open()) {
                    outputFileStream << solnOutput;
                    outputFileStream.close();
                    cout << "结果已写入文件: " + outputFile << endl;
                }
                else {
                    cerr << "无法打开输出文件: " << outputFile << endl;
                }
            }
        }
        else {
            // 单目标返回的情况
            string solnOutput = dataLoader->writeJsonOutput(resultTours, runningTime);
            string outputFile = "../data/" + solverName+'/'+ solverName + "_" + outputname;

            std::ofstream outputFileStream(outputFile);
            if (outputFileStream.is_open()) {
                outputFileStream << solnOutput;
                outputFileStream.close();
                cout << "结果已写入文件: " << outputFile << endl;
            }
            else {
                cerr << "无法打开输出文件: " << outputFile << endl;
            }
        }

        // ==================== 清理 ====================

        schedule->clearSoln();
        delete dataLoader;
        delete schedule;

        cout << "\n========= 运行完成 ===========" << endl;
        cout << "总运行时间: " << runningTime + readTime << "秒" << endl;

    }
    catch (const exception& e) {
        cerr << "\n错误: " << e.what() << endl;
        return 1;
    }
    catch (const IloException& e) {
        cerr << "\nCPLEX错误: " << e.getMessage() << endl;
        return 1;
    }

    return 0;
}

