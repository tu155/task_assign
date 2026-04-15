#pragma once
// PerformanceUtils.h
#ifndef PERFORMANCE_UTILS_H
#define PERFORMANCE_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "Schedule.h"
#include "Tour.h"

// 训练数据生成器
class TrainingDataGenerator {
public:
    // 生成算法推荐训练数据
    static nlohmann::json generate_algorithm_training_data(
        const std::vector<nlohmann::json>& scenario_features,
        const std::vector<nlohmann::json>& algorithm_results);

    // 生成参数推荐训练数据
    static std::map<std::string, nlohmann::json> generate_parameter_training_data(
        const std::vector<nlohmann::json>& algorithm_results);

    // 分析特征重要性
    static nlohmann::json analyze_feature_importance(
        const std::vector<nlohmann::json>& training_data);

    // 生成测试集和训练集
    static std::pair<nlohmann::json, nlohmann::json> split_train_test(
        const nlohmann::json& all_data, float test_ratio = 0.2);
};

#endif // PERFORMANCE_UTILS_H