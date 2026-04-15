// HyperparameterOptimizer.h - 两层推荐系统头文件
#ifndef HYPERPARAMETER_OPTIMIZER_H
#define HYPERPARAMETER_OPTIMIZER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/ml.hpp>

// 前向声明
class Schedule;
class Target;
class Tour;
class Model;
class NSGAII;
class Greedy;
class DataLoader_Nlohmann;
class Fleet;

using json = nlohmann::json;

// ==================== ScenarioFeatureExtractor 类 ====================

class ScenarioFeatureExtractor {
public:
    // 特征分布统计结构
    struct DistributionStats {
        float mean = 0.0f;
        float std_dev = 0.0f;
        float min = 0.0f;
        float max = 0.0f;
        float skewness = 0.0f;
        float kurtosis = 0.0f;
        std::vector<float> percentiles;  // 25%, 50%, 75%
    };

    // 历史规划性能结构
    struct HistoricalPlanMetrics {
        float previous_coverage = 0.0f;
        float previous_cost = 0.0f;
        float previous_time = 0.0f;
        float previous_resource_utilization = 0.0f;
    };
    struct ScenarioFeatures {
        // 基础特征
        std::string scenario_name;
        int num_targets = 0;
        int num_resources = 0;
        int num_target_types = 0;
        int num_resource_types = 0;

        //外部场景特征
        double decision_preferance = 0.0;//0表示时间最短，1表示成本最小，2表示差异最小
        double target_time_urgency = 0.0;

        // 资源特征
        double resource_target_ratio = 10.0;


        // 统计特征
        DistributionStats target_distance_stats;      // 目标距离分布
        DistributionStats resource_capacity_stats;    // 资源容量分布
        DistributionStats time_window_stats;          // 时间窗口分布
        DistributionStats cost_distribution_stats;    // 成本分布

        // 拓扑特征
        float spatial_clustering_coefficient = 0.0f;  // 空间聚类系数
        //float resource_target_distance_mean = 0.0f;   // 资源-目标平均距离
        float target_density = 0.0f;                  // 目标密度

        // 复杂度特征
        float problem_complexity_score = 0.0f;        // 问题复杂度评分
        float constraint_tightness = 0.0f;            // 约束紧度

        // 二次规划特定特征
        bool is_secondary_planning = false;
        float target_change_ratio = 0.0f;             // 目标数量变化比例
        int original_target_count = 0;                // 原目标数量

        // 历史规划性能特征
        HistoricalPlanMetrics historical_metrics;
        // 转换为OpenCV矩阵
        cv::Mat to_mat() const;

        // 转换为JSON
        json to_json() const;
    };

    static ScenarioFeatures extract_features(Schedule* schedule, const std::string& scenario_name, const std::map<std::string, float>& features_map);
    static json get_default_features_template();
private:

    // 统计计算工具
    static DistributionStats calculate_distribution_stats(const std::vector<float>& values);
    static float calculate_mean(const std::vector<float>& values);
    static float calculate_std_dev(const std::vector<float>& values, float mean);
    static std::vector<float> calculate_percentiles(const std::vector<float>& values);
    static float calculate_skewness(const std::vector<float>& values, float mean, float std_dev);
    static float calculate_kurtosis(const std::vector<float>& values, float mean, float std_dev);

    // 空间特征计算
    static float calculate_spatial_clustering(const std::vector<std::pair<double, double>>& positions);
    static float calculate_target_density(const std::vector<std::pair<double, double>>& positions);
    static std::vector<float> calculate_target_distances(const std::vector<std::pair<double, double>>& positions);

    // 资源特征计算
    static DistributionStats calculate_resource_capacity_stats(const std::vector<Fleet*>& resources);
    static DistributionStats calculate_cost_stats(const std::vector<Fleet*>& resources);
    static std::vector<float> extract_leadtimes(const std::vector<Target*>& targets);



    // 坐标转换辅助函数
    static std::pair<double, double> parse_coordinates(const std::string& coord_str);
    static double calculate_distance(const std::pair<double, double>& p1,
        const std::pair<double, double>& p2);
};
// ==================== AlgorithmEvaluator 类 ====================
enum ScenarioCategory {
    CATEGORY_A_SMALL_EXACT = 0,      // 小规模精确可解
    CATEGORY_B_MEDIUM_MULTIOBJECTIVE = 1, // 中规模多目标
    CATEGORY_C_LARGE_HEURISTIC = 2,  // 大规模启发式
    CATEGORY_D_REPLANNING = 3        // 动态重规划
};
class AlgorithmEvaluator {
public:
    struct EvaluationResult {
        bool is_feasible = false;
        double total_score = 0.0;
        double coverage_score = 0.0;
        double cost_score = 0.0;
        double time_score = 0.0;
        double resource_score = 0.0;
        double feasibility_score = 0.0;
        double computation_time = 0.0;
        double difference_score= 0.0;
        json detailed_metrics;

        // 转换为JSON
        json to_json() const;
    };
    
    //评估单一算法得分
    static EvaluationResult evaluate_algorithm(const std::string& algorithm_name,
        const json& parameters,
        Schedule* schedule,
        int timeout_seconds = 180,
        float decision_preferance=0.0);
    //计算多目标加权得分
    static double calculate_final_score(const EvaluationResult& result, const json& weights);
    static std::string generate_algorithm_label(Schedule* schedule,
        const std::vector<std::string>& algorithms,
        const json& parameters,
        int timeout_seconds=180,
        float decision_preferance=0.0,
        const std::string& output_dir = "", // 新增输出目录参数
        const std::string& ScenarioName=""); // 新增场景名称参数

    static json calculate_scenario_statistics(const json& comparison_stats);

    static void save_comparison_results(const json& comparison_stats,
        const std::string& output_dir,
        const std::string& scenario_name);

    static json analyze_overall_statistics(const std::string& results_dir);

    static void generate_csv_summary(const std::map<std::string, json>& algo_stats,
        const std::string& results_dir);
    static void generate_category_summary_table(
        const std::map<ScenarioCategory, std::map<std::string, json>>& category_algo_stats,
        const std::map<ScenarioCategory, int>& category_scenario_counts,
        const std::string& results_dir);
    static void generate_category_csv_summary(
        const std::map<ScenarioCategory, std::map<std::string, json>>& category_algo_stats,
        const std::map<ScenarioCategory, int>& category_scenario_counts,
        const std::string& results_dir);
};

// ==================== AlgorithmRecommender 类 ====================

class AlgorithmRecommender {
public:
    AlgorithmRecommender();
    struct TestResult {
        double accuracy = 0.0;
        double primary_accuracy = 0.0;      // 新增：一次规划准确率
        double secondary_accuracy = 0.0;    // 新增：二次规划准确率
        int primary_total = 0;              // 新增：一次规划样本数
        int secondary_total = 0;            // 新增：二次规划样本数
        std::map<std::string, int> correct_by_algorithm;
        std::map<std::string, int> total_by_algorithm;
        std::map<std::string, int> primary_by_algorithm;    // 新增
        std::map<std::string, int> secondary_by_algorithm;  // 新增
        std::vector<std::vector<int>> confusion_matrix;
        json detailed_results;

        json to_json() const;
    };

    TestResult evaluate_model(const std::string& test_data_path, const std::string& secondary_test_dir);

    // 模型训练和保存
    bool train_algorithm_recommender(const std::string& training_data_path, const std::map<std::string, float>& class_weights = {});
    bool load_pretrained_model(const std::string& model_path);
    bool save_model(const std::string& base_path) const;

    // 预测
    std::string recommend_algorithm(const ScenarioFeatureExtractor::ScenarioFeatures& features);
    std::map<std::string, double> predict_algorithm_performance(
        const ScenarioFeatureExtractor::ScenarioFeatures& features);

    // 辅助函数
    int algo_label_to_int(const std::string& algo_label) const;
    std::string default_recommendation(const ScenarioFeatureExtractor::ScenarioFeatures& features) const;

    // 状态查询
    bool is_model_trained() const { return model_trained_; }
    const std::vector<std::string>& get_algorithm_labels() const { return algorithm_labels_; }

private:
    cv::Ptr<cv::ml::DTrees> decision_tree_;
    bool model_trained_;
    std::vector<std::string> algorithm_labels_;
    std::vector<std::string> feature_names_;

    // 训练辅助函数
    double calculate_training_accuracy(const cv::Mat& features, const cv::Mat& labels);
    std::vector<double> get_feature_importance() const;
    void save_decision_rules(const std::string& filename) const;
    void print_test_results(const TestResult& result);
    void save_evaluation_results(const TestResult& result, const std::string& base_path);
};

// ==================== BayesianOptimizer 接口 ====================

class BayesianOptimizer {
public:
    virtual ~BayesianOptimizer() = default;
    virtual json suggest_next_parameters(const json& parameter_space) = 0;
    virtual void update(const json& parameters, double performance) = 0;
    virtual json get_best_parameters() const = 0;
    virtual double get_best_performance() const = 0;
    virtual void reset() = 0;
    virtual json get_optimization_history() const = 0;
    virtual size_t get_iteration_count() const = 0;
};

// ==================== UCBBayesianOptimizer 类 ====================

class UCBBayesianOptimizer : public BayesianOptimizer {
private:
    class Impl;
    std::unique_ptr<Impl> impl_;

public:
    UCBBayesianOptimizer(double exploration_weight = 2.0);
    ~UCBBayesianOptimizer();

    // BayesianOptimizer 接口实现
    json suggest_next_parameters(const json& parameter_space) override;
    void update(const json& parameters, double performance) override;
    json get_best_parameters() const override;
    double get_best_performance() const override;
    void reset() override;
    json get_optimization_history() const override;
    size_t get_iteration_count() const override;

    // 特定方法
    void set_exploration_weight(double weight);
    double get_exploration_weight() const;
    void set_random_seed(unsigned int seed);
};

// ==================== HyperparameterOptimizer 类 ====================

class HyperparameterOptimizer {
public:
    struct AlgorithmConfig {
        std::string algorithm_name;
        json parameters;
        double expected_performance = 0.0;
        double actual_performance = 0.0;
        double computation_time = 0.0;

        json to_json() const;
    };
    json get_default_parameters(const std::string& algorithm_name);
    json get_preoptimized_parameters(
        const std::string& algorithm_name,
        const ScenarioFeatureExtractor::ScenarioFeatures& features,
        const std::string& cluster_model_path);
private:
    std::unique_ptr<BayesianOptimizer> bayesian_optimizer_;
    std::vector<AlgorithmConfig> optimization_history_;

    // 辅助函数
    json get_parameter_space(const std::string& algorithm_name) const;
    double evaluate_parameters(const std::string& algorithm_name,
        const json& parameters,
        Schedule* schedule);
    void apply_parameters_to_algorithm(const std::string& algorithm_name,
        const json& parameters,
        Schedule* schedule,
        std::vector<Tour*>& result_tours);

public:
    HyperparameterOptimizer();
    ~HyperparameterOptimizer();

    // 主要功能
    AlgorithmConfig optimize_algorithm(const std::string& algorithm_name,
        Schedule* schedule,
        int max_iterations = 20,
        int timeout_seconds = 300);

    // 配置
    void set_bayesian_optimizer(std::unique_ptr<BayesianOptimizer> optimizer);

    // 查询
    json get_optimization_history() const;
};

// ==================== TwoLayerRecommender 类 ====================

class TwoLayerRecommender {
private:
    class Impl;
    std::unique_ptr<Impl> impl_;//智能指针 std::unique_ptr 来持有 Impl 类的实例

public:
    struct AlgorithmConfig {
        std::string algorithm_name;
        json parameters;
        double expected_performance = 0.0;
        double actual_performance = 0.0;
        double computation_time = 0.0;

        json to_json() const;
    };

    TwoLayerRecommender(const std::string& model_dir);
    ~TwoLayerRecommender();

    // 模型管理
    // 原有方法（保持向后兼容）
    bool train_algorithm_recommender(const std::string& training_data_path);
    bool train_algorithm_recommender(const std::string& training_data_path,const std::map<std::string, float>& class_weights = {});
    bool load_pretrained_model(const std::string& model_path);

    // 推荐和优化
    AlgorithmConfig recommend_and_optimize(Schedule* schedule,
        const std::string& scenario_name, const std::map<std::string, float>& features_map);

    // 批量处理
    void process_scenarios_batch(const std::string& scenarios_dir);
    // 处理二次规划场景的函数
    void process_secondary_scenarios(const std::string& secondary_scenarios_dir);

    // 状态管理
    bool save_state(const std::string& save_path) const;
    bool load_state(const std::string& load_path);

    // 查询
    std::vector<std::string> get_available_algorithms() const;
    json get_statistics() const;

};

// ==================== 工具函数 ====================

json create_training_data_from_existing_scenarios(const std::string& scenarios_dir,
    const std::vector<std::string>& algorithms,
    int max_scenarios = 0,
    const std::string& output_path = "../data/training/",
    bool is_secondary = false,
    std::string results_dir="",  // 规划计算结果的保存目录
    bool save_comparison_results = true);   // 新增参数

// 查找上次规划结果文件
std::string find_previous_plan_file(const std::string& scenario_name, const std::string& prev_plan_dir);

// 加载上次规划结果
void load_previous_plan(Schedule* schedule, const std::string& prev_plan_path);

#endif // HYPERPARAMETER_OPTIMIZER_H