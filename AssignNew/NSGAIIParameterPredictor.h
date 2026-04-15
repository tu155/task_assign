//#pragma once
//// NSGAIIParameterPredictor.h
//// OpenCV
//#include <opencv2/core.hpp>
//#include <opencv2/ml.hpp>
//
//// 前向声明
//class Schedule;
//class ScenarioFeatureExtractor;
//class NSGAIIParameterPredictor {
//private:
//    cv::Ptr<cv::ml::RTrees> model_;
//    std::vector<std::string> feature_names_;
//    bool is_trained_ = false;
//
//    // NSGAII参数范围
//    struct ParamRange {
//        double min, max;
//    };
//    std::map<std::string, ParamRange> param_ranges_;
//
//public:
//    NSGAIIParameterPredictor();
//
//    // 收集训练数据：场景特征 -> 最佳NSGAII参数
//    void collect_training_data(Schedule* schedule,
//        const std::string& scenario_name,
//        const std::map<std::string, float>& features_map);
//
//    // 训练模型
//    bool train(const std::string& data_path);
//
//    // 预测参数
//    std::map<std::string, float> predict_parameters(
//        const ScenarioFeatureExtractor::ScenarioFeatures& features);
//
//    // 保存/加载模型
//    bool save(const std::string& path);
//    bool load(const std::string& path);
//};