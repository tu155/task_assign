// HyperparameterOptimizer.cpp - 超参数优化器实现
#include "HyperparameterOptimizer.h"
#include "DataIO.h"
#include "Util.h"
#include "Model.h"
#include "NSGAII.h"
#include "Greedy.h"
#include "Schedule.h"
#include "Mount.h"
#include "Load.h"
#include "Fleet.h"
#include "Target.h"       
#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>
#include <filesystem>
#include <regex>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ==================== ScenarioFeatureExtractor 实现 ====================
// ==================== 私有工具函数实现 ====================

// 计算分布统计量
ScenarioFeatureExtractor::DistributionStats
ScenarioFeatureExtractor::calculate_distribution_stats(const std::vector<float>& values) {
    DistributionStats stats;

    if (values.empty()) {
        return stats;
    }

    // 计算基本统计量
    stats.mean = calculate_mean(values);
    stats.std_dev = calculate_std_dev(values, stats.mean);
    stats.min = *std::min_element(values.begin(), values.end());
    stats.max = *std::max_element(values.begin(), values.end());
    stats.percentiles = calculate_percentiles(values);

    // 计算偏度和峰度
    if (stats.std_dev > 0 && values.size() > 1) {
        stats.skewness = calculate_skewness(values, stats.mean, stats.std_dev);
        stats.kurtosis = calculate_kurtosis(values, stats.mean, stats.std_dev);
    }

    return stats;
}

float ScenarioFeatureExtractor::calculate_mean(const std::vector<float>& values) {
    if (values.empty()) return 0.0f;
    float sum = 0.0f;
    for (float val : values) {
        sum += val;
    }
    return sum / values.size();
}

float ScenarioFeatureExtractor::calculate_std_dev(const std::vector<float>& values, float mean) {
    if (values.size() < 2) return 0.0f;

    float variance = 0.0f;
    for (float val : values) {
        variance += (val - mean) * (val - mean);
    }
    return std::sqrt(variance / values.size());
}

std::vector<float> ScenarioFeatureExtractor::calculate_percentiles(const std::vector<float>& values) {
    std::vector<float> percentiles;
    if (values.empty()) return percentiles;

    auto sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());

    // 计算25th, 50th, 75th百分位数
    const std::vector<float> quantiles = { 0.25f, 0.5f, 0.75f };
    for (float q : quantiles) {
        int idx = static_cast<int>(sorted_values.size() * q);
        if (idx >= sorted_values.size()) idx = sorted_values.size() - 1;
        percentiles.push_back(sorted_values[idx]);
    }

    return percentiles;
}

float ScenarioFeatureExtractor::calculate_skewness(const std::vector<float>& values, float mean, float std_dev) {
    if (values.size() < 2 || std_dev == 0) return 0.0f;

    float skew_sum = 0.0f;
    for (float val : values) {
        float z = (val - mean) / std_dev;
        skew_sum += z * z * z;
    }
    return skew_sum / values.size();
}

float ScenarioFeatureExtractor::calculate_kurtosis(const std::vector<float>& values, float mean, float std_dev) {
    if (values.size() < 2 || std_dev == 0) return 0.0f;

    float kurt_sum = 0.0f;
    for (float val : values) {
        float z = (val - mean) / std_dev;
        kurt_sum += z * z * z * z;
    }
    // 计算超额峰度
    return (kurt_sum / values.size()) - 3.0f;
}

// 解析坐标字符串（如："116.461141,39.994166"）
std::pair<double, double> ScenarioFeatureExtractor::parse_coordinates(const std::string& coord_str) {
    std::pair<double, double> coord = { 0.0, 0.0 };
    size_t comma_pos = coord_str.find(',');
    if (comma_pos != std::string::npos) {
        try {
            coord.first = std::stod(coord_str.substr(0, comma_pos));
            coord.second = std::stod(coord_str.substr(comma_pos + 1));
        }
        catch (const std::exception& e) {
            std::cerr << "解析坐标失败: " << coord_str << ", 错误: " << e.what() << std::endl;
        }
    }
    return coord;
}

// 计算两点间距离（简化版，使用欧几里得距离）
double ScenarioFeatureExtractor::calculate_distance(
    const std::pair<double, double>& p1,
    const std::pair<double, double>& p2) {

    double dx = p1.first - p2.first;
    double dy = p1.second - p2.second;
    return std::sqrt(dx * dx + dy * dy);
}

// 计算目标之间的距离
std::vector<float> ScenarioFeatureExtractor::calculate_target_distances(
    const std::vector<std::pair<double, double>>& positions) {

    std::vector<float> distances;

    if (positions.size() < 2) {
        return distances;
    }

    // 计算所有目标到基地的距离
    // 假设基地位置在 (116.475260, 40.038568) - 从base_position获取
    std::pair<double, double> base_pos = { 116.475260, 40.038568 };

    for (const auto& target_pos : positions) {
        float dist = static_cast<float>(calculate_distance(target_pos, base_pos));
        distances.push_back(dist);
    }

    return distances;
}

// 计算空间聚类系数
float ScenarioFeatureExtractor::calculate_spatial_clustering(
    const std::vector<std::pair<double, double>>& positions) {

    if (positions.size() < 3) {
        return 0.0f;
    }

    // 计算平均最近邻距离
    std::vector<float> nearest_distances;
    for (size_t i = 0; i < positions.size(); ++i) {
        float min_dist = std::numeric_limits<float>::max();
        for (size_t j = 0; j < positions.size(); ++j) {
            if (i == j) continue;
            float dist = static_cast<float>(getDistance(positions[i].first,positions[i].second,positions[j].first, positions[j].second));
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        if (min_dist < std::numeric_limits<float>::max()) {
            nearest_distances.push_back(min_dist);
        }
    }

    if (nearest_distances.empty()) {
        return 0.0f;
    }

    // 计算平均最近邻距离
    float avg_nearest_dist = calculate_mean(nearest_distances);

    // 转换为聚类系数：距离越小，聚类程度越高
    // 使用sigmoid函数归一化到0-1
    float clustering = 1.0f / (1.0f + avg_nearest_dist / 10.0f);

    return clustering;
}

// 计算目标密度
float ScenarioFeatureExtractor::calculate_target_density(
    const std::vector<std::pair<double, double>>& positions) {

    if (positions.empty()) {
        return 0.0f;
    }

    // 计算包围盒面积
    double min_x = positions[0].first, max_x = positions[0].first;
    double min_y = positions[0].second, max_y = positions[0].second;

    for (const auto& pos : positions) {
        if (pos.first < min_x) min_x = pos.first;
        if (pos.first > max_x) max_x = pos.first;
        if (pos.second < min_y) min_y = pos.second;
        if (pos.second > max_y) max_y = pos.second;
    }
    // 使用平均纬度处的经度-距离转换
    double avg_lat = (min_y + max_y) / 2.0;
    double width = getDistance(min_x, avg_lat, max_x, avg_lat);

    // 使用平均经度处的纬度-距离转换
    double avg_lng = (min_x + max_x) / 2.0;
    double height = getDistance(avg_lng, min_y, avg_lng, max_y);

    double area = width * height;  // 简化计算，不考虑地球曲率

    // 避免除零
    if (area < 0.0001) {
        area = 0.0001;
    }

    return static_cast<float>(positions.size() / area);
}

// 提取leadtime（前置时间）数据
std::vector<float> ScenarioFeatureExtractor::extract_leadtimes(const std::vector<Target*>& targets) {
    std::vector<float> leadtimes;

    for (auto target : targets) {
        // 假设Target类有getLeadTime()方法
         float leadtime = target->getTimeRange().second-target->getTimeRange().first;
         leadtimes.push_back(leadtime);

        // 暂时使用示例值，需要根据实际数据结构调整
        //leadtimes.push_back(30.0f);  // 示例值
    }

    return leadtimes;
}

// 计算资源容量统计
ScenarioFeatureExtractor::DistributionStats
ScenarioFeatureExtractor::calculate_resource_capacity_stats(const std::vector<Fleet*>& resources) {
    std::vector<float> capacities;

    for (auto resource : resources) {
        try {
            // 假设Fleet类有getCapacity()或getWeaponNum()方法
            // 从JSON数据看，有weapon_num字段
            float capacity = resource->getMountPlanList().size();
            capacities.push_back(capacity);
        }
        catch(...){
            // 暂时使用示例值
            capacities.push_back(30.0f);  // 示例值
        }
    }
    // 如果没有数据，添加一些默认值避免空向量
    if (capacities.empty() && !resources.empty()) {
        for (size_t i = 0; i < resources.size(); ++i) {
            capacities.push_back(1.0f);
        }
    }

    return calculate_distribution_stats(capacities);
}

// 计算成本统计
ScenarioFeatureExtractor::DistributionStats
ScenarioFeatureExtractor::calculate_cost_stats(const std::vector<Fleet*>& resources) {
    std::vector<float> costs;

    for (auto resource : resources) {
        try {
            // 假设Fleet类有getCost()方法
            // 从JSON数据看，有fuel_cost和setup_cost

            float cost = resource->getCostUse() + resource->getCostSpeed();;
            costs.push_back(cost);
        }
        catch(...){
            // 暂时使用示例值
            costs.push_back(1.0f);  // 示例值
        }
    }
    // 如果没有数据，添加一些默认值避免空向量
    if (costs.empty() && !resources.empty()) {
        for (size_t i = 0; i < resources.size(); ++i) {
            costs.push_back(1.0f);
        }
    }
    return calculate_distribution_stats(costs);
}


cv::Mat ScenarioFeatureExtractor::ScenarioFeatures::to_mat() const {
    // 18个特征
    cv::Mat mat(1, 31, CV_32F);

    int idx = 0;
    // 基础特征 (7个)
    mat.at<float>(idx++) = static_cast<float>(num_targets);
    mat.at<float>(idx++) = static_cast<float>(num_resources);
    mat.at<float>(idx++) = static_cast<float>(num_target_types);
    mat.at<float>(idx++) = static_cast<float>(num_resource_types);
    mat.at<float>(idx++) = static_cast<float>(decision_preferance);
    mat.at<float>(idx++) = static_cast<float>(target_time_urgency);
    mat.at<float>(idx++) = static_cast<float>(resource_target_ratio);

    // 统计特征 (8个)
    mat.at<float>(idx++) = target_distance_stats.mean;
    mat.at<float>(idx++) = target_distance_stats.std_dev;
    mat.at<float>(idx++) = resource_capacity_stats.mean;
    mat.at<float>(idx++) = resource_capacity_stats.std_dev;
    mat.at<float>(idx++) = time_window_stats.mean;
    mat.at<float>(idx++) = time_window_stats.std_dev;
    mat.at<float>(idx++) = cost_distribution_stats.mean;
    mat.at<float>(idx++) = cost_distribution_stats.std_dev;

    // 拓扑和复杂度特征 (5个)
    mat.at<float>(idx++) = spatial_clustering_coefficient;
    //mat.at<float>(idx++) = resource_target_distance_mean;
    mat.at<float>(idx++) = target_density;
    mat.at<float>(idx++) = problem_complexity_score;
    mat.at<float>(idx++) = constraint_tightness;

    // 二次规划特征 (3个)
    mat.at<float>(idx++) = is_secondary_planning ? 1.0f : 0.0f;
    mat.at<float>(idx++) = target_change_ratio;
    mat.at<float>(idx++) = static_cast<float>(original_target_count);

    // 历史性能特征 (4个)
    mat.at<float>(idx++) = historical_metrics.previous_coverage;
    mat.at<float>(idx++) = historical_metrics.previous_cost;
    mat.at<float>(idx++) = historical_metrics.previous_time;
    mat.at<float>(idx++) = historical_metrics.previous_resource_utilization;

    // 分位数特征 (5个位置)
    for (size_t i = 0; i < 3 && i < target_distance_stats.percentiles.size(); i++) {
        mat.at<float>(idx++) = target_distance_stats.percentiles[i];
    }
    for (size_t i = 0; i < 2 && i < time_window_stats.percentiles.size(); i++) {
        mat.at<float>(idx++) = time_window_stats.percentiles[i];
    }

    // 确保所有特征都被填充
    while (idx < 31) {
        mat.at<float>(idx++) = 0.0f;
    }
    return mat;
}

json ScenarioFeatureExtractor::ScenarioFeatures::to_json() const {
    json result= {
        {"scenario_name", scenario_name},
        {"num_targets", num_targets},
        {"num_resources", num_resources},
        {"num_target_types", num_target_types},
        {"num_resource_types", num_resource_types},
        {"decision_preferance", decision_preferance},
        {"target_time_urgency", target_time_urgency},
        {"resource_target_ratio", resource_target_ratio},

        // 统计特征
        {"statistical_features", {
            {"target_distance", {
                {"mean", target_distance_stats.mean},
                {"std_dev", target_distance_stats.std_dev},
                {"min", target_distance_stats.min},
                {"max", target_distance_stats.max},
                {"percentiles", target_distance_stats.percentiles}
            }},
            {"resource_capacity", {
                {"mean", resource_capacity_stats.mean},
                {"std_dev", resource_capacity_stats.std_dev}
            }},
            {"time_window", {
                {"mean", time_window_stats.mean},
                {"std_dev", time_window_stats.std_dev},
                {"percentiles", time_window_stats.percentiles}
            }},
            {"cost_distribution", {
                {"mean", cost_distribution_stats.mean},
                {"std_dev", cost_distribution_stats.std_dev}
            }}
        }},

        // 拓扑特征
        {"topological_features", {
            {"spatial_clustering", spatial_clustering_coefficient},
            {"target_density", target_density}
        }},

        // 复杂度特征
        {"complexity_features", {
            {"problem_complexity", problem_complexity_score},
            {"constraint_tightness", constraint_tightness},
        }},

        // 二次规划特征
        {"secondary_planning_features", {
            {"is_secondary_planning", is_secondary_planning},
            {"target_change_ratio", target_change_ratio},
            {"original_target_count", original_target_count}
        }},

        // 历史性能特征
        {"historical_metrics", {
            {"previous_coverage", historical_metrics.previous_coverage},
            {"previous_cost", historical_metrics.previous_cost},
            {"previous_time", historical_metrics.previous_time},
            {"previous_resource_utilization", historical_metrics.previous_resource_utilization},
        }}
    };

    return result;
}

json ScenarioFeatureExtractor::get_default_features_template() {
    return {
        {"scenario_name", ""},
        {"num_targets", 0},
        {"num_resources", 0},
        {"num_target_types", 0},
        {"num_resource_types", 0},
        {"decision_preferance", 0},
        {"target_time_urgency", 0.0},
        {"resource_target_ratio", 0.0}
    };
}

ScenarioFeatureExtractor::ScenarioFeatures
ScenarioFeatureExtractor::extract_features(Schedule* schedule,
    const std::string& scenario_name,const std::map<string,float>&features_map) {

    ScenarioFeatures features;
    features.scenario_name = scenario_name;

    // 从schedule获取数据
    const auto& target_list = schedule->getTargetList();
    const auto& resource_list = schedule->getFleetList();

    features.num_targets = target_list.size();
    features.num_resources = resource_list.size();
    features.resource_target_ratio =resource_list.size() / target_list.size();
    // 提取目标类型数量
    std::set<std::string> target_types;
    for (auto target : target_list) {
        if (target->getType()) {
            target_types.insert(target->getType()->getName());
        }
    }
    features.num_target_types = target_types.size();

    // 提取资源类型数量
    std::set<std::string> resource_types;
    for (auto resource : resource_list) {
        if (resource->getFleetType()) {
            resource_types.insert(resource->getFleetType()->getName());
        }
    }
    features.num_resource_types = resource_types.size();

    //决策偏好
    features.decision_preferance = features_map.at("decision_preferance");
    // 时间窗口紧度
    features.target_time_urgency = features_map.at("urgency_level");
    
    // 2. 提取统计特征（示例实现，需要根据实际数据结构调整）

    // ==================== 2. 提取统计特征 ====================
    try {
        // 从场景文件中提取坐标信息来计算空间特征
        std::string scenario_file = "../data/scenarios/" + scenario_name + ".json";
        // 如果文件不存在，说明当前在提取二次规划特征，使用二次规划场景文件
        if (!fs::exists(scenario_file)) {
            scenario_file = "../data/secondary_scenarios/" + scenario_name + ".json";
        }
        json scenario_json;

        if (fs::exists(scenario_file)) {
            std::ifstream file(scenario_file);
            if (file.is_open()) {
                file >> scenario_json;
                file.close();

                // 提取目标位置计算空间特征
                if (scenario_json.contains("data") &&
                    scenario_json["data"].contains("target_list")) {

                    const auto& target_data = scenario_json["data"]["target_list"];
                    std::vector<std::pair<double, double>> target_positions;

                    for (const auto& target : target_data) {
                        if (target.contains("target_position")) {
                            std::string pos_str = target["target_position"];
                            auto pos = parse_coordinates(pos_str);
                            target_positions.push_back(pos);
                        }
                    }

                    // 计算目标距离统计
                    if (!target_positions.empty()) {
                        auto distances = calculate_target_distances(target_positions);
                        features.target_distance_stats = calculate_distribution_stats(distances);

                        // 计算空间聚类系数和目标密度
                        features.spatial_clustering_coefficient = calculate_spatial_clustering(target_positions);
                        features.target_density = calculate_target_density(target_positions);
                    }
                }

                // 提取资源信息计算容量统计
                if (scenario_json.contains("data") &&
                    scenario_json["data"].contains("forces_list")) {

                    const auto& forces_data = scenario_json["data"]["forces_list"];
                    std::vector<float> weapon_nums;
                    std::vector<float> fuel_costs;

                    for (const auto& force : forces_data) {
                        if (force.contains("weapon_num")) {
                            std::string weapon_str = force["weapon_num"];
                            // 解析武器数量（可能有逗号分隔的多个值）
                            size_t comma_pos = weapon_str.find(',');
                            if (comma_pos != std::string::npos) {
                                // 取第一个值
                                weapon_str = weapon_str.substr(0, comma_pos);
                            }
                            try {
                                float weapon_num = std::stof(weapon_str);
                                weapon_nums.push_back(weapon_num);
                            }
                            catch (...) {
                                weapon_nums.push_back(30.0f); // 默认值
                            }
                        }

                        if (force.contains("fuel_cost")) {
                            std::string cost_str = force["fuel_cost"];
                            try {
                                float cost = std::stof(cost_str);
                                fuel_costs.push_back(cost);
                            }
                            catch (...) {
                                fuel_costs.push_back(0.5f); // 默认值
                            }
                        }
                    }

                    // 计算资源容量统计
                    if (!weapon_nums.empty()) {
                        features.resource_capacity_stats = calculate_distribution_stats(weapon_nums);
                    }

                    // 计算成本统计
                    if (!fuel_costs.empty()) {
                        features.cost_distribution_stats = calculate_distribution_stats(fuel_costs);
                    }
                }

                // 提取时间窗口信息（leadtime）
                if (scenario_json.contains("data") &&
                    scenario_json["data"].contains("target_list")) {

                    const auto& target_data = scenario_json["data"]["target_list"];
                    std::vector<float> leadtimes;

                    for (const auto& target : target_data) {
                        if (target.contains("leadtime")) {
                            std::string leadtime_str = target["leadtime"];
                            try {
                                float leadtime = std::stof(leadtime_str);
                                leadtimes.push_back(leadtime);
                            }
                            catch (...) {
                                leadtimes.push_back(30.0f); // 默认值
                            }
                        }
                    }

                    // 计算时间窗口统计
                    if (!leadtimes.empty()) {
                        features.time_window_stats = calculate_distribution_stats(leadtimes);
                    }
                }
            }
        }

    }
    catch (const std::exception& e) {
        std::cerr << "提取统计特征时出错: " << e.what() << std::endl;
    }
    // 3. 提取二次规划特征
    // 读取场景JSON文件
    std::string scenario_file = "../data/secondary_scenarios/" + scenario_name + ".json";
    json scenario_json;
    if (fs::exists(scenario_file)) {
        std::ifstream file(scenario_file);
        if (file.is_open()) {
            file >> scenario_json;
            file.close();
        }
        try {
            if (scenario_json.contains("scenario_features")) {
                const auto& scenario_feat = scenario_json["scenario_features"];

                if (scenario_feat.contains("planning_type") &&
                    scenario_feat["planning_type"] == "secondary") {

                    features.is_secondary_planning = true;

                    // 提取二次规划特定特征
                    if (scenario_feat.contains("target_change_ratio")) {
                        features.target_change_ratio = scenario_feat["target_change_ratio"];
                    }

                    if (scenario_feat.contains("original_target_count")) {
                        features.original_target_count = scenario_feat["original_target_count"];
                    }
                    else if (features.target_change_ratio > 0) {
                        // 估算原目标数量
                        features.original_target_count = static_cast<int>(
                            features.num_targets / (1.0f + features.target_change_ratio));
                    }

                    // 提取历史规划性能指标
                    if (scenario_json.contains("historical_plan")) {
                        const auto& historical = scenario_json["historical_plan"];

                        if (historical.contains("target_coverage")) {
                            features.historical_metrics.previous_coverage =
                                historical["target_coverage"];
                        }

                        if (historical.contains("total_cost")) {
                            features.historical_metrics.previous_cost =
                                historical["total_cost"];
                        }

                        if (historical.contains("total_time")) {
                            features.historical_metrics.previous_time =
                                historical["total_time"];
                        }

                        if (historical.contains("resource_utilization")) {
                            features.historical_metrics.previous_resource_utilization =
                                historical["resource_utilization"];
                        }
                    }
                }
            }
        }
        catch (const json::exception& e) {
            std::cerr << "解析二次规划特征时出错: " << e.what() << std::endl;
        }
    }

    // 4. 计算复杂度特征（简化实现）
    features.problem_complexity_score = std::log10(features.num_targets * features.num_resources + 1);
    features.constraint_tightness = features.target_time_urgency;  // 用时间紧迫度近似

    return features;
}

// ==================== AlgorithmEvaluator 实现 ====================

json AlgorithmEvaluator::EvaluationResult::to_json() const {
    return {
        {"is_feasible", is_feasible},
        {"total_score", total_score},
        {"coverage_score", coverage_score},
        {"cost_score", cost_score},
        {"time_score", time_score},
        {"resource_score", resource_score},
        {"feasibility_score", feasibility_score},
        {"computation_time", computation_time},
        {"detailed_metrics", detailed_metrics}
    };
}
// 在 HyperparameterOptimizer.cpp 中添加


ScenarioCategory classify_scenario(int target_count, int resource_count, bool is_secondary) {
    if (is_secondary) {
        return CATEGORY_D_REPLANNING;
    }

    if (target_count <= 20 && resource_count <= 250) {
        return CATEGORY_A_SMALL_EXACT;
    }
    else if (target_count <= 40 && resource_count <= 400) {
        return CATEGORY_B_MEDIUM_MULTIOBJECTIVE;
    }
    else {
        return CATEGORY_C_LARGE_HEURISTIC;
    }
}

std::string get_category_name(ScenarioCategory category) {
    switch (category) {
    case CATEGORY_A_SMALL_EXACT:
        return "A_Small_Exact";
    case CATEGORY_B_MEDIUM_MULTIOBJECTIVE:
        return "B_Medium_MultiObjective";
    case CATEGORY_C_LARGE_HEURISTIC:
        return "C_Large_Heuristic";
    case CATEGORY_D_REPLANNING:
        return "D_Replanning";
    default:
        return "Unknown_Category";
    }
}
AlgorithmEvaluator::EvaluationResult
AlgorithmEvaluator::evaluate_algorithm(const std::string& algorithm_name,
    const json& parameters,
    Schedule* schedule,
    int timeout_seconds,
    float decision_preferance) {

    // 每次评估前重置静态计数器
    Mount::initCountId();
    Load::initCountId();
    Target::initCountId();
    Base::initCountId();
    TargetType::initCountId();
    Tour::initCountId();
    MainTarget::initCountId();

    EvaluationResult result;
    auto start_time = std::chrono::steady_clock::now();
    //清空之前的数据

    try {
        std::vector<Tour*> result_tours;
        float minDifference;
        if (algorithm_name == "NSGAII" && (30<=schedule->getTargetList().size() <= 50) && (400<=schedule->getFleetList().size() <= 600)) {
            NSGAII nsgaii(schedule);
            nsgaii.run();
            auto results = nsgaii.getResult();
            if (!results.empty()) {
                if (decision_preferance == 0) {
                    auto it = results.find("minTime");
                    result_tours = it->second;
                    map<string, map<string, float>> objectives = nsgaii.getObjectives();
                    auto it2 = objectives.find("minDifference");
                    if (it2 != objectives.end()) {
                        minDifference = it2->second["minDifference"];
                    }
                }
                else if (decision_preferance == 1) {
                    auto it = results.find("minCost");
                    result_tours = it->second;
                    map<string, map<string, float>> objectives = nsgaii.getObjectives();
                    auto it2 = objectives.find("minDifference");
                    if (it2 != objectives.end()) {
                        minDifference = it2->second["minDifference"];
                    }
                }
                else if (decision_preferance == 2) {
                    auto it = results.find("minDifference");
                    result_tours = it->second;
                    map<string, map<string, float>> objectives = nsgaii.getObjectives();
                    auto it2 = objectives.find("minDifference");
                    if (it2 != objectives.end()) {
                        minDifference = it2->second["minDifference"];
                    }
                }
                //默认成本最小
                else{
                    result_tours= results.begin()->second;
                }
                    
            }
        }
        else if (algorithm_name == "Greedy_Time" && (schedule->getTargetList().size() > 50) && (schedule->getFleetList().size() > 600) && decision_preferance !=2 ) {
            Greedy* greedy = new Greedy(schedule);
            auto results = greedy->getResult();
            auto it = results.find("timeShortest");
            if (it != results.end()) {
                result_tours = it->second;
            }
            map<string, map<string, float>> objectives = greedy->getObjectives();
            auto it2 = objectives.find("minDifference");
            if (it2 != objectives.end()) {
                minDifference = it2->second["minDifference"];
            }
        }
        else if (algorithm_name == "Greedy_Cost" && (schedule->getTargetList().size() > 50) && (schedule->getFleetList().size() > 600) && decision_preferance != 2 ) {
            Greedy* greedy = new Greedy(schedule);
            auto results = greedy->getResult();
            auto it = results.find("costLowest");
            if (it != results.end()) {
                result_tours = it->second;
            }
            map<string, map<string, float>> objectives = greedy->getObjectives();
            auto it2 = objectives.find("minDifference");
            if (it2 != objectives.end()) {
                minDifference = it2->second["minDifference"];
            }
        }
        else if (algorithm_name == "Greedy_Difference"&& decision_preferance==2) {
            Greedy* greedy = new Greedy(schedule);
            auto results = greedy->getResult();
            auto it = results.find("minDifference");
            if (it != results.end()) {
                result_tours = it->second;
            }
            map<string, map<string, float>> objectives = greedy->getObjectives();
            auto it2 = objectives.find("minDifference");
            if (it2 != objectives.end()) {
                minDifference = it2->second["minDifference"];
            }
            delete greedy;
        }
        //精确算法只考虑一定规模下的
        else if (algorithm_name == "Exact_Time" && (schedule->getTargetList().size() <= 30 )&&(schedule->getFleetList().size() <= 400)) {
            Util::weightTourTime = 1000;
            schedule->generateSchemes();
            Model model(schedule);
            model.solve();
            result_tours = model.getSolnTourList();
            minDifference = 100.0;//给一个很大的值，因为精确算法不用于重规划 
        }
        else if (algorithm_name == "Exact_Cost" && (schedule->getTargetList().size() <= 30) && (schedule->getFleetList().size() <= 400)) {
            Util::weightTourTime = 100;
            //schedule->generateSchemes();
            Model model(schedule);
            model.solve();
            result_tours = model.getSolnTourList();
            minDifference = 100.0;//给一个很大的值，因为精确算法不用于重规划
        }
        // 计算性能分数
        if (!result_tours.empty()) {
            result.is_feasible = true;
            auto end_time = std::chrono::steady_clock::now();//计算耗时
            Base* base = schedule->getBase();

            // 更新资源使用情况和目标时间
            for (auto tour : result_tours) {
                base->pushUsedFleetMap(tour->getFleet(), 1);

                for (auto operTarget : tour->getOperTargetList()) {
                    Target* target = operTarget->getTarget();
                    target->pushResourse(tour, operTarget->getMountLoadPlan());

                    for (auto& mlPair : operTarget->getMountLoadPlan()) {
                        base->pushUsedMountMap(mlPair.first->_mount, mlPair.second);
                        base->pushUsedLoadMap(mlPair.first->_load, mlPair.second);
                    }
                }

                tour->computeTimeMap();
                auto timeMap = tour->getTargetTimeMap();
                for (auto it = timeMap.begin(); it != timeMap.end(); ++it) {
                    Target* target = it->first;
                    double completeTime = it->second;
                    target->pushAsgTime(tour->getFleet(), completeTime);

                    double earliestCmpTime = target->getEarliestCmpTime();
                    if (earliestCmpTime == 0) {
                        target->setEarliestCmpTime(completeTime);
                    }
                    else if (completeTime > earliestCmpTime) {
                        target->setEarliestCmpTime(completeTime);
                    }
                }
            }

            // 计算各项指标
            vector<Target*> coverTargetList = schedule->getCoverTList();
            vector<Target*> allTargetList = schedule->getTargetList();

            // 1. 计算总成本
            double totalTourCost = 0;
            for (auto tour : result_tours) {
                totalTourCost += tour->getCost();
            }
            result.cost_score = 1.0 / (1.0 + totalTourCost / 500.0);

            // 2. 计算最大完成时间
            double maxCmpTime = 0;
            for (auto target : coverTargetList) {
                auto sortedTime = target->getSortedByValue();
                if (!sortedTime.empty() && sortedTime.back().second > maxCmpTime) {
                    maxCmpTime = sortedTime.back().second;
                }
            }
            result.time_score = 1.0 / (0.7 + maxCmpTime);

            // 3. 目标覆盖率
            double targetCoverRate = (double)coverTargetList.size() / (double)allTargetList.size();
            result.coverage_score = targetCoverRate*1.2;

            // 4. 资源使用率,越小越好，节省资源
            auto fleetList = schedule->getFleetList();
            double resourceUseRate = (double)result_tours.size() / (double)fleetList.size();
            result.resource_score = 1.0 / (0.1 + resourceUseRate);

            //5.算法计算耗时
            result.computation_time = std::chrono::duration<double>(end_time - start_time).count();

            if (result.computation_time > timeout_seconds) {
                result.is_feasible = false;
            }
            //6. 计算差异度,越小越好
            result.difference_score = 1.0 / (1.0 + minDifference);

            // 计算总分
            //根据决策偏好确定权重，如果没有提供权重，则使用默认权重
            json default_weights = {
                {"coverage", 0.3},
                {"cost", 0.2},
                {"time", 0.2},
                {"resources", 0.2},
                {"difference", 0.1}
            };
            json weights;
            if (decision_preferance == 0) { // 时间最短优先
                weights = {
                    {"coverage", 0.4},
                    {"cost", 0.15},
                    {"time", 0.35},
                    {"resources", 0.1},
                    {"difference", 0.0}
                };
            }
            else if (decision_preferance == 1) { // 成本最小优先
                weights = {
                    {"coverage", 0.4},
                    {"cost", 0.4},
                    {"time", 0.1},
                    {"resources", 0.1},
                    {"difference", 0.0}
                };
            }
            else if (decision_preferance == 2) { // 差异最小优先
                weights = {
                    {"coverage", 0.3},
                    {"cost", 0.1},
                    {"time", 0.2},
                    {"resources", 0.05},
                    {"difference", 0.35}
                };
            }
            else {
                weights = default_weights;
            }
            result.total_score = calculate_final_score(result, weights);
            
            //如果耗时超过最大阈值，则总得分置为0
            if (result.computation_time > timeout_seconds) {
                result.total_score = 0;
            }
            // 存储详细原始数据
            result.detailed_metrics = {
                {"raw_metrics", {
                    {"total_cost", totalTourCost},
                    {"max_completion_time", maxCmpTime},
                    {"covered_targets", coverTargetList.size()},
                    {"total_targets", allTargetList.size()},
                    {"tour_count", result_tours.size()},
                    {"resource_utilization_rate", resourceUseRate},
                    {"difference", minDifference},
                }},
                {"normalized_scores", {
                    {"coverage", result.coverage_score},
                    {"cost", result.cost_score},
                    {"time", result.time_score},
                    {"resources", result.resource_score},
                    {"difference", result.difference_score}

                }}
            };
        }
        else {
            result.is_feasible = false;
            result.cost_score = 0;
            result.time_score = 0;
            result.coverage_score = 0;
            result.resource_score = 0;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "评估算法时出错: " << e.what() << std::endl;
        result.is_feasible = false;
    }



    return result;
}

double AlgorithmEvaluator::calculate_final_score(const EvaluationResult& result,
    const json& weights) {

    double total = 0.0;

    total += result.coverage_score * weights["coverage"].get<double>();
    total += result.cost_score * weights["cost"].get<double>();
    total += result.time_score * weights["time"].get<double>();
    total += result.resource_score * weights["resources"].get<double>();
    total += result.difference_score * weights["difference"].get<double>();

    return total;
}
// 新增：保存对比结果的函数
void AlgorithmEvaluator::save_comparison_results(const json& comparison_stats,
    const std::string& output_dir,
    const std::string& scenario_name) {

    // 确保输出目录存在
    fs::create_directories(output_dir);

    // 保存单场景详细结果
    std::string scenario_file = output_dir + "/" + scenario_name + "_comparison.json";
    std::ofstream file(scenario_file);
    if (file.is_open()) {
        // 计算并添加统计指标
        auto stats_with_analysis = calculate_scenario_statistics(comparison_stats);
        file << stats_with_analysis.dump(4);
        file.close();
        std::cout << "场景对比结果已保存到: " << scenario_file << std::endl;
    }

    // 追加到总体结果文件
    std::string overall_file = output_dir + "/overall_comparison.json";
    json overall_results;

    if (fs::exists(overall_file)) {
        std::ifstream in_file(overall_file);
        if (in_file.is_open()) {
            in_file >> overall_results;
            in_file.close();
        }
    }

    if (!overall_results.contains("scenarios")) {
        overall_results["scenarios"] = json::array();
    }

    overall_results["scenarios"].push_back(comparison_stats);

    std::ofstream out_file(overall_file);
    if (out_file.is_open()) {
        out_file << overall_results.dump(4);
        out_file.close();
    }
}

std::string AlgorithmEvaluator::generate_algorithm_label(Schedule* schedule,
    const std::vector<std::string>& algorithms,
    const json& parameters,
    int timeout_seconds,
    float decision_preferance,
    const std::string& output_dir,
    const std::string& ScenarioName) {

    if (algorithms.empty()) {
        return "Exact_Time";
    }

    std::string best_algorithm = algorithms[0];
    double best_score = -std::numeric_limits<double>::infinity();

    std::vector<std::pair<std::string, EvaluationResult>> results;
    std::cout<<"决策偏好："<<decision_preferance<<std::endl;
    // 新增：保存所有算法结果的容器
    json all_results_json = json::array();
    json comparison_stats = {
        {"scenario_name",ScenarioName}, 
        {"total_targets", schedule->getTargetList().size()},
        {"total_resources", schedule->getFleetList().size()},
        {"decision_preference", decision_preferance},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
        {"algorithm_results", json::array()},
        {"statistics", json::object()}
    };
    for (const auto& algorithm_name : algorithms) {
        try {
            std::cout<<"       "<<std::endl;
            std::cout << ">>>>>>>>>>>>>>>>>>>评估算法: " << algorithm_name << std::endl;

            json algo_params = parameters;
            if (parameters.contains(algorithm_name)) {
                algo_params = parameters[algorithm_name];
            }

            auto result = evaluate_algorithm(algorithm_name, algo_params, schedule, timeout_seconds, decision_preferance);

            results.emplace_back(algorithm_name, result);

            std::cout << " >>>>>>>>>>>>>>>>>>>分数: " << result.total_score << std::endl;
            
            if (result.total_score > best_score) {
                best_score = result.total_score;
                best_algorithm = algorithm_name;
            }
            // 新增：收集每个算法的详细结果
            json algo_result_json = {
                {"algorithm_name", algorithm_name},
                {"evaluation_result", result.to_json()},
                {"ranking_score", result.total_score}
            };
            all_results_json.push_back(algo_result_json);

            comparison_stats["algorithm_results"].push_back(algo_result_json);
        }

        catch (const std::exception& e) {
            std::cerr << "评估算法 " << algorithm_name << " 时出错: " << e.what() << std::endl;
            continue;
        }
    }
    // 新增：保存结果到文件
    if (!output_dir.empty()) {
        save_comparison_results(comparison_stats, output_dir, ScenarioName);
    }
    std::cout << "最佳算法: " << best_algorithm << " (分数: " << best_score << ")" << std::endl;
    //输出所有算法的细节
    for (const auto& result : results) {
        cout << "算法名称: " << result.first << endl;
        cout << "是否可行: " << result.second.is_feasible << endl;
        cout << "总耗时: " << result.second.computation_time << endl;
        cout << "总得分: " << result.second.total_score << endl;
        cout << "覆盖率得分: " << result.second.coverage_score << endl;
        cout << "成本得分: " << result.second.cost_score << endl;
        cout << "时间得分: " << result.second.time_score << endl;
        cout << "资源得分: " << result.second.resource_score << endl;
        cout << "差异得分: " << result.second.difference_score << endl;
        cout << "详细原始数据: " << result.second.detailed_metrics << endl;
    }
    return best_algorithm;
}

// 新增：计算单场景统计指标的函数
json AlgorithmEvaluator::calculate_scenario_statistics(const json& comparison_stats) {
    json result = comparison_stats;

    if (!comparison_stats.contains("algorithm_results") ||comparison_stats["algorithm_results"].empty()) {
        return result;
    }

    const auto& algo_results = comparison_stats["algorithm_results"];

    // 找出各项指标的最佳值
    double best_cost = std::numeric_limits<double>::max();
    double best_time = std::numeric_limits<double>::max();
    double best_coverage = 0.0;
    double best_score = 0.0;
    double fastest_time = std::numeric_limits<double>::max();

    std::vector<std::string> cost_optimal_algorithms;
    std::vector<std::string> time_optimal_algorithms;
    std::vector<std::string> coverage_optimal_algorithms;
    std::vector<std::string> fast_algorithms;  // 1秒内
    std::vector<std::string> acceptable_algorithms;  // 5秒内且gap<5%

    // 第一遍：找出最佳值
    for (const auto& algo_result : algo_results) {
        const auto& eval_result = algo_result["evaluation_result"];

        if (eval_result["is_feasible"]) {
            double cost = eval_result["detailed_metrics"]["raw_metrics"]["total_cost"];
            double time = eval_result["detailed_metrics"]["raw_metrics"]["max_completion_time"];
            double coverage = eval_result["coverage_score"];
            double comp_time = eval_result["computation_time"];

            if (cost < best_cost) best_cost = cost;
            if (time < best_time) best_time = time;
            if (coverage > best_coverage) best_coverage = coverage;
            if (comp_time < fastest_time) fastest_time = comp_time;
        }
    }

    // 第二遍：标记最优算法
    for (const auto& algo_result : algo_results) {
        std::string algo_name = algo_result["algorithm_name"];
        const auto& eval_result = algo_result["evaluation_result"];

        if (eval_result["is_feasible"]) {
            double cost = eval_result["detailed_metrics"]["raw_metrics"]["total_cost"];
            double time = eval_result["detailed_metrics"]["raw_metrics"]["max_completion_time"];
            double coverage = eval_result["coverage_score"];
            double total_score = eval_result["total_score"];
            double comp_time = eval_result["computation_time"];

            const double OPTIMALITY_GAP = 0.05;  // 5%

            // 成本最优
            if (std::abs(cost - best_cost) / (best_cost + 1e-10) <= OPTIMALITY_GAP) {
                cost_optimal_algorithms.push_back(algo_name);
            }

            // 时间最优
            if (std::abs(time - best_time) / (best_time + 1e-10) <= OPTIMALITY_GAP) {
                time_optimal_algorithms.push_back(algo_name);
            }

            // 覆盖率最优
            if (std::abs(coverage - best_coverage) <= OPTIMALITY_GAP) {
                coverage_optimal_algorithms.push_back(algo_name);
            }

            // 快速解（1秒内）
            if (comp_time <= 1.0) {
                fast_algorithms.push_back(algo_name);
            }

            // 可接受解（5秒内且gap<5%）
            if (comp_time <= 5.0) {
                // 找到该场景中的最佳分数
                double best_score_in_scene = 0.0;
                for (const auto& other_result : algo_results) {
                    if (other_result["evaluation_result"]["is_feasible"]) {
                        double other_score = other_result["evaluation_result"]["total_score"];
                        if (other_score > best_score_in_scene) {
                            best_score_in_scene= other_score;
                        }
                    }
                }

                if (std::abs(total_score - best_score_in_scene) / (best_score_in_scene + 1e-10) <= OPTIMALITY_GAP) {
                    acceptable_algorithms.push_back(algo_name);
                }
            }
        }
    }

    // 添加统计指标到结果
    result["statistics"] = {
        {"optimal_algorithms", {
            {"cost_optimal", cost_optimal_algorithms},
            {"time_optimal", time_optimal_algorithms},
            {"coverage_optimal", coverage_optimal_algorithms}
        }},
        {"performance_algorithms", {
            {"fast_solutions_1s", fast_algorithms},
            {"acceptable_solutions_5s_gap5", acceptable_algorithms}
        }},
        {"counts", {
            {"total_algorithms", algo_results.size()},
            {"feasible_algorithms",std::count_if(algo_results.begin(), algo_results.end(),
                    [](const json& j) { return j["evaluation_result"]["is_feasible"]; })},
            {"cost_optimal_count", cost_optimal_algorithms.size()},
            {"time_optimal_count", time_optimal_algorithms.size()},
            {"coverage_optimal_count", coverage_optimal_algorithms.size()},
            {"fast_solution_count", fast_algorithms.size()},
            {"acceptable_solution_count", acceptable_algorithms.size()}
        }}
    };

    return result;
}

// 新增：分析所有场景的总体统计
json AlgorithmEvaluator::analyze_overall_statistics(const std::string& results_dir) {
    json overall_stats = {
        {"total_scenarios", 0},
        {"scenarios_with_data", 0},
        {"categories", {
            {"A_Small_Exact", json::object()},
            {"B_Medium_MultiObjective", json::object()},
            {"C_Large_Heuristic", json::object()},
            {"D_Replanning", json::object()}
        }}
    };
    // 按类别统计
    std::map<ScenarioCategory, std::map<std::string, json>> category_algo_stats;
    std::map<ScenarioCategory, int> category_scenario_counts;

    // 初始化所有类别的统计
    for (int cat = CATEGORY_A_SMALL_EXACT; cat <= CATEGORY_D_REPLANNING; ++cat) {
        category_scenario_counts[static_cast<ScenarioCategory>(cat)] = 0;
    }

    // 收集所有结果文件
    std::vector<fs::path> result_files;
    for (const auto& entry : fs::directory_iterator(results_dir)) {
        if (entry.path().extension() == ".json" &&
            entry.path().filename().string().find("_comparison.json") != std::string::npos) {
            result_files.push_back(entry.path());
        }
    }

    overall_stats["total_scenarios"] = result_files.size();

    // 处理每个结果文件
    for (const auto& result_file : result_files) {
        try {
            std::ifstream file(result_file);
            json scenario_result;
            file >> scenario_result;
            file.close();

            if (!scenario_result.contains("algorithm_results") ||
                !scenario_result.contains("scenario_name") ||
                !scenario_result.contains("total_targets") ||
                !scenario_result.contains("total_resources")) {
                continue;
            }

            overall_stats["scenarios_with_data"] =
                overall_stats["scenarios_with_data"].get<int>() + 1;

            // 判断场景类别
            int target_count = scenario_result["total_targets"];
            int resource_count = scenario_result["total_resources"];
            bool is_secondary = scenario_result.contains("is_secondary") ?
                scenario_result["is_secondary"].get<bool>() : false;

            ScenarioCategory category = classify_scenario(target_count, resource_count, is_secondary);
            category_scenario_counts[category]++;

            // 获取该场景的最佳算法
            std::string best_algorithm = "";
            double best_score = -std::numeric_limits<double>::infinity();

            const auto& algo_results = scenario_result["algorithm_results"];
            for (const auto& algo_result : algo_results) {
                if (algo_result["evaluation_result"]["is_feasible"]) {
                    double score = algo_result["evaluation_result"]["total_score"];
                    if (score > best_score) {
                        best_score = score;
                        best_algorithm = algo_result["algorithm_name"];
                    }
                }
            }

            // 处理每个算法的结果
            for (const auto& algo_result : algo_results) {
                std::string algo_name = algo_result["algorithm_name"];
                const auto& eval_result = algo_result["evaluation_result"];

                // 初始化算法统计（如果不存在）
                if (category_algo_stats[category].find(algo_name) == category_algo_stats[category].end()) {
                    category_algo_stats[category][algo_name] = {
                        {"total_runs", 0},
                        {"feasible_runs", 0},
                        {"best_algorithm_count", 0},
                        {"total_cost_sum", 0.0},
                        {"total_time_sum", 0.0},
                        {"total_coverage_sum", 0.0},
                        {"total_resource_util_sum", 0.0},
                        {"total_difference_sum", 0.0},
                        {"total_computation_time_sum", 0.0},
                        {"min_cost", std::numeric_limits<double>::max()},
                        {"min_time", std::numeric_limits<double>::max()},
                        {"max_coverage", 0.0}
                    };
                }

                auto& stats = category_algo_stats[category][algo_name];
                stats["total_runs"] = stats["total_runs"].get<int>() + 1;

                if (eval_result["is_feasible"]) {
                    stats["feasible_runs"] = stats["feasible_runs"].get<int>() + 1;

                    // 检查是否是最佳算法
                    if (algo_name == best_algorithm) {
                        stats["best_algorithm_count"] = stats["best_algorithm_count"].get<int>() + 1;
                    }

                    // 获取原始数据
                    if (eval_result["detailed_metrics"].contains("raw_metrics")) {
                        const auto& raw = eval_result["detailed_metrics"]["raw_metrics"];

                        double cost = raw["total_cost"];
                        double time = raw["max_completion_time"];
                        double coverage = raw["covered_targets"] * 1.0 / raw["total_targets"];
                        double resource_util = raw["resource_utilization_rate"];
                        double difference = raw.value("difference", 0.0);
                        double comp_time = eval_result["computation_time"];

                        // 累加求和
                        stats["total_cost_sum"] = stats["total_cost_sum"].get<double>() + cost;
                        stats["total_time_sum"] = stats["total_time_sum"].get<double>() + time;
                        stats["total_coverage_sum"] = stats["total_coverage_sum"].get<double>() + coverage;
                        stats["total_resource_util_sum"] = stats["total_resource_util_sum"].get<double>() + resource_util;
                        stats["total_difference_sum"] = stats["total_difference_sum"].get<double>() + difference;
                        stats["total_computation_time_sum"] = stats["total_computation_time_sum"].get<double>() + comp_time;

                        // 更新最优值
                        if (cost < stats["min_cost"].get<double>()) {
                            stats["min_cost"] = cost;
                        }
                        if (time < stats["min_time"].get<double>()) {
                            stats["min_time"] = time;
                        }
                        if (coverage > stats["max_coverage"].get<double>()) {
                            stats["max_coverage"] = coverage;
                        }
                    }
                }
            }

        }
        catch (const std::exception& e) {
            std::cerr << "分析文件 " << result_file << " 时出错: " << e.what() << std::endl;
        }
    }

    // 计算各类别的统计结果
    for (int cat = CATEGORY_A_SMALL_EXACT; cat <= CATEGORY_D_REPLANNING; ++cat) {
        ScenarioCategory category = static_cast<ScenarioCategory>(cat);
        std::string category_name = get_category_name(category);
        int category_count = category_scenario_counts[category];

        json category_stats;

        if (category_count > 0) {
            // 计算每个算法的平均指标
            for (const auto& [algo_name, algo_stat] : category_algo_stats[category]) {
                int feasible_runs = algo_stat["feasible_runs"].get<int>();

                if (feasible_runs > 0) {
                    category_stats[algo_name] = {
                        {"scenario_count", category_count},
                        {"feasible_count", feasible_runs},
                        {"success_rate", static_cast<double>(feasible_runs) / algo_stat["total_runs"].get<double>() * 100.0},
                        {"best_algorithm_ratio", static_cast<double>(algo_stat["best_algorithm_count"].get<int>()) / category_count * 100.0},
                        {"avg_min_cost", algo_stat["total_cost_sum"].get<double>() / feasible_runs},
                        {"avg_min_time", algo_stat["total_time_sum"].get<double>() / feasible_runs},
                        {"avg_coverage", algo_stat["total_coverage_sum"].get<double>() / feasible_runs * 100.0},
                        {"avg_resource_utilization", algo_stat["total_resource_util_sum"].get<double>() / feasible_runs * 100.0},
                        {"avg_difference", category == CATEGORY_D_REPLANNING ?
                            (algo_stat["total_difference_sum"].get<double>() / feasible_runs) : 0.0},
                        {"avg_computation_time", algo_stat["total_computation_time_sum"].get<double>() / feasible_runs},
                        {"historical_best_cost", algo_stat["min_cost"]},
                        {"historical_best_time", algo_stat["min_time"]},
                        {"historical_best_coverage", algo_stat["max_coverage"].get<double>() * 100.0}
                    };
                }
            }
        }

        overall_stats["categories"][category_name] = {
            {"scenario_count", category_count},
            {"algorithm_stats", category_stats}
        };
    }

    // 保存总体统计
    std::string stats_file = results_dir + "/overall_statistics_by_category.json";
    std::ofstream out_file(stats_file);
    if (out_file.is_open()) {
        out_file << overall_stats.dump(4);
        out_file.close();
        std::cout << "按类别分类的总体统计已保存到: " << stats_file << std::endl;
    }

    // 生成CSV格式的汇总报告
    generate_category_csv_summary(category_algo_stats, category_scenario_counts, results_dir);

    return overall_stats;
}
// 新增：生成CSV汇总报告
void AlgorithmEvaluator::generate_category_csv_summary(
    const std::map<ScenarioCategory, std::map<std::string, json>>& category_algo_stats,
    const std::map<ScenarioCategory, int>& category_scenario_counts,
    const std::string& results_dir) {

    std::string csv_file = results_dir + "/algorithm_comparison_by_category.csv";
    std::ofstream csv(csv_file);

    if (csv.is_open()) {
        // 表头
        csv << "场景类别,算法名称,场景数,可行解数,成功率(%),最优算法比例(%),"
            << "平均最小成本,平均最小时间,平均覆盖率(%),平均资源使用率(%),"
            << "平均差异度,平均计算时间(s),历史最优成本,历史最优时间,历史最高覆盖率(%)\n";

        std::vector<std::string> algorithms = {
            "Greedy_Time", "Greedy_Cost", "Greedy_Difference",
            "NSGAII", "Exact_Time", "Exact_Cost"
        };

        // 按类别顺序输出
        for (int cat = CATEGORY_A_SMALL_EXACT; cat <= CATEGORY_D_REPLANNING; ++cat) {
            ScenarioCategory category = static_cast<ScenarioCategory>(cat);
            std::string category_name = get_category_name(category);
            int category_count = category_scenario_counts.at(category);

            // 输出每个算法在该类别的数据
            for (const auto& algo_name : algorithms) {
                // 检查该算法在该类别是否有数据
                const auto& category_stats = category_algo_stats.at(category);
                auto it = category_stats.find(algo_name);

                if (it != category_stats.end() && category_count > 0) {
                    const auto& stats = it->second;
                    int feasible_runs = stats["feasible_runs"].get<int>();

                    if (feasible_runs > 0) {
                        double success_rate = static_cast<double>(feasible_runs) /
                            stats["total_runs"].get<double>() * 100.0;
                        double best_ratio = static_cast<double>(stats["best_algorithm_count"].get<int>()) /
                            category_count * 100.0;

                        csv << category_name << ","
                            << algo_name << ","
                            << category_count << ","
                            << feasible_runs << ","
                            << std::fixed << std::setprecision(1) << success_rate << ","
                            << std::fixed << std::setprecision(1) << best_ratio << ","
                            << std::fixed << std::setprecision(2) << (stats["total_cost_sum"].get<double>() / feasible_runs) << ","
                            << std::fixed << std::setprecision(2) << (stats["total_time_sum"].get<double>() / feasible_runs) << ","
                            << std::fixed << std::setprecision(1) << (stats["total_coverage_sum"].get<double>() / feasible_runs * 100.0) << ","
                            << std::fixed << std::setprecision(1) << (stats["total_resource_util_sum"].get<double>() / feasible_runs * 100.0) << ","
                            << std::fixed << std::setprecision(3) << (category == CATEGORY_D_REPLANNING ?
                                (stats["total_difference_sum"].get<double>() / feasible_runs) : 0.0) << ","
                            << std::fixed << std::setprecision(3) << (stats["total_computation_time_sum"].get<double>() / feasible_runs) << ","
                            << std::fixed << std::setprecision(2) << stats["min_cost"].get<double>() << ","
                            << std::fixed << std::setprecision(2) << stats["min_time"].get<double>() << ","
                            << std::fixed << std::setprecision(1) << (stats["max_coverage"].get<double>() * 100.0) << "\n";
                    }
                    else {
                        // 没有可行解的情况
                        csv << category_name << ","
                            << algo_name << ","
                            << category_count << ","
                            << "0,0.0,0.0,"
                            << "N/A,N/A,N/A,N/A,"
                            << (category == CATEGORY_D_REPLANNING ? "N/A" : "N/A") << ","
                            << "N/A,N/A,N/A,N/A\n";
                    }
                }
                else {
                    // 该算法在该类别没有数据
                    csv << category_name << ","
                        << algo_name << ","
                        << category_count << ","
                        << "0,0.0,0.0,"
                        << "N/A,N/A,N/A,N/A,"
                        << (category == CATEGORY_D_REPLANNING ? "N/A" : "N/A") << ","
                        << "N/A,N/A,N/A,N/A\n";
                }
            }

            // 添加空行分隔不同类别
            csv << "\n";
        }

        csv.close();
        std::cout << "按类别分类的CSV汇总报告已保存到: " << csv_file << std::endl;

        // 生成汇总表（每个类别的最佳算法）
        generate_category_summary_table(category_algo_stats, category_scenario_counts, results_dir);
    }
}
void AlgorithmEvaluator::generate_category_summary_table(
    const std::map<ScenarioCategory, std::map<std::string, json>>& category_algo_stats,
    const std::map<ScenarioCategory, int>& category_scenario_counts,
    const std::string& results_dir) {

    std::string summary_file = results_dir + "/category_summary_table.csv";
    std::ofstream summary(summary_file);

    if (summary.is_open()) {
        // 表头：横向显示算法，纵向显示指标
        summary << "指标\\算法";
        std::vector<std::string> algorithms = {
            "Greedy_Time", "Greedy_Cost", "Greedy_Difference",
            "NSGAII", "Exact_Time", "Exact_Cost"
        };

        for (const auto& algo : algorithms) {
            summary << "," << algo;
        }
        summary << "\n";

        // 对每个类别生成一行
        for (int cat = CATEGORY_A_SMALL_EXACT; cat <= CATEGORY_D_REPLANNING; ++cat) {
            ScenarioCategory category = static_cast<ScenarioCategory>(cat);
            std::string category_name = get_category_name(category);

            // 场景数量行
            summary << category_name << "_场景数";
            for (const auto& algo : algorithms) {
                summary << "," << category_scenario_counts.at(category);
            }
            summary << "\n";

            // 最优算法比例行
            summary << category_name << "_最优算法比例(%)";
            for (const auto& algo : algorithms) {
                const auto& category_stats = category_algo_stats.at(category);
                auto it = category_stats.find(algo);

                if (it != category_stats.end() && category_scenario_counts.at(category) > 0) {
                    const auto& stats = it->second;
                    double best_ratio = static_cast<double>(stats["best_algorithm_count"].get<int>()) /
                        category_scenario_counts.at(category) * 100.0;
                    summary << "," << std::fixed << std::setprecision(1) << best_ratio;
                }
                else {
                    summary << ",0.0";
                }
            }
            summary << "\n";

            // 平均计算时间行
            summary << category_name << "_平均计算时间(s)";
            for (const auto& algo : algorithms) {
                const auto& category_stats = category_algo_stats.at(category);
                auto it = category_stats.find(algo);

                if (it != category_stats.end()) {
                    const auto& stats = it->second;
                    int feasible_runs = stats["feasible_runs"].get<int>();
                    if (feasible_runs > 0) {
                        double avg_time = stats["total_computation_time_sum"].get<double>() / feasible_runs;
                        summary << "," << std::fixed << std::setprecision(3) << avg_time;
                    }
                    else {
                        summary << ",N/A";
                    }
                }
                else {
                    summary << ",N/A";
                }
            }
            summary << "\n";

            // 成功率行
            summary << category_name << "_成功率(%)";
            for (const auto& algo : algorithms) {
                const auto& category_stats = category_algo_stats.at(category);
                auto it = category_stats.find(algo);

                if (it != category_stats.end()) {
                    const auto& stats = it->second;
                    double success_rate = static_cast<double>(stats["feasible_runs"].get<int>()) /
                        stats["total_runs"].get<double>() * 100.0;
                    summary << "," << std::fixed << std::setprecision(1) << success_rate;
                }
                else {
                    summary << ",0.0";
                }
            }
            summary << "\n";

            // 添加空行分隔不同类别
            summary << "\n";
        }

        summary.close();
        std::cout << "类别汇总表已保存到: " << summary_file << std::endl;
    }
}
// ==================== AlgorithmRecommender 实现 ====================

AlgorithmRecommender::AlgorithmRecommender() : model_trained_(false) {
    // 初始化决策树
    decision_tree_ = cv::ml::DTrees::create();
    // 1. 设置类别权重 - 这是关键！
    // 方法2：手动设置的经验权重（更直观）
    // 如果你更喜欢手动控制，使用这个：
     //cv::Mat priors = (cv::Mat_<float>(1, 5) <<
     //    2.5f,    // Exact_Time: 从1.0提到2.5，恢复性能
     //    //2.0f,    // Exact_Cost: 保持
     //    2.5f,    // NSGAII: 从1.0提到2.5，恢复性能
     //    1.5f,    // Greedy_Time: 保持
     //    0.8f,    // Greedy_Cost: 从0.3提到0.8，适度恢复
     //    5.0f     // Greedy_Difference: 从12.0降到5.0，仍然较高
     //);

    //decision_tree_->setPriors(priors);
    // 配置决策树参数
    // 2. 调整决策树参数防止过拟合到多数类
    decision_tree_->setMaxDepth(3);           // 降低深度，防止过拟合到Greedy_Cost
    decision_tree_->setMinSampleCount(3);     // 降低最小样本数，让少数类也能分裂

    // 3. 增加正则化参数
    decision_tree_->setRegressionAccuracy(0.1f);  // 回归精度

    decision_tree_->setUseSurrogates(false);
    // 4. 剪枝设置
    decision_tree_->setCVFolds(1);// 必须设置为 1，因为 OpenCV 不支持交叉验证剪枝
    decision_tree_->setUse1SERule(true);          // 使用1个标准误差规则剪枝
    decision_tree_->setTruncatePrunedTree(true);   // 截断剪枝的树
   
    // 5. 分裂质量评估标准
    decision_tree_->setMaxCategories(5);          // 增加最大类别数



    // 算法标签
    algorithm_labels_ = { "Exact_Time","Exact_Cost", "NSGAII", "Greedy_Time", "Greedy_Cost","Greedy_Difference"};

    // 特征名称
    feature_names_ = {
        // 基础特征 (7)
        "num_targets", "num_resources", "num_target_types", "num_resource_types",
         "resource_target_ratio", "target_time_urgency","urgency_level"

        // 统计特征 (8)
        "target_dist_mean", "target_dist_std", "resource_cap_mean", "resource_cap_std",
        "time_window_mean", "time_window_std", "cost_mean", "cost_std",

        // 拓扑和复杂度特征 (4)
        "spatial_clustering", "target_density",
        "problem_complexity", "constraint_tightness", 

        // 二次规划特征 (3)
        "is_secondary", "target_change_ratio", "original_target_count",

        // 历史性能特征 (4)
        "prev_coverage", "prev_cost", "prev_time", "prev_resource_util",

        // 分位数特征 (5个位置，可能少于5个)
        "dist_percentile_25", "dist_percentile_50", "dist_percentile_75",
        "time_percentile_25", "time_percentile_50"
    };
}

double AlgorithmRecommender::calculate_training_accuracy(const cv::Mat& features, const cv::Mat& labels) {
    if (!model_trained_ || features.rows == 0) return 0.0;

    int correct = 0;
    for (int i = 0; i < features.rows; ++i) {
        cv::Mat prediction;
        float predicted_value = decision_tree_->predict(features.row(i), prediction);
        int predicted_label = static_cast<int>(predicted_value);

        if (predicted_label == labels.at<int>(i)) {
            correct++;
        }
    }
    return static_cast<double>(correct) / features.rows;
}

std::vector<double> AlgorithmRecommender::get_feature_importance() const {
    std::vector<double> importance(feature_names_.size(), 0.0);

    // 简化实现：OpenCV的DTrees没有直接提供特征重要性
    // 这里返回均匀分布，实际项目中应该实现更准确的计算
    double uniform_value = 1.0 / feature_names_.size();
    for (size_t i = 0; i < importance.size(); ++i) {
        importance[i] = uniform_value;
    }

    return importance;
}

void AlgorithmRecommender::save_decision_rules(const std::string& filename) const {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "无法打开文件保存决策规则: " << filename << std::endl;
        return;
    }

    file << "算法推荐决策规则\n";
    file << "==================\n\n";

    file << "特征说明:\n";
    for (size_t i = 0; i < feature_names_.size(); ++i) {
        file << "  [" << i << "] " << feature_names_[i] << "\n";
    }

    file << "\n算法映射:\n";
    for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
        file << "  " << i << ": " << algorithm_labels_[i] << "\n";
    }

    file << "\n决策树结构:\n";
    file << "  - 最大深度: " << decision_tree_->getMaxDepth() << "\n";
    file << "  - 最小样本数: " << decision_tree_->getMinSampleCount() << "\n";
    file << "  - 类别数: " << decision_tree_->getMaxCategories() << "\n";

    file << "\n基于训练数据的经验规则:\n";
    file << "1. 当目标数量 <= 25 且 资源数量 <= 300 时，推荐 Exact 算法\n";
    file << "2. 当时间窗口紧度 > 0.7 时，推荐 NSGAII 算法\n";
    file << "3. 当目标数量 > 40 时，推荐 Greedy_Time 算法\n";
    file << "4. 当资源成本方差 > 0.8 时，推荐 Greedy_Cost 算法\n";
    file << "5. 其他情况根据决策树模型预测\n";

    file.close();
    std::cout << "决策规则已保存到: " << filename << std::endl;
}

bool AlgorithmRecommender::train_algorithm_recommender(const std::string& training_data_path,
    const std::map<std::string, float>& class_weights) {
    std::string data_file_path;

    // 检查是否有现成的训练数据
    if (fs::exists(training_data_path)) {
        data_file_path = training_data_path + "merged_training_data.json";
    }
    else {
        std::cerr << "无法找到训练数据或场景数据" << std::endl;
        return false;
    }

    // 加载训练数据
    std::ifstream data_file(data_file_path);
    if (!data_file.is_open()) {
        std::cerr << "无法打开训练数据文件: " << data_file_path << std::endl;
        return false;
    }

    json training_data;
    try {
        data_file >> training_data;
    }
    catch (const json::parse_error& e) {
        std::cerr << "解析训练数据失败: " << e.what() << std::endl;
        return false;
    }
    data_file.close();

    if (training_data.empty()) {
        std::cerr << "训练数据为空" << std::endl;
        return false;
    }

    std::cout << "加载到 " << training_data.size() << " 个训练样本" << std::endl;

    // ==================== 处理类别权重 ====================
    std::map<std::string, float> weights_to_use = class_weights;

    // 如果有传入权重，过滤掉权重为0的类别
    if (!weights_to_use.empty()) {
        std::cout << "处理传入的类别权重..." << std::endl;

        // 过滤权重为0的类别
        std::map<std::string, float> filtered_weights;
        std::vector<std::string> filtered_labels;
        std::map<std::string, int> label_remapping; // 旧标签 -> 新标签索引

        for (const auto& [algo, weight] : weights_to_use) {
            if (weight > 0) {
                filtered_weights[algo] = weight;
                filtered_labels.push_back(algo);
                label_remapping[algo] = filtered_labels.size() - 1;
                std::cout << "  保留类别: " << algo << " (权重: " << weight << ")" << std::endl;
            }
            else {
                std::cout << "  排除类别: " << algo << " (权重为0)" << std::endl;
            }
        }

        if (filtered_weights.empty()) {
            std::cerr << "所有类别的权重都为0，无法训练模型" << std::endl;
            return false;
        }

        // 更新算法标签列表
        algorithm_labels_ = filtered_labels;
        weights_to_use = filtered_weights;

        std::cout << "过滤后剩余 " << algorithm_labels_.size() << " 个类别" << std::endl;

        // 创建先验权重矩阵
        cv::Mat priors(1, algorithm_labels_.size(), CV_32F);
        for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
            const std::string& algo = algorithm_labels_[i];
            priors.at<float>(i) = weights_to_use[algo];
        }

        // 设置决策树的类别先验权重
        decision_tree_->setPriors(priors);

        std::cout << "已设置决策树类别权重:" << std::endl;
        for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
            std::cout << "  " << algorithm_labels_[i] << ": " << priors.at<float>(i) << std::endl;
        }

        // 在解析训练数据时，需要使用新的标签映射
        // 我们将在下面的循环中使用 label_remapping
    }
    else {
        std::cout << "未传入类别权重，决策树将使用默认权重设置" << std::endl;
    }

    std::vector<cv::Mat> features;
    std::vector<int> labels;

    // 解析训练数据
    for (const auto& sample : training_data) {
        try {
            json feature_json = sample["features"];
            ScenarioFeatureExtractor::ScenarioFeatures feature;

            // 1. 提取基础特征
            feature.scenario_name = feature_json["scenario_name"];
            feature.num_targets = feature_json["num_targets"];
            feature.num_resources = feature_json["num_resources"];
            feature.num_target_types = feature_json["num_target_types"];
            feature.num_resource_types = feature_json["num_resource_types"];
            feature.decision_preferance = feature_json.value("decision_preference", 0.0);
            feature.target_time_urgency = feature_json.value("urgency_level", feature_json.value("target_time_urgency", 0.7));
            feature.resource_target_ratio = feature_json.value("resource_target_ratio", 1.0);

            // 2. 提取统计特征
            if (feature_json.contains("statistical_features")) {
                const auto& stats = feature_json["statistical_features"];

                // 目标距离统计
                if (stats.contains("target_distance")) {
                    const auto& dist_stats = stats["target_distance"];
                    feature.target_distance_stats.mean = dist_stats.value("mean", 0.0f);
                    feature.target_distance_stats.std_dev = dist_stats.value("std_dev", 0.0f);
                    feature.target_distance_stats.min = dist_stats.value("min", 0.0f);
                    feature.target_distance_stats.max = dist_stats.value("max", 0.0f);
                    if (dist_stats.contains("percentiles") && dist_stats["percentiles"].is_array()) {
                        for (const auto& p : dist_stats["percentiles"]) {
                            feature.target_distance_stats.percentiles.push_back(p);
                        }
                    }
                }

                // 资源容量统计
                if (stats.contains("resource_capacity")) {
                    const auto& cap_stats = stats["resource_capacity"];
                    feature.resource_capacity_stats.mean = cap_stats.value("mean", 0.0f);
                    feature.resource_capacity_stats.std_dev = cap_stats.value("std_dev", 0.0f);
                }

                // 时间窗口统计
                if (stats.contains("time_window")) {
                    const auto& time_stats = stats["time_window"];
                    feature.time_window_stats.mean = time_stats.value("mean", 60.0f);
                    feature.time_window_stats.std_dev = time_stats.value("std_dev", 0.0f);
                    if (time_stats.contains("percentiles") && time_stats["percentiles"].is_array()) {
                        for (const auto& p : time_stats["percentiles"]) {
                            feature.time_window_stats.percentiles.push_back(p);
                        }
                    }
                }

                // 成本分布统计
                if (stats.contains("cost_distribution")) {
                    const auto& cost_stats = stats["cost_distribution"];
                    feature.cost_distribution_stats.mean = cost_stats.value("mean", 0.0f);
                    feature.cost_distribution_stats.std_dev = cost_stats.value("std_dev", 0.0f);
                }
            }

            // 3. 提取拓扑特征
            if (feature_json.contains("topological_features")) {
                const auto& topo = feature_json["topological_features"];
                feature.spatial_clustering_coefficient = topo.value("spatial_clustering", 0.0f);
                feature.target_density = topo.value("target_density", 0.0f);
            }

            // 4. 提取复杂度特征
            if (feature_json.contains("complexity_features")) {
                const auto& complexity = feature_json["complexity_features"];
                feature.problem_complexity_score = complexity.value("problem_complexity", 0.0f);
                feature.constraint_tightness = complexity.value("constraint_tightness", 0.0f);
            }

            // 5. 提取二次规划特征
            if (feature_json.contains("secondary_planning_features")) {
                const auto& secondary = feature_json["secondary_planning_features"];
                feature.is_secondary_planning = secondary.value("is_secondary_planning", false);
                feature.target_change_ratio = secondary.value("target_change_ratio", 0.0f);
                feature.original_target_count = secondary.value("original_target_count", 0);
            }
            else {
                // 兼容旧数据
                feature.is_secondary_planning = feature_json.value("is_secondary_planning", false);
            }

            // 6. 提取历史性能特征
            if (feature_json.contains("historical_metrics")) {
                const auto& historical = feature_json["historical_metrics"];
                feature.historical_metrics.previous_coverage = historical.value("previous_coverage", 0.0f);
                feature.historical_metrics.previous_cost = historical.value("previous_cost", 0.0f);
                feature.historical_metrics.previous_time = historical.value("previous_time", 0.0f);
                feature.historical_metrics.previous_resource_utilization = historical.value("previous_resource_utilization", 0.0f);
            }

            // 7. 设置默认值
            if (feature.problem_complexity_score == 0.0f && feature.num_targets > 0 && feature.num_resources > 0) {
                feature.problem_complexity_score = std::log10(feature.num_targets * feature.num_resources + 1) / 3.0f;
            }

            if (feature.constraint_tightness == 0.0f) {
                feature.constraint_tightness = feature.target_time_urgency;
            }

            // 8. 转换为OpenCV矩阵
            features.push_back(feature.to_mat());

            // 9. 提取标签
            std::string algo_label = sample["best_algorithm"];

            // 根据是否使用权重来确定标签索引
            int label = -1;

            if (class_weights.empty()) {
                // 没有使用权重，使用原始映射
                label = algo_label_to_int(algo_label);
            }
            else {
                // 使用了权重，检查该算法是否在过滤后的列表中
                if (std::find(algorithm_labels_.begin(), algorithm_labels_.end(), algo_label) != algorithm_labels_.end()) {
                    // 在过滤后的列表中找到该算法，使用新索引
                    for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
                        if (algorithm_labels_[i] == algo_label) {
                            label = i;
                            break;
                        }
                    }
                }
                else {
                    // 该算法不在过滤后的列表中，跳过这个样本
                    std::cout << "跳过样本 " << feature.scenario_name
                        << "，算法 " << algo_label << " 不在训练类别中" << std::endl;
                    features.pop_back(); // 移除刚刚添加的特征
                    continue;
                }
            }

            if (label >= 0) {
                labels.push_back(label);
            }
            else {
                // 标签无效，跳过这个样本
                features.pop_back(); // 移除刚刚添加的特征
                std::cout << "跳过样本 " << feature.scenario_name
                    << "，无法映射算法标签: " << algo_label << std::endl;
                continue;
            }

            // 输出调试信息
            if (features.size() <= 5) {
                std::cout << "解析样本 " << feature.scenario_name
                    << ": 特征维度=" << feature.to_mat().cols
                    << ", 二次规划=" << (feature.is_secondary_planning ? "是" : "否")
                    << ", 目标数=" << feature.num_targets
                    << ", 算法=" << algo_label << ", 标签=" << label
                    << std::endl;
            }
        }
        catch (const json::exception& e) {
            std::cerr << "解析训练样本失败: " << e.what() << std::endl;
            continue;
        }
        catch (const std::exception& e) {
            std::cerr << "处理训练样本失败: " << e.what() << std::endl;
            continue;
        }
    }

    // 检查特征维度是否一致
    if (!features.empty()) {
        int expected_dim = features[0].cols;
        for (size_t i = 1; i < features.size(); ++i) {
            if (features[i].cols != expected_dim) {
                std::cerr << "警告: 样本 " << i << " 的特征维度不一致: "
                    << features[i].cols << " vs " << expected_dim << std::endl;
            }
        }
        std::cout << "训练数据特征维度: " << expected_dim << std::endl;
    }

    // ==================== 训练模型 ====================
    if (features.empty() || labels.empty()) {
        std::cerr << "没有有效的训练数据" << std::endl;
        return false;
    }

    if (features.size() != labels.size()) {
        std::cerr << "特征和标签数量不匹配" << std::endl;
        return false;
    }

    std::cout << "开始训练决策树模型，使用 " << features.size() << " 个样本..." << std::endl;
    std::cout << "训练类别数量: " << algorithm_labels_.size() << std::endl;

    // 准备训练数据矩阵
    cv::Mat train_data(features.size(), features[0].cols, CV_32F);
    cv::Mat label_data(labels.size(), 1, CV_32S);

    for (size_t i = 0; i < features.size(); ++i) {
        features[i].copyTo(train_data.row(i));
        label_data.at<int>(i) = labels[i];
    }

    // 创建训练数据对象
    cv::Ptr<cv::ml::TrainData> train_data_ptr = cv::ml::TrainData::create(
        train_data, cv::ml::ROW_SAMPLE, label_data);

    // 训练决策树
    try {
        bool success = decision_tree_->train(train_data_ptr);
        if (!success) {
            std::cerr << "决策树训练失败（返回false）" << std::endl;
            return false;
        }

        model_trained_ = true;
        std::cout << "决策树训练完成" << std::endl;

        // 计算训练准确率
        double train_accuracy = calculate_training_accuracy(train_data, label_data);
        std::cout << "训练集准确率: " << std::fixed << std::setprecision(2)
            << train_accuracy * 100 << "%" << std::endl;

        // 保存模型
        if (!save_model(training_data_path)) {
            std::cerr << "保存模型失败" << std::endl;
            return false;
        }

        // 保存决策规则
        save_decision_rules(training_data_path + "decision_rules.txt");

        return true;

    }
    catch (const cv::Exception& e) {
        std::cerr << "训练决策树异常: " << e.what() << std::endl;
        return false;
    }
}

bool AlgorithmRecommender::save_model(const std::string& base_path) const {
    if (!model_trained_) {
        std::cerr << "模型未训练，无法保存" << std::endl;
        return false;
    }

    std::string model_path = base_path + "algorithm_recommender.yml";
    std::string info_path = base_path + "algorithm_recommender_info.json";

    try {
        // 保存OpenCV模型
        decision_tree_->save(model_path);
        std::cout << "模型已保存到: " << model_path << std::endl;

        // 保存模型信息
        json model_info = {
            {"model_type", "DecisionTree"},
            {"algorithm_labels", algorithm_labels_},
            {"feature_names", feature_names_},
            {"training_time", std::chrono::system_clock::now().time_since_epoch().count()},
            {"model_parameters", {
                {"max_depth", decision_tree_->getMaxDepth()},
                {"min_sample_count", decision_tree_->getMinSampleCount()},
                {"max_categories", decision_tree_->getMaxCategories()},
                {"cv_folds", 1},
                {"use_1se_rule", true},
                {"truncate_pruned_tree", true}
            }}
        };

        std::ofstream info_file(info_path);
        if (!info_file.is_open()) {
            std::cerr << "无法打开模型信息文件: " << info_path << std::endl;
            return false;
        }

        info_file << model_info.dump(4);
        info_file.close();

        std::cout << "模型信息已保存到: " << info_path << std::endl;

        // 同时复制到models目录
        fs::path models_dir = fs::path(base_path).parent_path().parent_path() / "models";
        fs::create_directories(models_dir);

        std::string deploy_model_path = models_dir.string() + "/algorithm_recommender.yml";
        std::string deploy_info_path = models_dir.string() + "/algorithm_recommender_info.json";

        fs::copy_file(model_path, deploy_model_path, fs::copy_options::overwrite_existing);
        fs::copy_file(info_path, deploy_info_path, fs::copy_options::overwrite_existing);

        std::cout << "模型已部署到: " << deploy_model_path << std::endl;

        return true;

    }
    catch (const cv::Exception& e) {
        std::cerr << "保存模型失败: " << e.what() << std::endl;
        return false;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "复制模型文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool AlgorithmRecommender::load_pretrained_model(const std::string& model_path) {
    try {
        decision_tree_ = cv::ml::DTrees::load(model_path);
        if (decision_tree_.empty()) {
            std::cerr << "无法加载模型: " << model_path << std::endl;
            return false;
        }

        model_trained_ = true;
        std::cout << "算法推荐器模型已加载: " << model_path << std::endl;

        // 尝试加载模型信息
        std::string info_path = model_path + ".info.json";
        if (!fs::exists(info_path)) {
            info_path = fs::path(model_path).parent_path().string() + "/algorithm_recommender_info.json";
        }

        if (fs::exists(info_path)) {
            std::ifstream info_file(info_path);
            json model_info;
            info_file >> model_info;

            if (model_info.contains("algorithm_labels")) {
                algorithm_labels_ = model_info["algorithm_labels"].get<std::vector<std::string>>();
                std::cout << "加载算法标签: ";
                for (const auto& label : algorithm_labels_) {
                    std::cout << label << " ";
                }
                std::cout << std::endl;
            }

            if (model_info.contains("feature_names")) {
                feature_names_ = model_info["feature_names"].get<std::vector<std::string>>();
            }
        }

        return true;
    }
    catch (const cv::Exception& e) {
        std::cerr << "加载模型失败: " << e.what() << std::endl;
        return false;
    }
}


std::string AlgorithmRecommender::recommend_algorithm(const ScenarioFeatureExtractor::ScenarioFeatures& features) {
    if (!model_trained_) {
        std::cerr << "模型未训练，使用默认推荐" << std::endl;
        return default_recommendation(features);
    }

    try {
        cv::Mat feature_mat = features.to_mat();
        cv::Mat prediction;
        float result = decision_tree_->predict(feature_mat, prediction);

        int label = static_cast<int>(result);
        if (label >= 0 && label < static_cast<int>(algorithm_labels_.size())) {
            std::cout << "模型推荐算法: " << algorithm_labels_[label] << std::endl;
            return algorithm_labels_[label];
        }
        else {
            std::cerr << "预测标签超出范围: " << label << std::endl;
            return default_recommendation(features);
        }
    }
    catch (const cv::Exception& e) {
        std::cerr << "预测失败: " << e.what() << std::endl;
        return default_recommendation(features);
    }
}


std::map<std::string, double> AlgorithmRecommender::predict_algorithm_performance(
    const ScenarioFeatureExtractor::ScenarioFeatures& features) {

    std::map<std::string, double> performance;

    if (!model_trained_) {
        for (const auto& algo : algorithm_labels_) {
            performance[algo] = 0.5;
        }
        return performance;
    }

    std::string best_algo = recommend_algorithm(features);
    for (const auto& algo : algorithm_labels_) {
        performance[algo] = (algo == best_algo) ? 0.9 : 0.3;
    }

    return performance;
}

//int AlgorithmRecommender::algo_label_to_int(const std::string& algo_label) const {
//    if (algo_label == "Exact_Time") return 0;
//    if (algo_label == "Exact_Cost") return 1;
//    if (algo_label == "NSGAII") return 2;
//    if (algo_label == "Greedy_Time") return 3;
//    if (algo_label == "Greedy_Cost") return 4;
//    if (algo_label == "Greedy_Difference") return 5;
//    return -1;
//}
int AlgorithmRecommender::algo_label_to_int(const std::string& algo_label) const {
    for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
        if (algorithm_labels_[i] == algo_label) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string AlgorithmRecommender::default_recommendation(const ScenarioFeatureExtractor::ScenarioFeatures& features) const {
    if (features.num_targets <= 25 && features.num_resources <= 300) {
        return "Exact_Time";
    }
    else if (features.num_targets >= 40) {
        return "Greedy_Time";
    }
    else if (features.target_time_urgency > 0.7) {
        return "Greedy_Cost";
    }
    else {
        return "NSGAII";
    }
}

// ==================== TestResult 的 to_json 方法实现 ====================
json AlgorithmRecommender::TestResult::to_json() const {
    return {
        {"accuracy", accuracy},
        {"correct_by_algorithm", correct_by_algorithm},
        {"total_by_algorithm", total_by_algorithm},
        {"confusion_matrix", confusion_matrix},
        {"detailed_results", detailed_results}
    };
}

// ==================== evaluate_model 方法实现 ====================
AlgorithmRecommender::TestResult AlgorithmRecommender::evaluate_model(
    const std::string& primary_test_dir,
    const std::string& secondary_test_dir) {

    TestResult result;

    if (!model_trained_) {
        std::cerr << "模型未训练，无法评估" << std::endl;
        return result;
    }

    // 检查模型是否已加载或训练
    if (decision_tree_.empty()) {
        std::cerr << "决策树模型为空，请先训练或加载模型" << std::endl;
        return result;
    }

    // 加载一次规划测试标签
    std::string primary_test_file = primary_test_dir + "test_labels.json";
    std::string secondary_test_file = secondary_test_dir + "secondary_test_labels.json";

    // 检查至少有一个标签文件存在
    bool has_primary = fs::exists(primary_test_file);
    bool has_secondary = fs::exists(secondary_test_file);

    if (!has_primary && !has_secondary) {
        std::cerr << "未找到任何测试标签文件" << std::endl;
        std::cerr << "检查路径: " << primary_test_file << std::endl;
        std::cerr << "检查路径: " << secondary_test_file << std::endl;
        return result;
    }

    json all_test_data = json::array();  // 合并所有测试数据

    // 加载一次规划测试数据
    if (has_primary) {
        std::cout << "加载一次规划测试标签: " << primary_test_file << std::endl;
        std::ifstream file(primary_test_file);
        try {
            json primary_data;
            file >> primary_data;

            // 为一次规划数据标记类型
            for (auto& sample : primary_data) {
                sample["planning_type"] = "primary";
                // 确保is_secondary_planning标志正确
                if (sample.contains("features")) {
                    if (sample["features"].contains("secondary_planning_features")) {
                        sample["features"]["secondary_planning_features"]["is_secondary_planning"] = false;
                    }
                    else {
                        sample["features"]["is_secondary_planning"] = false;
                    }
                }
            }

            all_test_data.insert(all_test_data.end(),
                primary_data.begin(),
                primary_data.end());
            file.close();
            std::cout << "加载一次规划测试样本: " << primary_data.size() << " 个" << std::endl;
        }
        catch (const json::exception& e) {
            std::cerr << "解析一次规划测试数据失败: " << e.what() << std::endl;
            file.close();
        }
    }

    // 加载二次规划测试数据
    if (has_secondary) {
        std::cout << "加载二次规划测试标签: " << secondary_test_file << std::endl;
        std::ifstream file(secondary_test_file);
        try {
            json secondary_data;
            file >> secondary_data;

            // 为二次规划数据标记类型
            for (auto& sample : secondary_data) {
                sample["planning_type"] = "secondary";
                // 确保is_secondary_planning标志正确
                if (sample.contains("features")) {
                    if (sample["features"].contains("secondary_planning_features")) {
                        sample["features"]["secondary_planning_features"]["is_secondary_planning"] = true;
                    }
                    else {
                        sample["features"]["is_secondary_planning"] = true;
                    }
                }
            }

            all_test_data.insert(all_test_data.end(),
                secondary_data.begin(),
                secondary_data.end());
            file.close();
            std::cout << "加载二次规划测试样本: " << secondary_data.size() << " 个" << std::endl;
        }
        catch (const json::exception& e) {
            std::cerr << "解析二次规划测试数据失败: " << e.what() << std::endl;
            file.close();
        }
    }

    if (all_test_data.empty()) {
        std::cerr << "合并后的测试数据为空" << std::endl;
        return result;
    }

    // 统计不同类型样本数量
    size_t primary_count = 0, secondary_count = 0;
    for (const auto& sample : all_test_data) {
        if (sample["planning_type"] == "primary") {
            primary_count++;
        }
        else if (sample["planning_type"] == "secondary") {
            secondary_count++;
        }
    }

    std::cout << "\n开始评估模型，使用 " << all_test_data.size() << " 个测试样本" << std::endl;
    std::cout << "（一次规划: " << primary_count << " 个，二次规划: " << secondary_count << " 个）" << std::endl;

    // 初始化统计
    int correct = 0;
    int primary_correct = 0;
    int secondary_correct = 0;
    int primary_total = 0;
    int secondary_total = 0;

    std::map<std::string, int> algorithm_correct;
    std::map<std::string, int> algorithm_total;
    std::map<std::string, int> primary_by_algorithm;
    std::map<std::string, int> secondary_by_algorithm;

    // 初始化混淆矩阵
    std::vector<std::vector<int>> confusion(algorithm_labels_.size(),
        std::vector<int>(algorithm_labels_.size(), 0));

    json detailed = json::array();

    // 处理每个测试样本
    for (const auto& sample : all_test_data) {
        try {
            std::string scenario_name = sample["scenario_name"];
            std::string true_label = sample["true_label"];
            std::string planning_type = sample["planning_type"];

            // 统计按规划类型
            if (planning_type == "primary") {
                primary_total++;
            }
            else if (planning_type == "secondary") {
                secondary_total++;
            }

            // 从样本中提取特征 - 与训练时完全一致
            json feature_json = sample["features"];
            ScenarioFeatureExtractor::ScenarioFeatures features;

            // 1. 提取基础特征（与训练时完全一致）
            features.scenario_name = feature_json["scenario_name"];
            features.num_targets = feature_json["num_targets"];
            features.num_resources = feature_json["num_resources"];
            features.num_target_types = feature_json["num_target_types"];
            features.num_resource_types = feature_json["num_resource_types"];
            features.decision_preferance = feature_json.value("decision_preferance",
                feature_json.value("decision_preference", 0.0)); // 兼容两种字段名
            features.target_time_urgency = feature_json.value("target_time_urgency",
                feature_json.value("urgency_level", 0.7));
            features.resource_target_ratio = feature_json.value("resource_target_ratio",
                static_cast<float>(features.num_resources) / std::max(1, features.num_targets));

            // 2. 提取统计特征（与训练时完全一致）
            if (feature_json.contains("statistical_features")) {
                const auto& stats = feature_json["statistical_features"];

                // 目标距离统计
                if (stats.contains("target_distance")) {
                    const auto& dist_stats = stats["target_distance"];
                    features.target_distance_stats.mean = dist_stats.value("mean", 0.0f);
                    features.target_distance_stats.std_dev = dist_stats.value("std_dev", 0.0f);
                    features.target_distance_stats.min = dist_stats.value("min", 0.0f);
                    features.target_distance_stats.max = dist_stats.value("max", 0.0f);

                    if (dist_stats.contains("percentiles") && dist_stats["percentiles"].is_array()) {
                        features.target_distance_stats.percentiles.clear();
                        for (const auto& p : dist_stats["percentiles"]) {
                            features.target_distance_stats.percentiles.push_back(p);
                        }
                    }
                }

                // 资源容量统计
                if (stats.contains("resource_capacity")) {
                    const auto& cap_stats = stats["resource_capacity"];
                    features.resource_capacity_stats.mean = cap_stats.value("mean", 0.0f);
                    features.resource_capacity_stats.std_dev = cap_stats.value("std_dev", 0.0f);
                }

                // 时间窗口统计
                if (stats.contains("time_window")) {
                    const auto& time_stats = stats["time_window"];
                    features.time_window_stats.mean = time_stats.value("mean", 60.0f);
                    features.time_window_stats.std_dev = time_stats.value("std_dev", 0.0f);

                    if (time_stats.contains("percentiles") && time_stats["percentiles"].is_array()) {
                        features.time_window_stats.percentiles.clear();
                        for (const auto& p : time_stats["percentiles"]) {
                            features.time_window_stats.percentiles.push_back(p);
                        }
                    }
                }

                // 成本分布统计
                if (stats.contains("cost_distribution")) {
                    const auto& cost_stats = stats["cost_distribution"];
                    features.cost_distribution_stats.mean = cost_stats.value("mean", 0.0f);
                    features.cost_distribution_stats.std_dev = cost_stats.value("std_dev", 0.0f);
                }
            }

            // 3. 提取拓扑特征（与训练时完全一致）
            if (feature_json.contains("topological_features")) {
                const auto& topo = feature_json["topological_features"];
                features.spatial_clustering_coefficient = topo.value("spatial_clustering", 0.0f);
                features.target_density = topo.value("target_density", 0.0f);
            }

            // 4. 提取复杂度特征（与训练时完全一致）
            if (feature_json.contains("complexity_features")) {
                const auto& complexity = feature_json["complexity_features"];
                features.problem_complexity_score = complexity.value("problem_complexity", 0.0f);
                features.constraint_tightness = complexity.value("constraint_tightness", 0.0f);
            }
            else {
                // 如果没有复杂度特征，使用默认计算
                if (features.problem_complexity_score == 0.0f && features.num_targets > 0 && features.num_resources > 0) {
                    features.problem_complexity_score = std::log10(features.num_targets * features.num_resources + 1) / 3.0f;
                }
                if (features.constraint_tightness == 0.0f) {
                    features.constraint_tightness = features.target_time_urgency;
                }
            }

            // 5. 提取二次规划特征（与训练时完全一致）
            if (feature_json.contains("secondary_planning_features")) {
                const auto& secondary = feature_json["secondary_planning_features"];
                features.is_secondary_planning = secondary.value("is_secondary_planning", false);
                features.target_change_ratio = secondary.value("target_change_ratio", 0.0f);
                features.original_target_count = secondary.value("original_target_count", 0);
            }
            else {
                // 兼容旧数据
                features.is_secondary_planning = feature_json.value("is_secondary_planning", false);
            }

            // 6. 提取历史性能特征（与训练时完全一致）
            if (feature_json.contains("historical_metrics")) {
                const auto& historical = feature_json["historical_metrics"];
                features.historical_metrics.previous_coverage = historical.value("previous_coverage", 0.0f);
                features.historical_metrics.previous_cost = historical.value("previous_cost", 0.0f);
                features.historical_metrics.previous_time = historical.value("previous_time", 0.0f);
                features.historical_metrics.previous_resource_utilization = historical.value("previous_resource_utilization", 0.0f);
            }

            // 使用模型预测
            std::string predicted_label = recommend_algorithm(features);

            // 统计
            algorithm_total[true_label]++;

            // 按规划类型统计
            if (planning_type == "primary") {
                primary_by_algorithm[true_label]++;
            }
            else if (planning_type == "secondary") {
                secondary_by_algorithm[true_label]++;
            }

            if (predicted_label == true_label) {
                correct++;
                algorithm_correct[true_label]++;

                // 按规划类型统计正确率
                if (planning_type == "primary") {
                    primary_correct++;
                }
                else if (planning_type == "secondary") {
                    secondary_correct++;
                }
            }

            // 更新混淆矩阵
            int true_idx = algo_label_to_int(true_label);
            int pred_idx = algo_label_to_int(predicted_label);
            if (true_idx >= 0 && true_idx < algorithm_labels_.size() &&
                pred_idx >= 0 && pred_idx < algorithm_labels_.size()) {
                confusion[true_idx][pred_idx]++;
            }

            // 保存详细结果
            detailed.push_back({
                {"scenario_name", scenario_name},
                {"planning_type", planning_type},
                {"decision_preference", features.decision_preferance},
                {"true_label", true_label},
                {"predicted_label", predicted_label},
                {"is_correct", (predicted_label == true_label)}
                });

        }
        catch (const std::exception& e) {
            std::cerr << "评估样本失败: " << e.what() << std::endl;
            continue;
        }
    }

    // 计算结果
    if (!all_test_data.empty()) {
        result.accuracy = static_cast<double>(correct) / all_test_data.size();

        // 按规划类型的准确率
        result.primary_accuracy = (primary_total > 0) ?
            static_cast<double>(primary_correct) / primary_total : 0.0;
        result.secondary_accuracy = (secondary_total > 0) ?
            static_cast<double>(secondary_correct) / secondary_total : 0.0;

        result.correct_by_algorithm = algorithm_correct;
        result.total_by_algorithm = algorithm_total;
        result.confusion_matrix = confusion;
        result.detailed_results = detailed;

        // 额外信息
        result.primary_total = primary_total;
        result.secondary_total = secondary_total;
        result.primary_by_algorithm = primary_by_algorithm;
        result.secondary_by_algorithm = secondary_by_algorithm;

        // 输出评估结果
        print_test_results(result);

        // 保存评估结果到文件
        save_evaluation_results(result, primary_test_dir);
    }

    return result;
}

void AlgorithmRecommender::print_test_results(const TestResult& result) {
    std::cout << "\n=== 模型评估结果 ===" << std::endl;
    std::cout << "总体准确率: " << std::fixed << std::setprecision(2)
        << result.accuracy * 100 << "%" << std::endl;

    // 计算总样本数
    int total_samples = 0;
    int correct_samples = 0;
    for (const auto& [algo, total] : result.total_by_algorithm) {
        total_samples += total;
        auto correct_it = result.correct_by_algorithm.find(algo);
        if (correct_it != result.correct_by_algorithm.end()) {
            correct_samples += correct_it->second;
        }
    }
    std::cout << "正确/总数: " << correct_samples << "/" << total_samples << std::endl;

    std::cout << "\n各算法准确率:" << std::endl;
    for (const auto& algo : algorithm_labels_) {
        auto total_it = result.total_by_algorithm.find(algo);
        if (total_it != result.total_by_algorithm.end()) {
            int total = total_it->second;
            auto correct_it = result.correct_by_algorithm.find(algo);
            int correct = (correct_it != result.correct_by_algorithm.end()) ? correct_it->second : 0;
            double algo_accuracy = (total > 0) ? static_cast<double>(correct) / total : 0.0;

            std::cout << "  " << std::setw(12) << std::left << algo
                << ": " << std::setw(3) << correct << "/" << std::setw(3) << total
                << " (" << std::fixed << std::setprecision(1)
                << algo_accuracy * 100 << "%)" << std::endl;
        }
        else {
            std::cout << "  " << std::setw(12) << std::left << algo
                << ": 没有测试样本" << std::endl;
        }
    }

    std::cout << "\n混淆矩阵 (行=真实标签，列=预测标签):" << std::endl;
    std::cout << std::setw(15) << " ";
    for (const auto& algo : algorithm_labels_) {
        std::cout << std::setw(12) << algo;
    }
    std::cout << std::endl;

    for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
        std::cout << std::setw(15) << algorithm_labels_[i];
        for (size_t j = 0; j < algorithm_labels_.size(); ++j) {
            std::cout << std::setw(12) << result.confusion_matrix[i][j];
        }
        std::cout << std::endl;
    }

    // 分析混淆矩阵
    std::cout << "\n混淆矩阵分析:" << std::endl;

    // 计算每个类别的精确率和召回率
    for (size_t i = 0; i < algorithm_labels_.size(); ++i) {
        std::string algo = algorithm_labels_[i];

        // 计算TP, FP, FN
        int TP = result.confusion_matrix[i][i];  // 真正例
        int FP = 0;  // 假正例
        int FN = 0;  // 假负例

        for (size_t j = 0; j < algorithm_labels_.size(); ++j) {
            if (j != i) {
                FP += result.confusion_matrix[j][i];  // 其他类别预测为该类别
                FN += result.confusion_matrix[i][j];  // 该类别预测为其他类别
            }
        }

        // 计算精确率和召回率
        double precision = (TP + FP > 0) ? static_cast<double>(TP) / (TP + FP) : 0.0;
        double recall = (TP + FN > 0) ? static_cast<double>(TP) / (TP + FN) : 0.0;
        double f1_score = (precision + recall > 0) ?
            2 * precision * recall / (precision + recall) : 0.0;

        std::cout << "  " << std::setw(12) << std::left << algo
            << ": 精确率=" << std::fixed << std::setprecision(2) << precision * 100 << "%"
            << ", 召回率=" << recall * 100 << "%"
            << ", F1分数=" << f1_score * 100 << "%" << std::endl;
    }
}

void AlgorithmRecommender::save_evaluation_results(const TestResult& result, const std::string& base_path) {
    std::string eval_file = base_path + "model_evaluation_results.json";

    // 计算总样本数
    int total_samples = 0;
    int correct_samples = 0;
    for (const auto& [algo, total] : result.total_by_algorithm) {
        total_samples += total;
        auto correct_it = result.correct_by_algorithm.find(algo);
        if (correct_it != result.correct_by_algorithm.end()) {
            correct_samples += correct_it->second;
        }
    }

    json evaluation = {
        {"summary", {
            {"accuracy", result.accuracy},
            {"total_samples", total_samples},
            {"correct_samples", correct_samples},
            {"evaluation_time", std::chrono::system_clock::now().time_since_epoch().count()}
        }},
        {"algorithm_performance", json::object()},
        {"confusion_matrix", result.confusion_matrix},
        {"confusion_matrix_labels", algorithm_labels_},
        {"detailed_results", result.detailed_results}
    };

    // 添加每个算法的性能指标
    for (const auto& algo : algorithm_labels_) {
        auto total_it = result.total_by_algorithm.find(algo);
        if (total_it != result.total_by_algorithm.end()) {
            int total = total_it->second;
            auto correct_it = result.correct_by_algorithm.find(algo);
            int correct = (correct_it != result.correct_by_algorithm.end()) ? correct_it->second : 0;
            double accuracy = (total > 0) ? static_cast<double>(correct) / total : 0.0;

            // 计算精确率、召回率、F1分数
            int true_idx = algo_label_to_int(algo);
            int TP = result.confusion_matrix[true_idx][true_idx];
            int FP = 0;
            int FN = 0;

            for (size_t j = 0; j < algorithm_labels_.size(); ++j) {
                if (j != true_idx) {
                    FP += result.confusion_matrix[j][true_idx];
                    FN += result.confusion_matrix[true_idx][j];
                }
            }

            double precision = (TP + FP > 0) ? static_cast<double>(TP) / (TP + FP) : 0.0;
            double recall = (TP + FN > 0) ? static_cast<double>(TP) / (TP + FN) : 0.0;
            double f1_score = (precision + recall > 0) ?
                2 * precision * recall / (precision + recall) : 0.0;

            evaluation["algorithm_performance"][algo] = {
                {"accuracy", accuracy},
                {"correct", correct},
                {"total", total},
                {"precision", precision},
                {"recall", recall},
                {"f1_score", f1_score}
            };
        }
    }

    std::ofstream file(eval_file);
    if (file.is_open()) {
        file << evaluation.dump(4);
        file.close();
        std::cout << "评估结果已保存到: " << eval_file << std::endl;
    }
}

// ==================== UCBBayesianOptimizer::Impl 实现 ====================
class UCBBayesianOptimizer::Impl {
public:
    Impl(double exploration_weight)
        : exploration_weight_(exploration_weight),
        rng_(std::chrono::system_clock::now().time_since_epoch().count()),
        best_performance_(-std::numeric_limits<double>::infinity()) {}

    json suggest_next_parameters(const json& parameter_space) {
        if (parameter_history_.empty()) {
            return random_sample(parameter_space);
        }
        return ucb_sample(parameter_space);
    }

    void update(const json& parameters, double performance) {
        parameter_history_.push_back(parameters);
        performance_history_.push_back(performance);

        if (performance > best_performance_) {
            best_parameters_ = parameters;
            best_performance_ = performance;
        }
    }

    json get_best_parameters() const {
        return best_parameters_;
    }

    double get_best_performance() const {
        return best_performance_;
    }

    void reset() {
        parameter_history_.clear();
        performance_history_.clear();
        acquisition_values_.clear();
        best_parameters_ = json();
        best_performance_ = -std::numeric_limits<double>::infinity();
    }

    json get_optimization_history() const {
        json history;
        for (size_t i = 0; i < parameter_history_.size(); ++i) {
            history.push_back({
                {"iteration", i + 1},
                {"parameters", parameter_history_[i]},
                {"performance", performance_history_[i]}
                });
        }
        return history;
    }

    size_t get_iteration_count() const {
        return parameter_history_.size();
    }

    void set_exploration_weight(double weight) {
        exploration_weight_ = weight;
    }

    double get_exploration_weight() const {
        return exploration_weight_;
    }

    void set_random_seed(unsigned int seed) {
        rng_.seed(seed);
    }

private:
    double exploration_weight_;
    std::default_random_engine rng_;
    std::vector<json> parameter_history_;
    std::vector<double> performance_history_;
    std::vector<double> acquisition_values_;
    json best_parameters_;
    double best_performance_;

    json random_sample(const json& param_space) {
        json sample;

        for (auto it = param_space.begin(); it != param_space.end(); ++it) {
            const std::string& param_name = it.key();
            const json& param_info = it.value();

            if (param_info.contains("type")) {
                std::string type = param_info["type"];

                if (type == "int") {
                    int min_val = param_info["min"];
                    int max_val = param_info["max"];
                    std::uniform_int_distribution<int> dist(min_val, max_val);
                    sample[param_name] = dist(rng_);
                }
                else if (type == "float") {
                    double min_val = param_info["min"];
                    double max_val = param_info["max"];
                    std::uniform_real_distribution<double> dist(min_val, max_val);
                    sample[param_name] = dist(rng_);
                }
                else if (type == "categorical") {
                    auto categories = param_info["values"];
                    std::uniform_int_distribution<int> dist(0, static_cast<int>(categories.size()) - 1);
                    sample[param_name] = categories[dist(rng_)];
                }
            }
        }

        return sample;
    }

    json ucb_sample(const json& param_space) {
        double best_score = -std::numeric_limits<double>::infinity();
        json best_sample;

        for (int i = 0; i < 20; ++i) {
            json sample = random_sample(param_space);
            double score = evaluate_ucb(sample);

            if (score > best_score) {
                best_score = score;
                best_sample = sample;
            }
        }

        return best_sample;
    }

    double evaluate_ucb(const json& sample) {
        double mean_performance = 0.0;
        int similar_count = 0;

        for (size_t i = 0; i < parameter_history_.size(); ++i) {
            if (is_similar(sample, parameter_history_[i])) {
                mean_performance += performance_history_[i];
                similar_count++;
            }
        }

        if (similar_count > 0) {
            mean_performance /= similar_count;
            double exploration_term = exploration_weight_ *
                std::sqrt(std::log(static_cast<double>(parameter_history_.size()) + 1.0) /
                    (similar_count + 1.0));
            return mean_performance + exploration_term;
        }
        else {
            return exploration_weight_ *
                std::sqrt(std::log(static_cast<double>(parameter_history_.size()) + 1.0));
        }
    }

    bool is_similar(const json& params1, const json& params2) {
        int match_count = 0;
        int total_count = 0;

        for (auto it = params1.begin(); it != params1.end(); ++it) {
            if (params2.contains(it.key())) {
                if (params2[it.key()] == it.value()) {
                    match_count++;
                }
                total_count++;
            }
        }

        return total_count > 0 &&
            static_cast<double>(match_count) / total_count > 0.7;
    }
};

// ==================== UCBBayesianOptimizer 公共方法实现 ====================

UCBBayesianOptimizer::UCBBayesianOptimizer(double exploration_weight)
    : impl_(std::make_unique<Impl>(exploration_weight)) {}

UCBBayesianOptimizer::~UCBBayesianOptimizer() = default;

json UCBBayesianOptimizer::suggest_next_parameters(const json & parameter_space) {
    return impl_->suggest_next_parameters(parameter_space);
}

void UCBBayesianOptimizer::update(const json & parameters, double performance) {
    impl_->update(parameters, performance);
}

json UCBBayesianOptimizer::get_best_parameters() const {
    return impl_->get_best_parameters();
}

double UCBBayesianOptimizer::get_best_performance() const {
    return impl_->get_best_performance();
}

void UCBBayesianOptimizer::reset() {
    impl_->reset();
}

json UCBBayesianOptimizer::get_optimization_history() const {
    return impl_->get_optimization_history();
}

size_t UCBBayesianOptimizer::get_iteration_count() const {
    return impl_->get_iteration_count();
}

void UCBBayesianOptimizer::set_exploration_weight(double weight) {
    impl_->set_exploration_weight(weight);
}

double UCBBayesianOptimizer::get_exploration_weight() const {
    return impl_->get_exploration_weight();
}

void UCBBayesianOptimizer::set_random_seed(unsigned int seed) {
    impl_->set_random_seed(seed);
}

// ==================== HyperparameterOptimizer 实现 ====================

json HyperparameterOptimizer::AlgorithmConfig::to_json() const {
    return {
        {"algorithm_name", algorithm_name},
        {"parameters", parameters},
        {"expected_performance", expected_performance},
        {"actual_performance", actual_performance},
        {"computation_time", computation_time}
    };
}

HyperparameterOptimizer::HyperparameterOptimizer() {
    bayesian_optimizer_ = std::make_unique<UCBBayesianOptimizer>(2.0);
}

HyperparameterOptimizer::~HyperparameterOptimizer() = default;

HyperparameterOptimizer::AlgorithmConfig
HyperparameterOptimizer::optimize_algorithm(const std::string & algorithm_name,
    Schedule * schedule,
    int max_iterations,
    int timeout_seconds) {

    std::cout << "\n开始优化算法: " << algorithm_name << std::endl;

    json parameter_space = get_parameter_space(algorithm_name);
    bayesian_optimizer_->reset();

    AlgorithmConfig best_config;
    best_config.algorithm_name = algorithm_name;
    best_config.expected_performance = -std::numeric_limits<double>::infinity();

    auto optimization_start = std::chrono::steady_clock::now();

    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration<double>(current_time - optimization_start).count();
        if (elapsed_time > timeout_seconds) {
            std::cout << "优化超时，使用当前最佳参数" << std::endl;
            break;
        }

        std::cout << "\n迭代 " << (iteration + 1) << "/" << max_iterations << std::endl;

        json parameters = bayesian_optimizer_->suggest_next_parameters(parameter_space);
        std::cout << "测试参数: " << parameters.dump(2) << std::endl;

        double performance = evaluate_parameters(algorithm_name, parameters, schedule);
        std::cout << "性能分数: " << performance << std::endl;

        bayesian_optimizer_->update(parameters, performance);

        if (performance > best_config.expected_performance) {
            best_config.parameters = parameters;
            best_config.expected_performance = performance;
            best_config.computation_time = elapsed_time;
            std::cout << "发现新的最佳参数！" << std::endl;
        }

        AlgorithmConfig current_config;
        current_config.algorithm_name = algorithm_name;
        current_config.parameters = parameters;
        current_config.expected_performance = performance;
        current_config.computation_time = elapsed_time;
        optimization_history_.push_back(current_config);
    }

    std::cout << "\n优化完成！" << std::endl;
    std::cout << "最佳性能: " << best_config.expected_performance << std::endl;

    return best_config;
}

void HyperparameterOptimizer::set_bayesian_optimizer(std::unique_ptr<BayesianOptimizer> optimizer) {
    bayesian_optimizer_ = std::move(optimizer);
}

json HyperparameterOptimizer::get_parameter_space(const std::string & algorithm_name) const {
    json space;

    if (algorithm_name == "NSGAII") {
        space["population_size"] = { {"type", "int"}, {"min", 50}, {"max", 300} };
        space["max_generations"] = { {"type", "int"}, {"min", 100}, {"max", 1000} };
        space["crossover_rate"] = { {"type", "float"}, {"min", 0.5}, {"max", 0.95} };
        space["mutation_rate"] = { {"type", "float"}, {"min", 0.01}, {"max", 0.3} };
        space["tournament_size"] = { {"type", "int"}, {"min", 2}, {"max", 10} };
    }
    else if (algorithm_name == "Greedy_Time" || algorithm_name == "Greedy_Cost") {
        space["lookahead_depth"] = { {"type", "int"}, {"min", 1}, {"max", 5} };
        space["local_search_iterations"] = { {"type", "int"}, {"min", 0}, {"max", 100} };
        space["time_weight"] = { {"type", "float"}, {"min", 0.0}, {"max", 1.0} };
        space["cost_weight"] = { {"type", "float"}, {"min", 0.0}, {"max", 1.0} };
    }
    else if (algorithm_name == "Exact_Time") {
        space["time_limit"] = { {"type", "int"}, {"min", 60}, {"max", 1800} };
        space["mip_gap"] = { {"type", "float"}, {"min", 0.001}, {"max", 0.1} };
        space["emphasis"] = { {"type", "categorical"},
                             {"values", {"balanced", "optimality", "feasibility"}} };
    }

    return space;
}

double HyperparameterOptimizer::evaluate_parameters(const std::string & algorithm_name,
    const json & parameters,
    Schedule * schedule) {

    auto start_time = std::chrono::steady_clock::now();
    std::vector<Tour*> result_tours;

    try {
        apply_parameters_to_algorithm(algorithm_name, parameters, schedule, result_tours);

        double performance_score = 0.0;
        if (!result_tours.empty()) {
            json algo_params = parameters;
            if (parameters.contains(algorithm_name)) {
                algo_params = parameters[algorithm_name];
            }

            auto result = AlgorithmEvaluator::evaluate_algorithm(algorithm_name, algo_params, schedule, 5);
            performance_score = result.total_score;
        }

        auto end_time = std::chrono::steady_clock::now();
        double compute_time = std::chrono::duration<double>(end_time - start_time).count();
        double time_penalty = std::exp(-compute_time / 60.0);

        return performance_score * time_penalty;

    }
    catch (const std::exception& e) {
        std::cerr << "评估参数时出错: " << e.what() << std::endl;
        return 0.0;
    }
}

void HyperparameterOptimizer::apply_parameters_to_algorithm(const std::string & algorithm_name,
    const json & parameters,
    Schedule * schedule,
    std::vector<Tour*>&result_tours) {

    result_tours.clear();

    if (algorithm_name == "NSGAII") {
        NSGAII* nsgaii = new NSGAII(schedule);
        nsgaii->run();
        auto results = nsgaii->getResult();
        if (!results.empty()) {
            result_tours = results.begin()->second;
        }
        delete nsgaii;
    }
    else if (algorithm_name == "Greedy_Time") {
        Greedy* greedy = new Greedy(schedule);
        auto results = greedy->getResult();
        auto it = results.find("timeShortest");
        if (it != results.end()) {
            result_tours = it->second;
        }
        delete greedy;
    }
    else if (algorithm_name == "Greedy_Cost") {
        Greedy* greedy = new Greedy(schedule);
        auto results = greedy->getResult();
        auto it = results.find("costLowest");
        if (it != results.end()) {
            result_tours = it->second;
        }
        delete greedy;
    }
    else if (algorithm_name == "Greedy_Difference") {
        Greedy* greedy = new Greedy(schedule);
        auto results = greedy->getResult();
        auto it = results.find("minDifference");
        if (it != results.end()) {
            result_tours = it->second;
        }
        delete greedy;
    }
    else if (algorithm_name == "Exact_Time") {
        schedule->generateSchemes();
        Model* model = new Model(schedule);
        model->solve();
        result_tours = model->getSolnTourList();
        delete model;
    }
}

json HyperparameterOptimizer::get_optimization_history() const {
    json history;
    for (const auto& config : optimization_history_) {
        history.push_back(config.to_json());
    }
    return history;
}

// ==================== TwoLayerRecommender::Impl 实现 ====================

class TwoLayerRecommender::Impl {
public:
    Impl(const std::string& model_dir)
        : model_dir_(model_dir),
        algo_recommender_(std::make_unique<AlgorithmRecommender>()),
        param_optimizer_(std::make_unique<HyperparameterOptimizer>()) {

        fs::create_directories(model_dir_);
        fs::create_directories(model_dir_ + "scenarios/");
        fs::create_directories(model_dir_ + "results/");
        fs::create_directories(model_dir_ + "logs/");

        stats_["total_scenarios"] = 0;
        stats_["successful_recommendations"] = 0;
        stats_["average_performance"] = 0.0;
    }

    bool train_algorithm_recommender(const std::string& training_data_path, const std::map<std::string, float>& class_weights = {}) {
        bool success = algo_recommender_->train_algorithm_recommender(training_data_path, class_weights);
        if (success) {
            stats_["model_trained"] = true;
            stats_["training_time"] = std::chrono::system_clock::now().time_since_epoch().count();
        }
        return success;
    }

    bool load_pretrained_model(const std::string& model_path) {
        return algo_recommender_->load_pretrained_model(model_path);
    }

    AlgorithmConfig recommend_and_optimize(Schedule* schedule,
        const std::string& scenario_name,const std::map<string,float>& features_map) {

        stats_["total_scenarios"] = stats_["total_scenarios"].get<int>() + 1;

        // 提取特征
        auto features = ScenarioFeatureExtractor::extract_features(schedule, scenario_name,features_map);

        // 推荐算法
        std::string recommended_algo = algo_recommender_->recommend_algorithm(features);

        // 优化参数
        //auto optimized_config = param_optimizer_->optimize_algorithm(
        //    recommended_algo, schedule);

        // 转换为AlgorithmConfig
        AlgorithmConfig result;
        result.algorithm_name = recommended_algo;
        //result.parameters = optimized_config.parameters;
        //result.expected_performance = optimized_config.expected_performance;
        //result.actual_performance = 0.0;

        // 保存推荐结果
        save_recommendation_result(scenario_name, features, recommended_algo, result);

        //if (result.expected_performance > 0.5) {
        //    stats_["successful_recommendations"] = stats_["successful_recommendations"].get<int>() + 1;
        //}

        history_.push_back(result);

        return result;
    }

    void process_scenarios_batch(const std::string& scenarios_dir) {
        std::vector<fs::path> scenario_files;
        for (const auto& entry : fs::directory_iterator(scenarios_dir)) {
            if (entry.path().extension() == ".json" &&
                entry.path().filename().string().find("scenario_") == 0) {
                scenario_files.push_back(entry.path());
            }
        }

        //std::sort(scenario_files.begin(), scenario_files.end());

        int processed = 0;
        for (const auto& scenario_path : scenario_files) {

            std::string scenario_name = scenario_path.stem().string();

            try {
                Schedule* schedule = new Schedule();
                DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
                dataLoader->loadFromFile(scenario_path.string());
                
                //读取测试场景中的特征数据
                map<string, float> features;
                features=dataLoader->loadFeaturesFromFile(scenario_path.string());
                //推荐算法，优化参数
                auto config = recommend_and_optimize(schedule, scenario_name, features);
                //按照算法求解结果并保存
                execute_with_config(schedule, config, scenario_name);

                delete dataLoader;
                delete schedule;

                processed++;
            }
            catch (const std::exception& e) {
                std::cerr << "处理场景 " << scenario_name << " 时出错: " << e.what() << std::endl;
            }
        }

        if (!history_.empty()) {
            double total_performance = 0.0;
            for (const auto& config : history_) {
                total_performance += config.expected_performance;
            }
            stats_["average_performance"] = total_performance / history_.size();
        }
    }

    // 处理二次规划场景的函数
    void process_secondary_scenarios(const std::string& secondary_scenarios_dir) {

        std::string prev_plan_dir = "../modelsForData/results/";

        if (!fs::exists(secondary_scenarios_dir)) {
            std::cout << "二次规划场景目录不存在: " << secondary_scenarios_dir << std::endl;
            return;
        }

        std::vector<fs::path> scenario_files;
        for (const auto& entry : fs::directory_iterator(secondary_scenarios_dir)) {
            if (entry.path().extension() == ".json" &&
                entry.path().filename().string().find("secondary_") == 0) {
                scenario_files.push_back(entry.path());
            }
        }

        if (scenario_files.empty()) {
            std::cout << "没有找到二次规划场景文件" << std::endl;
            return;
        }

        std::cout << "\n处理二次规划场景，共 " << scenario_files.size() << " 个..." << std::endl;

        for (const auto& scenario_path : scenario_files) {
            std::string scenario_name = scenario_path.stem().string();
            std::cout << "\n处理二次规划场景: " << scenario_name << std::endl;

            try {
                // 重置静态计数器
                Mount::initCountId();
                Load::initCountId();
                Target::initCountId();
                Base::initCountId();
                TargetType::initCountId();
                Tour::initCountId();
                MainTarget::initCountId();

                Schedule* schedule = new Schedule();
                DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
                dataLoader->loadFromFile(scenario_path.string());

                // 加载上次规划结果
                std::string prev_plan_path = find_previous_plan_file(scenario_name, prev_plan_dir);
                if (!prev_plan_path.empty()) {
                    load_previous_plan(schedule, prev_plan_path);
                }
                else {
                    std::cout << "警告: 未找到上次规划结果" << std::endl;
                }

                // 读取特征
                map<string, float> features_map = dataLoader->loadFeaturesFromFile(scenario_path.string());

                // 推荐算法
                auto config = recommend_and_optimize(schedule, scenario_name, features_map);

                std::cout << "推荐算法: " << config.algorithm_name << std::endl;

                // 按照算法求解结果并保存
                // 这里需要根据你的实际执行逻辑进行修改
                execute_with_config(schedule, config, scenario_name);

                delete dataLoader;
                delete schedule;

            }
            catch (const std::exception& e) {
                std::cerr << "处理二次规划场景 " << scenario_name << " 时出错: " << e.what() << std::endl;
            }
        }
    }


    std::vector<std::string> get_available_algorithms() const {
        return { "Exact_Time","Exact_Cost", "NSGAII", "Greedy_Time", "Greedy_Cost","Greedy_Difference"};
    }

    bool save_state(const std::string& save_path) {
        json state;
        state["statistics"] = stats_;
        state["history"] = json::array();

        for (const auto& config : history_) {
            state["history"].push_back(config.to_json());
        }

        std::ofstream file(save_path);
        if (file.is_open()) {
            file << state.dump(4);
            file.close();
            return true;
        }
        return false;
    }

    bool load_state(const std::string& load_path) {
        std::ifstream file(load_path);
        if (!file.is_open()) {
            return false;
        }

        try {
            json state;
            file >> state;

            stats_ = state["statistics"];
            history_.clear();

            for (const auto& config_json : state["history"]) {
                AlgorithmConfig config;
                config.algorithm_name = config_json["algorithm_name"];
                config.parameters = config_json["parameters"];
                config.expected_performance = config_json["expected_performance"];
                config.actual_performance = config_json["actual_performance"];
                history_.push_back(config);
            }

            return true;
        }
        catch (const json::exception& e) {
            std::cerr << "加载状态失败: " << e.what() << std::endl;
            return false;
        }
    }

    json get_statistics() const {
        return stats_;
    }

private:
    std::string model_dir_;
    std::unique_ptr<AlgorithmRecommender> algo_recommender_;
    std::unique_ptr<HyperparameterOptimizer> param_optimizer_;
    json stats_;
    std::vector<AlgorithmConfig> history_;

    void save_recommendation_result(const std::string& scenario_name,
        const ScenarioFeatureExtractor::ScenarioFeatures& features,
        const std::string& recommended_algo,
        const AlgorithmConfig& config) {

        json result = {
            {"scenario_name", scenario_name},
            {"scenario_features", features.to_json()},
            {"recommended_algorithm", recommended_algo},
            {"optimized_config", config.to_json()},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        };

        std::string result_file = model_dir_ + "results/recommendation_" +
            scenario_name + ".json";
        std::ofstream file(result_file);
        if (file.is_open()) {
            file << result.dump(4);
            file.close();
        }
    }

    void execute_with_config(Schedule* schedule,
        const AlgorithmConfig& config,
        const std::string& scenario_name) {

        std::vector<Tour*> result_tours;
        auto start_time = std::chrono::steady_clock::now();

        try {
            if (config.algorithm_name == "NSGAII") {
                NSGAII* nsgaii = new NSGAII(schedule);
                nsgaii->run();
                auto results = nsgaii->getResult();
                if (!results.empty()) {
                    result_tours = results.begin()->second;
                }
                delete nsgaii;
            }
            else if (config.algorithm_name == "Greedy_Time") {
                Greedy* greedy = new Greedy(schedule);
                auto results = greedy->getResult();
                auto it = results.find("timeShortest");
                if (it != results.end()) {
                    result_tours = it->second;
                }
                delete greedy;
            }
            else if (config.algorithm_name == "Greedy_Cost") {
                Greedy* greedy = new Greedy(schedule);
                auto results = greedy->getResult();
                auto it = results.find("costLowest");
                if (it != results.end()) {
                    result_tours = it->second;
                }
                delete greedy;
            }
            else if (config.algorithm_name == "Greedy_Difference") {
                Greedy* greedy = new Greedy(schedule);
                auto results = greedy->getResult();
                auto it = results.find("minDifference");
                if (it != results.end()) {
                    result_tours = it->second;
                }
                delete greedy;
            }
            else if (config.algorithm_name == "Exact_Time") {
                Util::weightTourTime = 1000;
                schedule->generateSchemes();
                Model* model = new Model(schedule);
                model->solve();
                result_tours = model->getSolnTourList();
                delete model;
            }
            else if (config.algorithm_name == "Exact_Cost") {
                Util::weightTourTime = 10;
                schedule->generateSchemes();
                Model* model = new Model(schedule);
                model->solve();
                result_tours = model->getSolnTourList();
                delete model;
            }
            if (!result_tours.empty()) {
                DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
                std::string solnOutput = dataLoader->writeJsonOutput(result_tours,
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count());

                std::string output_file = model_dir_ + "results/execution_" +
                    scenario_name + "_" + config.algorithm_name + ".json";
                std::ofstream file(output_file);
                if (file.is_open()) {
                    file << solnOutput;
                    file.close();
                }
                delete dataLoader;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "执行算法时出错: " << e.what() << std::endl;
        }
    }
};

// ==================== TwoLayerRecommender 公共方法实现 ====================

json TwoLayerRecommender::AlgorithmConfig::to_json() const {
    return {
        {"algorithm_name", algorithm_name},
        {"parameters", parameters},
        {"expected_performance", expected_performance},
        {"actual_performance", actual_performance},
        {"computation_time", computation_time}
    };
}

TwoLayerRecommender::TwoLayerRecommender(const std::string & model_dir)
    : impl_(std::make_unique<Impl>(model_dir)) {}

TwoLayerRecommender::~TwoLayerRecommender() = default;

bool TwoLayerRecommender::train_algorithm_recommender(const std::string & training_data_path, const std::map<std::string, float>& class_weights) {
    return impl_->train_algorithm_recommender(training_data_path, class_weights);
}

bool TwoLayerRecommender::load_pretrained_model(const std::string & model_path) {
    return impl_->load_pretrained_model(model_path);
}

TwoLayerRecommender::AlgorithmConfig
TwoLayerRecommender::recommend_and_optimize(Schedule * schedule,
    const std::string & scenario_name,const map<string,float>&features_map) {
    return impl_->recommend_and_optimize(schedule, scenario_name,features_map);
}

void TwoLayerRecommender::process_scenarios_batch(const std::string & scenarios_dir) {
    impl_->process_scenarios_batch(scenarios_dir);
}
void TwoLayerRecommender::process_secondary_scenarios(const std::string& scenarios_dir) {
    impl_->process_secondary_scenarios(scenarios_dir);
}

std::vector<std::string> TwoLayerRecommender::get_available_algorithms() const {
    return impl_->get_available_algorithms();
}

bool TwoLayerRecommender::save_state(const std::string & save_path) const {
    return impl_->save_state(save_path);
}

bool TwoLayerRecommender::load_state(const std::string & load_path) {
    return impl_->load_state(load_path);
}

json TwoLayerRecommender::get_statistics() const {
    return impl_->get_statistics();
}

// ==================== 工具函数 ====================

json create_training_data_from_existing_scenarios(const std::string& scenarios_dir,
    const std::vector<std::string>& algorithms,
    int max_scenarios,
    const std::string& output_path,
    bool is_secondary,
    string results_dir,  // 规划计算结果的保存目录
    bool save_comparison_results) {  // 新增参数

    std::vector<fs::path> scenario_files;
    for (const auto& entry : fs::directory_iterator(scenarios_dir)) {
        if (entry.path().extension() == ".json" &&
            (entry.path().filename().string().find("scenario_") == 0 ||
                entry.path().filename().string().find("secondary_") == 0)) {
            scenario_files.push_back(entry.path());
        }
    }

    json training_data = json::array();

    int processed = 0;
    int total = std::min(static_cast<int>(scenario_files.size()),
        max_scenarios > 0 ? max_scenarios : INT_MAX);

    std::cout << "开始处理 " << total << " 个场景..." << std::endl;
    if (is_secondary) {
        std::cout << "（二次规划模式）" << std::endl;
    }

    // 上次规划结果目录（如果是二次规划）
    std::string prev_plan_dir = "../modelsForData/results/";

    // 创建对比结果输出目录
    std::string comparison_dir;
    if (save_comparison_results) {
        comparison_dir = fs::path(results_dir).string() + "/comparison_results/";
        fs::create_directories(comparison_dir);
        std::cout << "对比结果将保存到: " << comparison_dir << std::endl;
    }

    for (int i = 0; i < total; ++i) {
        const auto& scenario_path = scenario_files[i];
        std::string scenario_name = scenario_path.stem().string();
        std::cout << "\n处理场景 " << (i + 1) << "/" << total
            << ": " << scenario_name << std::endl;

        try {
            // 重置静态计数器
            Mount::initCountId();
            Load::initCountId();
            Target::initCountId();
            Base::initCountId();
            TargetType::initCountId();
            Tour::initCountId();
            MainTarget::initCountId();

            Schedule* schedule = new Schedule();
            DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
            dataLoader->loadFromFile(scenario_path.string());

            // 如果是二次规划，加载上次规划结果
            if (is_secondary) {
                std::cout << "  加载上次规划结果..." << std::endl;
                std::string prev_plan_path = find_previous_plan_file(scenario_name, prev_plan_dir);
                if (!prev_plan_path.empty()) {
                    load_previous_plan(schedule, prev_plan_path);
                    std::cout << "  √ 已加载上次规划结果: " << prev_plan_path << std::endl;
                }
                else {
                    std::cout << "  ⚠ 未找到上次规划结果，继续处理..." << std::endl;
                }
            }

            std::cout << "  读取场景特征..." << std::endl;
            map<string, float> features_map = dataLoader->loadFeaturesFromFile(scenario_path.string());

            std::ifstream file(scenario_path.string());
            json scenario_json;
            file >> scenario_json;
            file.close();

            // 提取特征
            ScenarioFeatureExtractor::ScenarioFeatures features;
            features.scenario_name = scenario_name;

            // 使用与 split_dataset 一致的特征提取方法
            features = ScenarioFeatureExtractor::extract_features(schedule, scenario_name, features_map);

            std::cout << "  评估算法性能..." << std::endl;
            // 决策偏好，0时间最短，1成本最小
            float decision_preferance = features_map["decision_preferance"];

            // 设置输出目录
            std::string output_dir_for_comparison = "";
            if (save_comparison_results && !comparison_dir.empty()) {
                output_dir_for_comparison = comparison_dir;
            }

            // 调用 AlgorithmEvaluator 生成标签（与 split_dataset 保持一致）
            std::string best_algorithm = AlgorithmEvaluator::generate_algorithm_label(
                schedule,
                algorithms,
                json::object(),
                5,  // 5秒超时
                decision_preferance,
                output_dir_for_comparison,  // 对比结果输出目录
                scenario_name
            );

            // 手动设置二次规划标志
            if (is_secondary) {
                features.is_secondary_planning = true;
                // 尝试从场景文件中提取二次规划特征
                try {
                    std::ifstream file(scenario_path.string());
                    json scenario_json;
                    file >> scenario_json;
                    file.close();

                    if (scenario_json.contains("scenario_features")) {
                        const auto& scenario_feat = scenario_json["scenario_features"];
                        if (scenario_feat.contains("target_change_ratio")) {
                            features.target_change_ratio = scenario_feat["target_change_ratio"];
                        }
                        if (scenario_feat.contains("original_target_count")) {
                            features.original_target_count = scenario_feat["original_target_count"];
                        }
                    }
                }
                catch (...) {
                    // 忽略解析错误
                }
            }

            // 创建训练样本
            json sample = {
                {"scenario_name", scenario_name},
                {"features", features.to_json()},
                {"best_algorithm", best_algorithm},
                {"scenario_path", scenario_path.string()},
                {"is_secondary", is_secondary},  // 记录是否是二次规划
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                {"comparison_results_saved", !output_dir_for_comparison.empty()}
            };

            training_data.push_back(sample);
            processed++;

            std::cout << "  √ 场景处理完成，最佳算法: " << best_algorithm << std::endl;

            // 打印一些特征信息用于调试
            if (is_secondary) {
                std::cout << "    目标变化率: " << features.target_change_ratio
                    << ", 原始目标数: " << features.original_target_count << std::endl;
            }

            // 如果启用了对比保存，并且当前场景处理完成，打印提示
            if (save_comparison_results && !output_dir_for_comparison.empty()) {
                std::cout << "    √ 对比结果已保存到: " << output_dir_for_comparison << std::endl;

                // 检查对比结果文件是否创建成功
                std::string comparison_file = output_dir_for_comparison + scenario_name + "_comparison.json";
                if (fs::exists(comparison_file)) {
                    std::cout << "    √ 对比结果文件已创建: " << comparison_file << std::endl;
                }
            }

            delete dataLoader;
            delete schedule;

        }
        catch (const std::exception& e) {
            std::cerr << "  × 处理场景 " << scenario_name << " 时出错: " << e.what() << std::endl;
            continue;
        }
    }

    // 保存训练数据到文件
    std::string training_data_file = output_path;
    std::ofstream out_file(training_data_file);
    if (out_file.is_open()) {
        out_file << training_data.dump(4);
        out_file.close();
        std::cout << "\n训练数据已保存到: " << training_data_file << std::endl;
        std::cout << "共处理 " << processed << "/" << total << " 个场景" << std::endl;

        // 输出统计信息
        std::cout << "\n训练数据统计信息:" << std::endl;
        std::cout << "  规划类型: " << (is_secondary ? "二次规划" : "一次规划") << std::endl;
        std::cout << "  场景目录: " << scenarios_dir << std::endl;
        std::cout << "  对比结果保存: " << (save_comparison_results ? "是" : "否") << std::endl;
        if (save_comparison_results) {
            std::cout << "  对比结果目录: " << comparison_dir << std::endl;
        }

        // 统计算法分布
        std::map<std::string, int> algo_counts;
        for (const auto& sample : training_data) {
            std::string algo = sample["best_algorithm"];
            algo_counts[algo]++;
        }

        std::cout << "  算法分布:" << std::endl;
        for (const auto& pair : algo_counts) {
            std::cout << "    " << pair.first << ": " << pair.second
                << " (" << (pair.second * 100.0 / processed) << "%)" << std::endl;
        }

        // 如果启用了对比保存，生成总体分析报告
        if (save_comparison_results && !comparison_dir.empty()) {
            std::cout << "\n=== 开始生成总体对比分析报告 ===" << std::endl;

            // 检查是否有对比结果文件
            int comparison_file_count = 0;
            for (const auto& entry : fs::directory_iterator(comparison_dir)) {
                if (entry.path().extension() == ".json" &&
                    entry.path().filename().string().find("_comparison.json") != std::string::npos) {
                    comparison_file_count++;
                }
            }

            if (comparison_file_count > 0) {
                std::cout << "找到 " << comparison_file_count << " 个对比结果文件" << std::endl;

                // 调用总体分析函数
                try {
                    auto overall_stats = AlgorithmEvaluator::analyze_overall_statistics(comparison_dir);

                    //// 将总体统计添加到训练数据文件中
                    //json training_data_with_stats;
                    //std::ifstream in_file(training_data_file);
                    //if (in_file.is_open()) {
                    //    in_file >> training_data_with_stats;
                    //    in_file.close();

                    //    training_data_with_stats["overall_comparison_stats"] = overall_stats;

                    //    std::ofstream stats_file(training_data_file);
                    //    if (stats_file.is_open()) {
                    //        stats_file << training_data_with_stats.dump(4);
                    //        stats_file.close();
                    //        std::cout << "总体对比统计已添加到训练数据文件" << std::endl;
                    //    }
                    //}

                    // 打印关键统计
                    std::cout << "\n=== 算法对比总体统计 ===" << std::endl;
                    std::cout << "总场景数: " << overall_stats["total_scenarios"] << std::endl;
                    std::cout << "有效场景数: " << overall_stats["scenarios_with_data"] << std::endl;

                    if (overall_stats.contains("algorithm_performance")) {
                        std::cout << "\n各算法表现:" << std::endl;
                        for (const auto& [algo, stats] : overall_stats["algorithm_performance"].items()) {
                            double success_rate = stats.value("success_rate", 0.0);
                            double avg_score = stats.value("avg_total_score", 0.0);
                            double avg_time = stats.value("avg_computation_time", 0.0);

                            std::cout << "  " << std::setw(15) << std::left << algo
                                << ": 成功率=" << std::fixed << std::setprecision(1) << success_rate * 100 << "%"
                                << ", 平均分=" << std::fixed << std::setprecision(3) << avg_score
                                << ", 平均耗时=" << std::fixed << std::setprecision(2) << avg_time << "秒" << std::endl;
                        }
                    }

                }
                catch (const std::exception& e) {
                    std::cerr << "生成总体分析报告时出错: " << e.what() << std::endl;
                }
            }
            else {
                std::cout << "警告: 未找到对比结果文件，跳过总体分析" << std::endl;
            }
        }

    }
    else {
        std::cerr << "无法保存训练数据到文件: " << training_data_file << std::endl;
    }

    return training_data;
}

// 查找上次规划结果文件
std::string find_previous_plan_file(const std::string& scenario_name, const std::string& prev_plan_dir) {
    // 二次规划文件名格式：secondary_1000_from_scenario_100_ratio60_T30_R116
    // 提取来源场景ID "scenario_100"

    std::regex secondary_pattern("secondary_\\d+_from_(scenario_\\d+)_ratio[-]?\\d+_T\\d+_R\\d+");
    std::regex primary_pattern("scenario_\\d+_T\\d+_R\\d+");
    std::smatch match;

    std::string source_scenario_id;

    // 首先检查是否是二次规划文件名
    if (std::regex_search(scenario_name, match, secondary_pattern)) {
        source_scenario_id = match[1].str();  // 提取 "scenario_100"
        std::cout << "二次规划场景: " << scenario_name
            << ", 来源场景ID: " << source_scenario_id << std::endl;
    }
    // 如果是原场景文件名
    else if (std::regex_search(scenario_name, match, primary_pattern)) {
        // 提取完整的原场景文件名部分，如 "scenario_100_T30_R193"
        source_scenario_id = match[0].str();  // 完整的原场景文件名模式
        std::cout << "原场景: " << scenario_name
            << ", 场景模式: " << source_scenario_id << std::endl;
    }
    else {
        std::cout << "无法识别的场景文件名格式: " << scenario_name << std::endl;
        return "";
    }

    // 在结果目录中查找相关文件
    if (fs::exists(prev_plan_dir)) {
        std::cout << "在目录 " << prev_plan_dir << " 中查找匹配 " << source_scenario_id << " 的文件..." << std::endl;

        for (const auto& entry : fs::directory_iterator(prev_plan_dir)) {
            std::string filename = entry.path().filename().string();

            // 查找包含来源场景ID的文件
            // 原规划结果文件可能格式如: "result_scenario_100_...json" 或 "scenario_100_plan.json" 等
            if (filename.find(source_scenario_id) != std::string::npos) {
                std::cout << "找到匹配文件: " << filename << std::endl;
                return entry.path().string();
            }
        }

        std::cout << "未找到包含 " << source_scenario_id << " 的规划结果文件" << std::endl;
    }
    else {
        std::cout << "规划结果目录不存在: " << prev_plan_dir << std::endl;
    }

    return ""; // 找不到上次规划结果
}

// 加载上次规划结果
void load_previous_plan(Schedule* schedule, const std::string& prev_plan_path) {
    if (prev_plan_path.empty()) {
        return;
    }

    try {
        DataLoader_Nlohmann* dataLoader = new DataLoader_Nlohmann(schedule);
        dataLoader->loadPlan(prev_plan_path);
        cout << "已加载上次规划结果: " << prev_plan_path << endl;
        delete dataLoader;
    }
    catch (const std::exception& e) {
        cerr << "加载上次规划结果失败: " << e.what() << endl;
    }
}


