//// main2.cpp - 两层机器学习推荐系统主程序（支持二次规划）
//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <map>
//#include <memory>
//#include <filesystem>
//#include <algorithm>
//#include <random>
//#include <chrono>
//#include <regex>
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
//    float train_ratio = 0.7,
//    bool is_secondary = false) { // 新增参数，标识是否是二次规划
//
//    std::vector<fs::path> scenario_files;
//    for (const auto& entry : fs::directory_iterator(scenarios_dir)) {
//        if (entry.path().extension() == ".json" &&
//            (entry.path().filename().string().find("scenario_") == 0 || entry.path().filename().string().find("secondary_") == 0)) {
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
//        {"is_secondary", is_secondary}, // 记录是否是二次规划
//        {"split_time", std::chrono::system_clock::now().time_since_epoch().count()}
//    };
//
//    std::string split_info_file = scenarios_dir + "dataset_split_info.json";
//    std::ofstream info_file(split_info_file);
//    if (info_file.is_open()) {
//        info_file << split_info.dump(4);
//        info_file.close();
//    }
//
//    std::cout << "为测试集生成标签..." << std::endl;
//
//    // 为测试集生成标签
//    json test_labels = json::array();
//
//    // 上次规划结果目录
//    std::string prev_plan_dir = "../modelsForData/results/";
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
//            Load::initCountId();
//            Target::initCountId();
//            Base::initCountId();
//            TargetType::initCountId();
//            Tour::initCountId();
//            MainTarget::initCountId();
//
//            Schedule* schedule = new Schedule();
//            DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
//            dataLoader->loadFromFile(scenario_path.string());
//
//            // 如果是二次规划，加载上次规划结果
//            if (is_secondary) {
//                std::string prev_plan_path = find_previous_plan_file(scenario_name, prev_plan_dir);
//                if (!prev_plan_path.empty()) {
//                    load_previous_plan(schedule, prev_plan_path);
//                }
//            }
//
//            //读取特征
//            map<string, float> features_map = dataLoader->loadFeaturesFromFile(scenario_path.string());
//
//            // 根据是否是二次规划设置决策偏好
//            float decision_preferance = features_map["decision_preferance"];
//
//            // 生成标签（评估所有算法）
//            std::cout << "  评估算法..." << std::endl;
//            std::string best_algorithm = AlgorithmEvaluator::generate_algorithm_label(
//                schedule, algorithms, json::object(), 5, decision_preferance);  // 5秒超时
//
//            // 提取特征（传递is_secondary参数）
//            auto features = ScenarioFeatureExtractor::extract_features(schedule, scenario_name, features_map);
//
//            // 手动设置二次规划标志
//            if (is_secondary) {
//                features.is_secondary_planning = true;
//                // 尝试从场景文件中提取二次规划特征
//                try {
//                    std::ifstream file(scenario_path.string());
//                    json scenario_json;
//                    file >> scenario_json;
//                    file.close();
//
//                    if (scenario_json.contains("scenario_features")) {
//                        const auto& scenario_feat = scenario_json["scenario_features"];
//                        if (scenario_feat.contains("target_change_ratio")) {
//                            features.target_change_ratio = scenario_feat["target_change_ratio"];
//                        }
//                        if (scenario_feat.contains("original_target_count")) {
//                            features.original_target_count = scenario_feat["original_target_count"];
//                        }
//                    }
//                }
//                catch (...) {
//                    // 忽略解析错误
//                }
//            }
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
//    if (is_secondary) {
//        test_labels_file = test_dir + "secondary_test_labels.json";
//    }
//
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
//                (entry.path().filename().string().find("scenario_") == 0 || entry.path().filename().string().find("secondary_") == 0)) {
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
//                (entry.path().filename().string().find("scenario_") == 0 || entry.path().filename().string().find("secondary_") == 0)) {
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
//                (entry.path().filename().string().find("scenario_") == 0 || entry.path().filename().string().find("secondary_") == 0)) {
//                has_scenarios = true;
//                break;
//            }
//        }
//    }
//
//    return has_scenarios && (training_dir_empty || test_dir_empty);//有场景文件，但训练集或测试集文件为空时，需要重新拆分
//}
//
//// 合并一次和二次规划训练数据
//json merge_training_data(const std::string& primary_data_file,
//    const std::string& secondary_data_file) {
//    json merged_data = json::array();
//
//    // 加载一次规划数据
//    if (fs::exists(primary_data_file)) {
//        std::ifstream primary_file(primary_data_file);
//        if (primary_file.is_open()) {
//            json primary_data;
//            primary_file >> primary_data;
//            primary_file.close();
//
//            // 为一次规划数据添加标记
//            for (auto& sample : primary_data) {
//                if (sample.contains("features")) {
//                    sample["features"]["is_secondary_planning"] = false;
//                    // 为一次规划设置默认的二次规划特征
//                    if (!sample["features"].contains("target_change_ratio")) {
//                        sample["features"]["target_change_ratio"] = 0.0;
//                    }
//                    if (!sample["features"].contains("original_target_count")) {
//                        sample["features"]["original_target_count"] = sample["features"]["num_targets"];
//                    }
//                }
//            }
//
//            merged_data.insert(merged_data.end(), primary_data.begin(), primary_data.end());
//        }
//    }
//
//    // 加载二次规划数据
//    if (fs::exists(secondary_data_file)) {
//        std::ifstream secondary_file(secondary_data_file);
//        if (secondary_file.is_open()) {
//            json secondary_data;
//            secondary_file >> secondary_data;
//            secondary_file.close();
//
//            // 为二次规划数据添加标记
//            for (auto& sample : secondary_data) {
//                if (sample.contains("features")) {
//                    sample["features"]["is_secondary_planning"] = true;
//                }
//            }
//
//            merged_data.insert(merged_data.end(), secondary_data.begin(), secondary_data.end());
//        }
//    }
//
//    return merged_data;
//}
//
//// 计算类别权重函数
//std::map<std::string, float> calculate_class_weights(const json& training_data,
//    const std::vector<std::string>& algorithms) {
//    std::map<std::string, float> class_weights;
//    std::map<std::string, int> class_counts;
//
//    // 初始化所有类别的计数为0
//    for (const auto& algo : algorithms) {
//        class_counts[algo] = 0;
//    }
//
//    // 统计每个类别的样本数量
//    for (const auto& sample : training_data) {
//        if (sample.contains("best_algorithm")) {
//            std::string algo = sample["best_algorithm"];
//            if (std::find(algorithms.begin(), algorithms.end(), algo) != algorithms.end()) {
//                class_counts[algo]++;
//            }
//        }
//    }
//
//    // 计算总样本数
//    int total_samples = training_data.size();
//
//    // 打印类别统计信息
//    std::cout << "\n类别统计信息:" << std::endl;
//    std::cout << "总训练样本数: " << total_samples << std::endl;
//    std::cout << "各类别样本数量:" << std::endl;
//
//    for (const auto& algo : algorithms) {
//        int count = class_counts[algo];
//        std::cout << "  " << algo << ": " << count;
//
//        if (count == 0) {
//            std::cout << " (无训练数据，权重设为0)";
//            class_weights[algo] = 0.0f;
//        }
//        else {
//            // 计算权重：总样本数 / (类别数 * 该类别样本数)
//            // 使用类别平衡权重，减少类别不平衡的影响
//            float weight = static_cast<float>(total_samples) / (algorithms.size() * count);
//            class_weights[algo] = weight;
//            std::cout << " (权重: " << weight << ")";
//        }
//        std::cout << std::endl;
//    }
//
//    return class_weights;
//}
//
//int main() {
//    try {
//        std::cout << "==========================================" << std::endl;
//        std::cout << "两层机器学习推荐系统 - 算法选择与参数优化" << std::endl;
//        std::cout << "支持一次规划和二次规划场景" << std::endl;
//        std::cout << "==========================================" << std::endl;
//
//        // 路径配置
//        std::string scenarios_dir = "../data/scenarios/";
//        std::string training_scenarios_dir = "../data/training_scenarios/";
//        std::string test_scenarios_dir = "../data/test_scenarios/";
//        std::string training_data_dir = "../data/training/";
//        std::string models_dir = "../models/";
//        std::string results_dir = "../models/results/";
//
//        // 二次规划相关路径
//        std::string secondary_scenarios_dir = "../data/secondary_scenarios/";
//        std::string secondary_training_dir = "../data/secondary_training/";
//        std::string secondary_test_dir = "../data/secondary_test/";
//        std::string prev_plan_dir = "../modelsForData/results/";
//
//        // 可用的算法列表
//        std::vector<std::string> algorithms = { "Greedy_Time", "Greedy_Cost","Greedy_Difference", "NSGAII","Exact_Time","Exact_Cost" };
//
//        // 第一步：检查场景数据
//        if (!fs::exists(scenarios_dir)) {
//            std::cout << "一次规划场景数据目录不存在" << std::endl;
//            std::cout << "请先运行数据生成器生成场景数据" << std::endl;
//            return 1;
//        }
//
//        // 第二步：检查是否需要分割一次规划数据集
//        if (need_dataset_split(training_scenarios_dir, test_scenarios_dir, scenarios_dir)) {
//            std::cout << "\n开始分割一次规划数据集 (70%训练, 30%测试)..." << std::endl;
//            split_dataset(scenarios_dir, training_scenarios_dir, test_scenarios_dir, algorithms, 0.7, false);
//        }
//        else {
//            std::cout << "\n一次规划数据集已分割或不需要分割" << std::endl;
//        }
//
//        // 检查二次规划数据
//        bool has_secondary_scenarios = fs::exists(secondary_scenarios_dir);
//        bool need_secondary_split = false;
//
//        if (has_secondary_scenarios) {
//            std::cout << "\n检测到二次规划场景数据" << std::endl;
//            need_secondary_split = need_dataset_split(secondary_training_dir, secondary_test_dir, secondary_scenarios_dir);
//
//            if (need_secondary_split) {
//                std::cout << "\n开始分割二次规划数据集 (70%训练, 30%测试)..." << std::endl;
//                split_dataset(secondary_scenarios_dir, secondary_training_dir, secondary_test_dir, algorithms, 0.7, true);
//            }
//            else {
//                std::cout << "\n二次规划数据集已分割或不需要分割" << std::endl;
//            }
//        }
//
//        // 第三步：统计数据集信息
//        int training_scenario_count = 0;
//        int test_scenario_count = 0;
//        int secondary_training_count = 0;
//        int secondary_test_count = 0;
//
//        if (fs::exists(training_scenarios_dir)) {
//            for (const auto& entry : fs::directory_iterator(training_scenarios_dir)) {
//                if (entry.path().extension() == ".json" &&
//                    entry.path().filename().string().find("scenario_") == 0) {
//                    training_scenario_count++;
//                }
//            }
//        }
//
//        if (fs::exists(test_scenarios_dir)) {
//            for (const auto& entry : fs::directory_iterator(test_scenarios_dir)) {
//                if (entry.path().extension() == ".json" &&
//                    entry.path().filename().string().find("scenario_") == 0) {
//                    test_scenario_count++;
//                }
//            }
//        }
//
//        if (has_secondary_scenarios && fs::exists(secondary_training_dir)) {
//            for (const auto& entry : fs::directory_iterator(secondary_training_dir)) {
//                if (entry.path().extension() == ".json" &&
//                    entry.path().filename().string().find("secondary_") == 0) {
//                    secondary_training_count++;
//                }
//            }
//        }
//
//        if (has_secondary_scenarios && fs::exists(secondary_test_dir)) {
//            for (const auto& entry : fs::directory_iterator(secondary_test_dir)) {
//                if (entry.path().extension() == ".json" &&
//                    entry.path().filename().string().find("secondary_") == 0) {
//                    secondary_test_count++;
//                }
//            }
//        }
//
//        std::cout << "\n数据集统计:" << std::endl;
//        std::cout << "  一次规划训练集场景: " << training_scenario_count << std::endl;
//        std::cout << "  一次规划测试集场景: " << test_scenario_count << std::endl;
//        if (has_secondary_scenarios) {
//            std::cout << "  二次规划训练集场景: " << secondary_training_count << std::endl;
//            std::cout << "  二次规划测试集场景: " << secondary_test_count << std::endl;
//        }
//        std::cout << "  总计: " << (training_scenario_count + test_scenario_count +
//            secondary_training_count + secondary_test_count) << std::endl;
//
//        // 第四步：初始化推荐器
//        std::cout << "\n初始化推荐器..." << std::endl;
//        TwoLayerRecommender recommender(models_dir);
//
//        // 第五步：检查是否需要生成训练数据
//        std::string primary_data_file = training_data_dir + "algorithm_training_data.json";
//        std::string secondary_data_file = training_data_dir + "secondary_training_data.json";
//        std::string merged_data_file = training_data_dir + "merged_training_data.json";
//        std::string model_file = models_dir + "algorithm_recommender.yml";
//        std::string training_model_file = training_data_dir + "algorithm_recommender.yml";
//
//        bool need_training = true;
//
//        if (fs::exists(model_file)) {
//            std::cout << "\n找到已部署模型，加载..." << std::endl;
//            if (recommender.load_pretrained_model(model_file)) {
//                std::cout << "模型加载成功" << std::endl;
//                need_training = false;
//            }
//            else {
//                std::cout << "模型加载失败，需要重新训练" << std::endl;
//            }
//        }
//        else if (fs::exists(training_model_file)) {
//            std::cout << "\n找到训练模型，加载并部署..." << std::endl;
//            if (recommender.load_pretrained_model(training_model_file)) {
//                std::cout << "训练模型加载成功，复制到部署目录..." << std::endl;
//                fs::copy_file(training_model_file, model_file, fs::copy_options::overwrite_existing);
//                need_training = false;
//            }
//        }
//
//        // 第六步：训练模型（如果需要）
//        if (need_training && (training_scenario_count > 0 || secondary_training_count > 0)) {
//            std::cout << "\n开始训练模型..." << std::endl;
//
//            bool need_generate_data = false;
//
//            // 检查是否需要生成一次规划训练数据
//            if (!fs::exists(primary_data_file) && training_scenario_count > 0) {
//                std::cout << "生成一次规划训练数据..." << std::endl;
//                create_training_data_from_existing_scenarios(
//                    training_scenarios_dir, algorithms,
//                    training_scenario_count,
//                    primary_data_file,
//                    false,//is_secondary_scenario= false
//                    results_dir,//算法结果的保存路径
//                    true);//is_save_result= true是否需要保存算法结果
//                need_generate_data = true;
//            }
//
//            else {
//                std::cout << "一次规划训练数据已存在" << std::endl;
//            }
//            //如果分析结果文件不存在，则重新生成
//            string comparison_dir = fs::path(results_dir).string() + "/comparison_results/";
//            string overall_statistics_by_category_dir = fs::path(results_dir).string() + "/comparison_results/overall_statistics_by_category.json";
//            if (!fs::exists(overall_statistics_by_category_dir)) {
//                // 检查是否有对比结果文件
//                int comparison_file_count = 0;
//                for (const auto& entry : fs::directory_iterator(comparison_dir)) {
//                    if (entry.path().extension() == ".json" &&
//                        entry.path().filename().string().find("_comparison.json") != std::string::npos) {
//                        comparison_file_count++;
//                    }
//                }
//
//                if (comparison_file_count > 0) {
//                    std::cout << "找到 " << comparison_file_count << " 个对比结果文件" << std::endl;
//
//                    // 调用总体分析函数
//                    try {
//                        auto overall_stats = AlgorithmEvaluator::analyze_overall_statistics(comparison_dir);
//
//                        // 将总体统计添加到训练数据文件中
//                        //json training_data_with_stats;
//                        //std::ifstream in_file(training_data_file);
//                        //if (in_file.is_open()) {
//                        //    in_file >> training_data_with_stats;
//                        //    in_file.close();
//
//                        //    training_data_with_stats["overall_comparison_stats"] = overall_stats;
//
//                        //    std::ofstream stats_file(training_data_file);
//                        //    if (stats_file.is_open()) {
//                        //        stats_file << training_data_with_stats.dump(4);
//                        //        stats_file.close();
//                        //        std::cout << "总体对比统计已添加到训练数据文件" << std::endl;
//                        //    }
//                        //}
//
//                        // 打印关键统计
//                        std::cout << "\n=== 算法对比总体统计 ===" << std::endl;
//                        std::cout << "总场景数: " << overall_stats["total_scenarios"] << std::endl;
//                        std::cout << "有效场景数: " << overall_stats["scenarios_with_data"] << std::endl;
//
//                        if (overall_stats.contains("algorithm_performance")) {
//                            std::cout << "\n各算法表现:" << std::endl;
//                            for (const auto& [algo, stats] : overall_stats["algorithm_performance"].items()) {
//                                double success_rate = stats.value("success_rate", 0.0);
//                                double avg_score = stats.value("avg_total_score", 0.0);
//                                double avg_time = stats.value("avg_computation_time", 0.0);
//
//                                std::cout << "  " << std::setw(15) << std::left << algo
//                                    << ": 成功率=" << std::fixed << std::setprecision(1) << success_rate * 100 << "%"
//                                    << ", 平均分=" << std::fixed << std::setprecision(3) << avg_score
//                                    << ", 平均耗时=" << std::fixed << std::setprecision(2) << avg_time << "秒" << std::endl;
//                            }
//                        }
//
//                    }
//                    catch (const std::exception& e) {
//                        std::cerr << "生成总体分析报告时出错: " << e.what() << std::endl;
//                    }
//                }
//                else {
//                    std::cout << "警告: 未找到对比结果文件，跳过总体分析" << std::endl;
//                }
//            }
//            // 检查是否需要生成二次规划训练数据
//            if (!fs::exists(secondary_data_file) && secondary_training_count > 0) {
//                std::cout << "生成二次规划训练数据..." << std::endl;
//                // 注意：这里需要调用专门处理二次规划的函数
//                // 由于create_training_data_from_existing_scenarios不支持二次规划，
//                // 你可能需要创建一个新函数或修改现有函数
//                create_training_data_from_existing_scenarios(
//                    secondary_training_dir, algorithms,
//                    secondary_training_count,
//                    secondary_data_file,
//                    true,//is_secondary_scenario= true
//                    results_dir,//算法结果的保存路径
//                    true);//is_save_result= true是否需要保存算法结果
//            }
//
//            // 合并一次和二次规划数据
//            if (!fs::exists(merged_data_file) &&
//                (fs::exists(primary_data_file) || fs::exists(secondary_data_file))) {
//                std::cout << "合并一次和二次规划训练数据..." << std::endl;
//                json merged_data = merge_training_data(primary_data_file, secondary_data_file);
//
//                std::ofstream merged_file(merged_data_file);
//                if (merged_file.is_open()) {
//                    merged_file << merged_data.dump(4);
//                    merged_file.close();
//                    std::cout << "合并后的训练数据已保存到: " << merged_data_file << std::endl;
//                }
//            }
//
//            // 使用合并后的数据进行训练
//            if (fs::exists(merged_data_file)) {
//                // 计算类别权重
//                std::ifstream merged_file(merged_data_file);
//                json merged_data;
//                merged_file >> merged_data;
//                merged_file.close();
//
//                std::map<std::string, float> class_weights = calculate_class_weights(merged_data, algorithms);
//
//                std::map<std::string, float> filtered_class_weights;
//                for (const auto& [algo, weight] : class_weights) {
//                    if (weight > 0) {
//                        filtered_class_weights[algo] = weight;
//                    }
//                    else {
//                        std::cout << "排除权重为0的类别: " << algo << std::endl;
//                    }
//                }
//                //不使用权重
//                filtered_class_weights = {};
//                // 然后使用过滤后的权重
//                if (recommender.train_algorithm_recommender(training_data_dir, filtered_class_weights)) {
//
//                //// 训练模型时传入类别权重
//                //if (recommender.train_algorithm_recommender(training_data_dir, class_weights)) {
//                    std::cout << "模型训练成功" << std::endl;
//
//                    if (fs::exists(training_model_file) && !fs::exists(model_file)) {
//                        fs::copy_file(training_model_file, model_file, fs::copy_options::overwrite_existing);
//                        std::cout << "模型已部署到: " << model_file << std::endl;
//                    }
//                }
//                else {
//                    std::cerr << "模型训练失败，使用默认规则" << std::endl;
//                }
//            }
//            else {
//                std::cerr << "无法生成训练数据" << std::endl;
//            }
//        }
//        else if (need_training) {
//            std::cout << "\n警告: 没有训练数据，无法训练模型" << std::endl;
//        }
//
//        // 第七步：处理测试场景
//        if (test_scenario_count > 0) {
//            // 检查测试结果是否已存在
//            bool all_processed = true;
//            std::vector<std::string> test_scenario_files;
//            std::string output_dir = models_dir + "results/";
//
//            // 获取测试场景文件列表
//            for (const auto& entry : std::filesystem::directory_iterator(test_scenarios_dir)) {
//                if (entry.path().extension() == ".json") {
//                    std::string scenario_name = entry.path().stem().string();
//                    if (scenario_name.find("test_labels") != std::string::npos
//                        || scenario_name.find("model_evaluation_results") != std::string::npos
//                        || scenario_name.find("scenario_features") != std::string::npos) {
//                        continue;
//                    }
//                    test_scenario_files.push_back(scenario_name);
//
//                    // 检查该场景是否已被处理（查找前缀匹配的文件）
//                    bool scenario_processed = false;
//                    for (const auto& result_entry : std::filesystem::directory_iterator(output_dir)) {
//                        if (result_entry.path().extension() == ".json") {
//                            std::string filename = result_entry.path().filename().string();
//                            // 检查文件名是否以 execution_scenario_name_ 开头
//                            std::string prefix = "execution_" + scenario_name + "_";
//                            if (filename.find(prefix) == 0) {
//                                scenario_processed = true;
//                                break;
//                            }
//                        }
//                    }
//
//                    if (!scenario_processed) {
//                        all_processed = false;
//                        break;
//                    }
//                }
//            }
//            if (!all_processed) {
//                std::cout << "\n处理一次规划测试场景..." << std::endl;
//                recommender.process_scenarios_batch(test_scenarios_dir);
//            }
//            else {
//                std::cout << "\n所有一次规划测试场景已处理，跳过..." << std::endl;
//            }
//        }
//        else if (training_scenario_count > 0) {
//            std::cout << "\n一次规划测试集为空，使用部分训练场景测试..." << std::endl;
//            recommender.process_scenarios_batch(training_scenarios_dir);
//        }
//        else {
//            std::cout << "\n警告: 没有可用的一次规划测试场景" << std::endl;
//        }
//
//        // 处理二次规划测试场景
//        if (has_secondary_scenarios && secondary_test_count > 0) {
//            // 检查二次规划测试结果是否已存在
//            bool all_processed = true;
//            std::vector<std::string> test_scenario_files;
//            std::string output_dir = models_dir + "results/";
//
//            // 获取二次规划测试场景文件列表
//            for (const auto& entry : std::filesystem::directory_iterator(secondary_test_dir)) {
//                if (entry.path().extension() == ".json") {
//                    std::string scenario_name = entry.path().stem().string();
//                    if (scenario_name.find("test_labels") != std::string::npos
//                        || scenario_name.find("model_evaluation_results") != std::string::npos
//                        || scenario_name.find("scenario_features") != std::string::npos) {
//                        continue;
//                    }
//                    test_scenario_files.push_back(scenario_name);
//
//                    // 检查该场景是否已被处理
//                    bool scenario_processed = false;
//
//                    // 方式1：检查是否有任意包含场景名的文件
//                    for (const auto& result_entry : std::filesystem::directory_iterator(output_dir)) {
//                        if (result_entry.path().extension() == ".json") {
//                            std::string filename = result_entry.path().filename().string();
//                            // 检查文件名是否包含场景名
//                            if (filename.find(scenario_name) != std::string::npos) {
//                                scenario_processed = true;
//                                break;
//                            }
//                        }
//                    }
//                    if (!scenario_processed) {
//                        all_processed = false;
//                        break;
//                    }
//                }
//            }
//
//            if (!all_processed) {
//                std::cout << "\n处理二次规划测试场景..." << std::endl;
//                recommender.process_secondary_scenarios(secondary_test_dir);
//            }
//            else {
//                std::cout << "\n所有二次规划测试场景已处理，跳过..." << std::endl;
//            }
//        }
//        else if (has_secondary_scenarios && secondary_training_count > 0) {
//            std::cout << "\n二次规划测试集为空，使用部分训练场景测试..." << std::endl;
//            recommender.process_secondary_scenarios(secondary_training_dir);
//        }
//
//        // 第八步：模型评估（新增）
//        bool has_primary_labels = fs::exists(test_scenarios_dir + "test_labels.json");
//        bool has_secondary_labels = fs::exists(secondary_test_dir + "secondary_test_labels.json");
//
//        if (has_primary_labels || has_secondary_labels) {
//            std::cout << "\n开始模型评估..." << std::endl;
//
//            // 创建测试评估器
//            AlgorithmRecommender evaluator;
//            if (evaluator.load_pretrained_model(model_file)) {
//                // 合并评估一次规划和二次规划
//                std::cout << "\n评估合并场景（包含一次规划和二次规划）..." << std::endl;
//                auto test_result = evaluator.evaluate_model(test_scenarios_dir, secondary_test_dir);
//
//                // 保存评估结果
//                std::string eval_result_file = models_dir + "model_evaluation_merged.json";
//                std::ofstream eval_file(eval_result_file);
//                if (eval_file.is_open()) {
//                    // 计算各个算法的准确率
//                    json algorithm_accuracy = json::object();
//                    for (const auto& algo : algorithms) {
//                        if (test_result.total_by_algorithm[algo] > 0) {
//                            double acc = static_cast<double>(test_result.correct_by_algorithm[algo]) /
//                                test_result.total_by_algorithm[algo];
//                            algorithm_accuracy[algo] = acc;
//                        }
//                        else {
//                            algorithm_accuracy[algo] = 0.0;
//                        }
//                    }
//
//                    json eval_json = {
//                        {"total_accuracy", test_result.accuracy},
//                        {"primary_accuracy", test_result.primary_accuracy},
//                        {"secondary_accuracy", test_result.secondary_accuracy},
//                        {"primary_test_count", test_result.primary_total},
//                        {"secondary_test_count", test_result.secondary_total},
//                        {"total_test_count", test_result.primary_total + test_result.secondary_total},
//                        {"confusion_matrix", test_result.confusion_matrix},
//                        {"algorithm_accuracy", algorithm_accuracy},
//                        {"primary_by_algorithm", test_result.primary_by_algorithm},
//                        {"secondary_by_algorithm", test_result.secondary_by_algorithm},
//                        {"evaluation_time", std::chrono::system_clock::now().time_since_epoch().count()},
//                        {"planning_type", "merged"}
//                    };
//
//                    eval_file << eval_json.dump(4);
//                    eval_file.close();
//                    std::cout << "合并评估结果已保存到: " << eval_result_file << std::endl;
//
//                    // 打印汇总信息
//                    std::cout << "\n============ 模型评估汇总 ============" << std::endl;
//                    std::cout << "总准确率: " << test_result.accuracy * 100 << "%" << std::endl;
//                    std::cout << "一次规划准确率: " << test_result.primary_accuracy * 100 << "%"
//                        << " (" << test_result.primary_total << " 个样本)" << std::endl;
//                    std::cout << "二次规划准确率: " << test_result.secondary_accuracy * 100 << "%"
//                        << " (" << test_result.secondary_total << " 个样本)" << std::endl;
//                    std::cout << "======================================" << std::endl;
//                }
//            }
//            else {
//                std::cerr << "无法加载模型进行评估" << std::endl;
//            }
//        }
//        else {
//            std::cout << "\n警告: 没有测试标签文件，无法计算准确率" << std::endl;
//            std::cout << "请先生成测试标签" << std::endl;
//        }
//
//        // 第九步：保存状态和报告
//        std::string state_file = models_dir + "recommender_state.json";
//        if (recommender.save_state(state_file)) {
//            std::cout << "\n状态已保存到: " << state_file << std::endl;
//        }
//
//        auto stats = recommender.get_statistics();
//        std::cout << "\n系统统计信息:\n" << stats.dump(4) << std::endl;
//
//        // 生成性能报告
//        json report = {
//            {"dataset_info", {
//                {"primary_training_scenarios", training_scenario_count},
//                {"primary_test_scenarios", test_scenario_count},
//                {"secondary_training_scenarios", secondary_training_count},
//                {"secondary_test_scenarios", secondary_test_count},
//                {"total_scenarios", training_scenario_count + test_scenario_count +
//                                  secondary_training_count + secondary_test_count}
//            }},
//            {"system_stats", stats},
//            {"execution_time", std::chrono::system_clock::now().time_since_epoch().count()}
//        };
//
//        std::string report_file = models_dir + "performance_report.json";
//        std::ofstream report_stream(report_file);
//        if (report_stream.is_open()) {
//            report_stream << report.dump(4);
//            report_stream.close();
//            std::cout << "性能报告已保存到: " << report_file << std::endl;
//        }
//
//        std::cout << "\n==========================================" << std::endl;
//        std::cout << "系统运行完成" << std::endl;
//        std::cout << "==========================================" << std::endl;
//        return 0;
//    }
//    catch (const std::exception& e) {
//        std::cerr << "系统错误: " << e.what() << std::endl;
//        return 1;
//    }
//}