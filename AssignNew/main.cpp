// main.cpp
#include <iostream>
#include "DataIO.h"
#include "Util.h"
#include "Model.h"
#include "NSGAII.h"
#include "Greedy.h"
using std::cout;


//int main() {
//    try {
//        //使用 nlohmann/json 读取
//        cout << "\n========= 读取运力数据 ===========" << endl;
//        Schedule* schedule = new Schedule();
//        DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
//
//        //string path = "../data/data.json";//算例1
//        //string path = "../data/dataR4T5.json";//算例2
//        //string path = "../data/dataR4T5New.json";//算例4
//        //string path = "../data/dataR550T45.json";//算例3
//        //string path = "../data/newComeTarget1.json";//scenario_16_T4_R32
//        string path = "../data/scenarios/scenario_16_T4_R47.json";//scenario_16_T4_R32 scenario_146_T24_R123
//        //string planPath="../data/costLowest_GreedyOutputdata.json";
//        string outputname(path.begin() + 8, path.end());
//        dataLoader->loadFromFile(path);
//        //dataLoader->loadPlan(planPath);
//        cout << "DataRead running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
//
//        //bool chooseNSGA = true;
//        bool chooseNSGA = false;
//        bool chooseGreedy= false;
//        //bool chooseGreedy = true;
//        //算法开始时间
//        Util::startRunningClock = clock();
//        if (chooseGreedy) {
//            Greedy* greedy = new Greedy(schedule);
//            map<string, vector<Tour*>>result = greedy->getResult();
//            //算法运行时间
//            double runningTime = (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC;
//            cout << "Total running time is " << runningTime << endl;
//            for (auto it = result.begin(); it != result.end(); it++) {
//                string solnOutput = dataLoader->writeJsonOutput(it->second,runningTime);
//                //将结果写入 JSON 文件
//                ofstream outputFile("../data/" + it->first + "_GreedyOutput" + outputname);
//                if (outputFile.is_open()) {
//                    outputFile << solnOutput;
//                    outputFile.close();
//                    std::cout << "结果已写入文件" + it->first + "_GreedyOutput" + outputname<< std::endl;
//                    cout << "Total running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
//                }
//                else {
//                    std::cerr << "无法打开输出文件" << std::endl;
//                }
//            }
//            return 0;
//        }
//    
//        //采用NSGAII算法求解
//        if (chooseNSGA) {
//            NSGAII* nsgaii = new NSGAII(schedule);
//            nsgaii->run();
//            map<string, vector<Tour*>>result = nsgaii->getResult();
//            //算法运行时间
//            double runningTime = (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC;
//            cout << "Total running time is " << runningTime << endl;
//            //将结果写入 JSON 文件
//            for (auto it = result.begin(); it != result.end(); it++) {
//                string solnOutput = dataLoader->writeJsonOutput(it->second,runningTime);
//                ofstream outputFile("../data/output_random_" + it->first+outputname);
//                //ofstream outputFile("../data/output_" + it->first + outputname);
//                if (outputFile.is_open()) {
//                    outputFile << solnOutput;
//                    outputFile.close();
//                    std::cout << "结果已写入文件" + it->first + "_output"+outputname<< std::endl;
//                    cout << "Total running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
//                }
//                else {
//                    std::cerr << "无法打开输出文件" << std::endl;
//                }
//            }
//            return 0;
//        }
//
//        ////求解范围
//        //cout << "\n==============开始确定求解范围================" << endl;
//        //schedule->initResAssign();
//        //if (!Util::ifContiOrderSameTime)
//        //{
//        //    schedule->computeSlotLB();//计算每个具体目标的“最早可开始时间下界”
//        //}
//        //schedule->computeSlotUB();
//        //预生成可用方案
//        cout << " ===============预生成可用方案================" << endl;
//        schedule->generateSchemes();
//        /*不需要目标偏好计划，所以不需要删除tour，注释这个函数*/
//        //schedule->reduceTours();
//        cout << "Scheme running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
//
//        //模型求解
//        cout << " ===============模型求解================" << endl;
//        Model* model = new Model(schedule);
//        model->solve();
//        //算法运行时间
//        double runningTime = (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC;
//        cout << "Total running time is " << runningTime << endl;
//        string solnOutput = dataLoader->writeJsonOutput(model->getSolnTourList(), runningTime);
//        //model->resultAnalysis();
//        delete model;
//        cout << "solve Model running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
//
//        //输出方案
//        cout << "\n==============输出方案================" << endl;
//         //将结果写入 JSON 文件
//        
//        ofstream outputFile("../data/output_"+outputname);
//        if (outputFile.is_open()) {
//            outputFile << solnOutput;
//            outputFile.close();
//            std::cout << "结果已写入文件 output.json" << std::endl;
//            cout << "Total running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
//        }
//        else {
//            std::cerr << "无法打开输出文件" << std::endl;
//        }
//        schedule->clearSoln();
//    }
//    catch (const exception& e) {
//        cerr << "错误: " << e.what() << endl;
//        return 1;
//    }
//
//    return 0;
//}
//



