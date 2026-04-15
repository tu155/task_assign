//#pragma once
//// AlgorithmParameterPredictor.h
//// OpenCV
//#include <opencv2/core.hpp>
//#include <opencv2/ml.hpp>
//class AlgorithmParameterPredictor {
//private:
//    // 各个算法的预测模型
//    cv::Ptr<cv::ml::RTrees> nsgaii_model_;
//    cv::Ptr<cv::ml::RTrees> exact_model_;
//
//    // 决策偏好映射
//    enum DecisionPreference { TIME_FIRST = 0, COST_FIRST = 1, DIFFERENCE_FIRST = 2 };
//
//public:
//    AlgorithmParameterPredictor();
//
//    // 收集所有算法的训练数据
//    void collect_training_data(
//        Schedule* schedule,
//        const std::string& scenario_name,
//        const std::map<std::string, float>& features_map,
//        DecisionPreference preference);
//
//    // 预测NSGAII参数
//    std::map<std::string, float> predict_nsgaii_params(
//        const ScenarioFeatureExtractor::ScenarioFeatures& features,
//        DecisionPreference preference);
//
//    // 预测Exact权重（重点！）
//    double predict_exact_weight(
//        const ScenarioFeatureExtractor::ScenarioFeatures& features,
//        DecisionPreference preference);
//
//    // 训练所有模型
//    bool train_all_models(const std::string& data_dir);
//
//    // 保存/加载
//    bool save_models(const std::string& path);
//    bool load_models(const std::string& path);
//};