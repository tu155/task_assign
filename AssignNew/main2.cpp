//// main2.cpp - 两层机器学习推荐系统主程序
//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <map>
//#include <memory>
//#include <filesystem>
//#include <algorithm>
//#include <random>
//#include <chrono>
//#include "DataIO.h"
//#include "Util.h"
//#include "Model.h"
//#include "NSGAII.h"
//#include "Greedy.h"
//#include "HyperparameterOptimizer.h"
//
//#include <nlohmann/json.hpp>
//#include <opencv2/opencv.hpp>
//
//namespace fs = std::filesystem;
//using json = nlohmann::json;
//using std::cout;
//using std::cerr;
//using std::endl;
//
//// 分割数据集为训练集和测试集
//void split_dataset(const std::string& scenarios_dir,
//    const std::string& training_dir,
//    const std::string& test_dir,
//    const std::vector<std::string>& algorithms,
//    float train_ratio = 0.7) {//70%训练集，30%测试集
//
//    std::vector<fs::path> scenario_files;
//    for (const auto& entry : fs::directory_iterator(scenarios_dir)) {
//        if (entry.path().extension() == ".json" &&
//            entry.path().filename().string().find("scenario_") == 0) {
//            scenario_files.push_back(entry.path());
//        }
//    }
//
//    if (scenario_files.empty()) {
//        std::cout << "没有找到场景文件" << std::endl;
//        return;
//    }
//
//    // 修改为固定种子
//    unsigned int fixed_seed = 123456; 
//    std::mt19937 g(fixed_seed);
//    std::shuffle(scenario_files.begin(), scenario_files.end(), g);//随机打乱场景文件顺序
//
//    size_t train_size = static_cast<size_t>(scenario_files.size() * train_ratio);
//    size_t test_size = scenario_files.size() - train_size;
//
//    std::cout << "数据集分割:" << std::endl;
//    std::cout << "  总场景数: " << scenario_files.size() << std::endl;
//    std::cout << "  训练集: " << train_size << " (" << train_ratio * 100 << "%)" << std::endl;
//    std::cout << "  测试集: " << test_size << " (" << (1 - train_ratio) * 100 << "%)" << std::endl;
//
//    fs::create_directories(training_dir);
//    fs::create_directories(test_dir);
//
//    std::cout << "\n复制训练集..." << std::endl;
//    for (size_t i = 0; i < train_size; ++i) {
//        try {
//            fs::copy_file(scenario_files[i],
//                training_dir + scenario_files[i].filename().string(),
//                fs::copy_options::overwrite_existing);
//        }
//        catch (const fs::filesystem_error& e) {
//            std::cerr << "复制失败: " << e.what() << std::endl;
//        }
//    }
//
//    std::cout << "复制测试集..." << std::endl;
//    for (size_t i = train_size; i < scenario_files.size(); ++i) {
//        try {
//            fs::copy_file(scenario_files[i],
//                test_dir + scenario_files[i].filename().string(),
//                fs::copy_options::overwrite_existing);
//        }
//        catch (const fs::filesystem_error& e) {
//            std::cerr << "复制失败: " << e.what() << std::endl;
//        }
//    }
//
//    json split_info = {
//        {"total_scenarios", scenario_files.size()},
//        {"train_ratio", train_ratio},
//        {"train_count", train_size},
//        {"test_count", test_size},
//        {"split_time", std::chrono::system_clock::now().time_since_epoch().count()}
//    };
//
//    std::string split_info_file = scenarios_dir + "dataset_split_info.json";
//    std::ofstream info_file(split_info_file);
//    if (info_file.is_open()) {
//        info_file << split_info.dump(4);
//        info_file.close();
//    }
//    std::cout << "为测试集生成标签..." << std::endl;
//
//    // 为测试集生成标签
//    json test_labels = json::array();
//
//    for (size_t i = train_size; i < scenario_files.size(); ++i) {
//        const auto& scenario_path = scenario_files[i];
//        std::string scenario_name = scenario_path.stem().string();
//
//        std::cout << "处理测试场景: " << scenario_name << std::endl;
//
//        try {
//            // 重置静态计数器
//            Mount::initCountId();
//            Load::initCountId();  // 如果有Load类
//            // 其他需要重置的类...
//
//            Schedule* schedule = new Schedule();
//            DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
//            dataLoader->loadFromFile(scenario_path.string());
//
//            //读取特征
//            map<string, float> features_map = dataLoader->loadFeaturesFromFile(scenario_path.string());
//            // 生成标签（评估所有算法）
//            std::cout << "  评估算法..." << std::endl;
//            std::string best_algorithm = AlgorithmEvaluator::generate_algorithm_label(
//                schedule, algorithms, json::object(), 5);  // 5秒超时
//
//            // 提取特征
//            auto features = ScenarioFeatureExtractor::extract_features(schedule, scenario_name,features_map);
//
//            json label_entry = {
//                {"scenario_name", scenario_name},
//                {"scenario_path", scenario_path.string()},
//                {"true_label", best_algorithm},
//                {"features", features.to_json()},
//                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
//            };
//
//            test_labels.push_back(label_entry);
//
//            std::cout << "  √ 标签: " << best_algorithm << std::endl;
//
//            delete dataLoader;
//            delete schedule;
//
//        }
//        catch (const std::exception& e) {
//            std::cerr << "  × 处理失败: " << e.what() << std::endl;
//            continue;
//        }
//    }
//
//    // 保存测试标签
//    std::string test_labels_file = test_dir + "test_labels.json";
//    std::ofstream labels_file(test_labels_file);
//    if (labels_file.is_open()) {
//        labels_file << test_labels.dump(4);
//        labels_file.close();
//        std::cout << "测试标签已保存到: " << test_labels_file << std::endl;
//    }
//}
//
//bool need_dataset_split(const std::string& training_dir,
//    const std::string& test_dir,
//    const std::string& scenarios_dir) {
//
//    bool training_dir_empty = true;
//    if (fs::exists(training_dir)) {
//        for (const auto& entry : fs::directory_iterator(training_dir)) {
//            if (entry.path().extension() == ".json" &&
//                entry.path().filename().string().find("scenario_") == 0) {
//                training_dir_empty = false;
//                break;
//            }
//        }
//    }
//
//    bool test_dir_empty = true;
//    if (fs::exists(test_dir)) {
//        for (const auto& entry : fs::directory_iterator(test_dir)) {
//            if (entry.path().extension() == ".json" &&
//                entry.path().filename().string().find("scenario_") == 0) {
//                test_dir_empty = false;
//                break;
//            }
//        }
//    }
//
//    bool has_scenarios = false;
//    if (fs::exists(scenarios_dir)) {
//        for (const auto& entry : fs::directory_iterator(scenarios_dir)) {
//            if (entry.path().extension() == ".json" &&
//                entry.path().filename().string().find("scenario_") == 0) {
//                has_scenarios = true;
//                break;
//            }
//        }
//    }
//
//    return has_scenarios && (training_dir_empty || test_dir_empty);//有场景文件，但训练集或测试集文件为空时，需要重新拆分
//}
//
////int main() {
////    try {
////        std::cout << "==========================================" << std::endl;
////        std::cout << "两层机器学习推荐系统 - 算法选择与参数优化" << std::endl;
////        std::cout << "==========================================" << std::endl;
////
////        // 路径配置
////        std::string scenarios_dir = "../data/scenarios/";
////        std::string training_scenarios_dir = "../data/training_scenarios/";
////        std::string test_scenarios_dir = "../data/test_scenarios/";
////        std::string training_data_dir = "../data/training/";
////        std::string models_dir = "../models/";
////
////        // 可用的算法列表
////        std::vector<std::string> algorithms = { "Greedy_Time", "Greedy_Cost","Greedy_Difference", "NSGAII","Exact_Time","Exact_Cost"};//, "NSGAII"
////
////        // 第一步：检查场景数据
////        if (!fs::exists(scenarios_dir)) {
////            std::cout << "场景数据目录不存在" << std::endl;
////            std::cout << "请先运行数据生成器生成场景数据" << std::endl;
////            return 1;
////        }
////
////        // 第二步：检查是否需要分割数据集
////        if (need_dataset_split(training_scenarios_dir, test_scenarios_dir, scenarios_dir)) {
////            std::cout << "\n开始分割数据集 (70%训练, 30%测试)..." << std::endl;
////            split_dataset(scenarios_dir, training_scenarios_dir, test_scenarios_dir,algorithms, 0.7);
////        }
////        else {
////            std::cout << "\n数据集已分割或不需要分割" << std::endl;
////        }
////
////        // 第三步：统计数据集信息
////        int training_scenario_count = 0;
////        int test_scenario_count = 0;
////
////        if (fs::exists(training_scenarios_dir)) {
////            for (const auto& entry : fs::directory_iterator(training_scenarios_dir)) {
////                if (entry.path().extension() == ".json" &&
////                    entry.path().filename().string().find("scenario_") == 0) {
////                    training_scenario_count++;
////                }
////            }
////        }
////
////        if (fs::exists(test_scenarios_dir)) {
////            for (const auto& entry : fs::directory_iterator(test_scenarios_dir)) {
////                if (entry.path().extension() == ".json" &&
////                    entry.path().filename().string().find("scenario_") == 0) {
////                    test_scenario_count++;
////                }
////            }
////        }
////
////        std::cout << "\n数据集统计:" << std::endl;
////        std::cout << "  训练集场景: " << training_scenario_count << std::endl;
////        std::cout << "  测试集场景: " << test_scenario_count << std::endl;
////        std::cout << "  总计: " << (training_scenario_count + test_scenario_count) << std::endl;
////
////        // 第四步：初始化推荐器
////        std::cout << "\n初始化推荐器..." << std::endl;
////        TwoLayerRecommender recommender(models_dir);
////
////        // 第五步：检查是否需要生成训练数据
////        std::string training_data_file = training_data_dir + "algorithm_training_data.json";
////        std::string model_file = models_dir + "algorithm_recommender.yml";
////        std::string training_model_file = training_data_dir + "algorithm_recommender.yml";
////
////        bool need_training = true;
////
////        if (fs::exists(model_file)) {
////            std::cout << "\n找到已部署模型，加载..." << std::endl;
////            if (recommender.load_pretrained_model(model_file)) {
////                std::cout << "模型加载成功" << std::endl;
////                need_training = false;
////            }
////            else {
////                std::cout << "模型加载失败，需要重新训练" << std::endl;
////            }
////        }
////        else if (fs::exists(training_model_file)) {
////            std::cout << "\n找到训练模型，加载并部署..." << std::endl;
////            if (recommender.load_pretrained_model(training_model_file)) {
////                std::cout << "训练模型加载成功，复制到部署目录..." << std::endl;
////                fs::copy_file(training_model_file, model_file, fs::copy_options::overwrite_existing);
////                need_training = false;
////            }
////        }
////
////        // 第六步：训练模型（如果需要）
////        if (need_training && training_scenario_count > 0) {
////            std::cout << "\n开始训练模型..." << std::endl;
////
////            if (!fs::exists(training_data_file)) {
////                std::cout << "生成训练数据..." << std::endl;
////                create_training_data_from_existing_scenarios(
////                    training_scenarios_dir, algorithms,
////                    training_scenario_count,
////                    training_data_dir);
////            }
////
////            if (fs::exists(training_data_file)) {
////                if (recommender.train_algorithm_recommender(training_data_dir)) {
////                    std::cout << "模型训练成功" << std::endl;
////
////                    if (fs::exists(training_model_file) && !fs::exists(model_file)) {
////                        fs::copy_file(training_model_file, model_file, fs::copy_options::overwrite_existing);
////                        std::cout << "模型已部署到: " << model_file << std::endl;
////                    }
////                }
////                else {
////                    std::cerr << "模型训练失败，使用默认规则" << std::endl;
////                }
////            }
////            else {
////                std::cerr << "无法生成训练数据" << std::endl;
////            }
////        }
////        else if (need_training) {
////            std::cout << "\n警告: 没有训练数据，无法训练模型" << std::endl;
////        }
////
////        // 第七步：处理测试场景
////        if (test_scenario_count > 0) {
////            std::cout << "\n处理测试场景..." << std::endl;
////            recommender.process_scenarios_batch(test_scenarios_dir);
////        }
////        else if (training_scenario_count > 0) {
////            std::cout << "\n测试集为空，使用部分训练场景测试..." << std::endl;
////            recommender.process_scenarios_batch(training_scenarios_dir);
////        }
////        else {
////            std::cout << "\n警告: 没有可用的测试场景" << std::endl;
////        }
////        // 第八步：模型评估（新增）
////        if (fs::exists(test_scenarios_dir + "test_labels.json")) {
////            std::cout << "\n开始模型评估..." << std::endl;
////
////            // 创建测试评估器
////            AlgorithmRecommender evaluator;
////            if (evaluator.load_pretrained_model(model_file)) {
////                auto test_result = evaluator.evaluate_model(test_scenarios_dir);
////
////                // 保存评估结果
////                std::string eval_result_file = models_dir + "model_evaluation.json";
////                std::ofstream eval_file(eval_result_file);
////                if (eval_file.is_open()) {
////                    json eval_json = {
////                        {"test_accuracy", test_result.accuracy},
////                        {"confusion_matrix", test_result.confusion_matrix},
////                        {"algorithm_accuracy", {
////                            {"exact_Time", test_result.total_by_algorithm["exact_Time"] > 0 ?
////                                static_cast<double>(test_result.correct_by_algorithm["exact_Time"]) /
////                                test_result.total_by_algorithm["exact_Time"] : 0.0},
////                            {"exact_Cost", test_result.total_by_algorithm["exact_Cost"] > 0 ?
////                                static_cast<double>(test_result.correct_by_algorithm["exact_Cost"]) /
////                                test_result.total_by_algorithm["exact_Cost"] : 0.0},
////                            {"nsgaii", test_result.total_by_algorithm["nsgaii"] > 0 ?
////                                static_cast<double>(test_result.correct_by_algorithm["nsgaii"]) /
////                                test_result.total_by_algorithm["nsgaii"] : 0.0},
////                            {"greedy_time", test_result.total_by_algorithm["greedy_time"] > 0 ?
////                                static_cast<double>(test_result.correct_by_algorithm["greedy_time"]) /
////                                test_result.total_by_algorithm["greedy_time"] : 0.0},
////                            {"greedy_cost", test_result.total_by_algorithm["greedy_cost"] > 0 ?
////                                static_cast<double>(test_result.correct_by_algorithm["greedy_cost"]) /
////                                test_result.total_by_algorithm["greedy_cost"] : 0.0}
////                        }},
////                        {"evaluation_time", std::chrono::system_clock::now().time_since_epoch().count()}
////                    };
////
////                    eval_file << eval_json.dump(4);
////                    eval_file.close();
////                    std::cout << "评估结果已保存到: " << eval_result_file << std::endl;
////                }
////            }
////            else {
////                std::cerr << "无法加载模型进行评估" << std::endl;
////            }
////        }
////        else {
////            std::cout << "\n警告: 没有测试标签文件，无法计算准确率" << std::endl;
////            std::cout << "请先生成测试标签: 运行 generate_test_labels()" << std::endl;
////        }
////
////        // 第九步：保存状态和报告
////        std::string state_file = models_dir + "recommender_state.json";
////        if (recommender.save_state(state_file)) {
////            std::cout << "\n状态已保存到: " << state_file << std::endl;
////        }
////
////        auto stats = recommender.get_statistics();
////        std::cout << "\n系统统计信息:\n" << stats.dump(4) << std::endl;
////
////        // 生成性能报告
////        json report = {
////            {"dataset_info", {
////                {"training_scenarios", training_scenario_count},
////                {"test_scenarios", test_scenario_count},
////                {"total_scenarios", training_scenario_count + test_scenario_count}
////            }},
////            {"system_stats", stats},
////            {"execution_time", std::chrono::system_clock::now().time_since_epoch().count()}
////        };
////
////        std::string report_file = models_dir + "performance_report.json";
////        std::ofstream report_stream(report_file);
////        if (report_stream.is_open()) {
////            report_stream << report.dump(4);
////            report_stream.close();
////            std::cout << "性能报告已保存到: " << report_file << std::endl;
////        }
////
////        std::cout << "\n==========================================" << std::endl;
////        std::cout << "系统运行完成" << std::endl;
////        std::cout << "==========================================" << std::endl;
////
////    }
////    catch (const std::exception& e) {
////        std::cerr << "系统错误: " << e.what() << std::endl;
////        return 1;
////    }
////
////    return 0;
////}
//
//
