#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <cmath>

using namespace std;
namespace fs = std::filesystem;

// 拉丁超立方采样类
class LatinHypercubeSampling {
private:
    int dimensions;
    int samples;
    std::mt19937 rng;

public:
    LatinHypercubeSampling(int dim, int n_samples, int seed = 42)
        : dimensions(dim), samples(n_samples), rng(seed) {}

    std::vector<std::vector<double>> generate(const std::vector<std::pair<double, double>>& ranges) {
        std::vector<std::vector<double>> result(samples, std::vector<double>(dimensions));

        // 为每个维度生成分位数
        for (int d = 0; d < dimensions; ++d) {
            std::vector<double> quantiles(samples);
            for (int i = 0; i < samples; ++i) {
                quantiles[i] = (i + 0.5) / samples; // 中心分位数
            }

            // 打乱分位数顺序
            std::shuffle(quantiles.begin(), quantiles.end(), rng);

            // 映射到实际范围
            double min_val = ranges[d].first;
            double max_val = ranges[d].second;
            for (int i = 0; i < samples; ++i) {
                result[i][d] = min_val + quantiles[i] * (max_val - min_val);
            }
        }

        return result;
    }
};

// 生成随机字符串的函数
static std::string generateRandomString(const std::string& prefix, int length) {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(0, sizeof(alphanum) - 2);

    std::string s = prefix;
    for (int i = 0; i < length; ++i) {
        s += alphanum[dist(mt)];
    }
    return s;
}

// 生成一个在 [min, max] 范围内的随机偶数
static int generate_even_number(int min, int max) {
    static std::mt19937 gen(std::random_device{}());

    if (min % 2 != 0) ++min;
    if (max % 2 != 0) --max;

    if (min > max) {
        return min;
    }

    std::uniform_int_distribution<int> dist(min / 2, max / 2);
    return dist(gen) * 2;
}

// 从 [min, max] 中不重复地随机取出 k 个数字
static vector<int> pick_unique(int min, int max, std::size_t k) {
    if (min > max) std::swap(min, max);
    std::size_t n = static_cast<std::size_t>(max - min + 1);
    if (k > n) k = n;

    std::vector<int> pool;
    pool.reserve(n);
    for (int i = min; i <= max; ++i) pool.push_back(i);

    std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::shuffle(pool.begin(), pool.end(), rng);

    pool.resize(k);
    return pool;
}

// 生成问题场景特征
struct ProblemScenario {
    int scenario_id;
    int num_targets;
    int num_resources;
    int num_target_types;
    int num_resource_types;
    double urgency_level;
    std::string decision_preference;

    // 添加到JSON
    nlohmann::json to_json() const {
        return {
            {"scenario_id", scenario_id},
            {"num_targets", num_targets},
            {"num_resources", num_resources},
            {"num_target_types", num_target_types},
            {"num_resource_types", num_resource_types},
            {"decision_preference",decision_preference},
            {"urgency_level", urgency_level}
        };
    }
};

// 生成数据集
class DatasetGenerator {
private:
    std::mt19937 rng;
    // 保存原场景的目标信息
    struct TargetInfo {
        std::string target_id;
        std::string batch_id;
        std::string target_position;
        std::string target_type;
        std::string target_type_max_mount;
        std::string target_priority;
        std::string weapon_type;
        std::string weapon_num;
        std::string appearance_time;
        std::string leadtime;
    };
    // 从JSON中提取目标信息
    std::vector<TargetInfo> extract_target_info(const nlohmann::json& scenario_json) {
        std::vector<TargetInfo> targets;

        if (scenario_json.contains("data") &&
            scenario_json["data"].contains("target_list")) {

            for (const auto& target_json : scenario_json["data"]["target_list"]) {
                TargetInfo info;
                info.target_id = target_json["target_id"];
                info.batch_id = target_json["batch_id"];
                info.target_position = target_json["target_position"];
                info.target_type = target_json["target_type"];
                info.target_type_max_mount = target_json["target_type_max_mount"];
                info.target_priority = target_json["target_priority"];
                info.weapon_type = target_json["weapon_type"];
                info.weapon_num = target_json["weapon_num"];
                info.appearance_time = target_json["appearance_time"];
                info.leadtime = target_json["leadtime"];
                targets.push_back(info);
            }
        }

        return targets;
    }
public:
    DatasetGenerator(int seed = 42) : rng(seed) {}

    // 生成80个样本
// 在 DataGenerator2.cpp 中修改 DatasetGenerator::generate_scenarios 方法：

    std::vector<ProblemScenario> generate_scenarios(int n_samples = 200) {
        std::vector<ProblemScenario> scenarios;

        // 修改参数范围，确保目标数量和资源数量保持2:25的比例
        // 目标数量:资源数量 = 2:25 = 1:12.5
        // 当目标数量为20-40时，资源数量应为250-500
        std::vector<std::pair<double, double>> param_ranges = {
            {2, 80},        // num_targets: 20-40
            {20, 800},      // num_resources: 250-500 (按比例调整)
            {2, 10},          // num_target_types
            {2, 10},          // num_resource_types
            {0.1, 1.0}       // urgency_level
        };

        // 使用拉丁超立方采样
        LatinHypercubeSampling lhs(param_ranges.size(), n_samples, 42);
        auto samples = lhs.generate(param_ranges);

        for (int i = 0; i < n_samples; ++i) {
            ProblemScenario scenario;
            scenario.scenario_id = i + 1;

            // 生成目标数量 (20-40)
            scenario.num_targets = static_cast<int>(std::round(samples[i][0]));

            // 随机生成资源与目标的比例
            // 设置随机数引擎
            //std::random_device rd;  // 获取真随机数种子
            std::mt19937 gen(12345); // 使用Mersenne Twister算法

            // 定义分布：生成5.0到12.5之间的浮点数
            std::uniform_real_distribution<double> dis(5.0, 12.5);

            // 生成一个随机数
            double ratio = dis(gen);
            //double ratio = 10; // 1:12.5 比例 (2:25 = 1:12.5)
            int calculated_resources = static_cast<int>(std::round(scenario.num_targets * ratio));

            // 确保资源数量在合理范围内
            int min_resources = param_ranges[1].first;
            int max_resources = param_ranges[1].second;
            if (calculated_resources < min_resources) calculated_resources = min_resources;
            if (calculated_resources > max_resources) calculated_resources = max_resources;
            scenario.num_resources = calculated_resources;


            scenario.num_target_types = static_cast<int>(std::round(samples[i][2]));
            scenario.num_resource_types = static_cast<int>(std::round(samples[i][3]));
            scenario.urgency_level = samples[i][4];
            if (scenario.urgency_level < 0.5) {
                scenario.decision_preference = "cost";
            }
            else {
                scenario.decision_preference = "time";
            }
            ////随机选取20%的样本，将决策偏好设置为“difference”
            //if (i % 5 == 0) {
            //    scenario.decision_preference = "difference";
            //}
            scenarios.push_back(scenario);
        }

        return scenarios;
    }
    // 根据场景生成具体的JSON数据
    nlohmann::json generate_json_data(const ProblemScenario& scenario) {
        std::mt19937 mt(scenario.scenario_id); // 使用场景ID作为随机种子

        // 运力类型
        vector<std::string> transports = { "plane1", "plane2", "vehical1", "vehical2" };

        // 挂载类型
        vector<std::string> mount_types = { "mount1", "mount2,mount3", "mount4", "mount5" };
        vector<std::string> mount_nums = { "30", "50,50", "40", "30" };
        vector<std::string> up_mount_type = { "mount1", "mount2", "mount3", "mount4", "mount5" };

        // 目标类型（根据需求的mount判断）
        vector<std::string> mountload_need = { "mount1#load1", "mount2#load2", "mount3#load3", "mount4#load4", "mount5#load5" };
        vector<std::string> mountnum = { "120", "200", "200", "240", "150" };

        // 均匀分布
        uniform_int_distribution<size_t> transport_dist(0, transports.size() - 1);
        uniform_int_distribution<size_t> mountload_need_dist(0, mountload_need.size() - 1);
        std::uniform_real_distribution<double> lat_dist(39.9093 - 0.13, 39.9093 + 0.13);
        std::uniform_real_distribution<> lon_dist(116.3974 - 0.09, 116.3974 + 0.09);
        std::uniform_int_distribution<int> int_dist(10, 40);
        std::uniform_int_distribution<int> bigger_int_dist(40, 80);
        std::uniform_int_distribution<int> ten_dist(1, 10);
        std::uniform_real_distribution<double> double_dist(0.0, 1.0);

        nlohmann::json j;
        nlohmann::json data;

        // 生成基地数据
        nlohmann::json base_json;
        base_json["base_name"] = "base1";
        base_json["base_position"] = std::to_string(lon_dist(mt)) + "," + std::to_string(lat_dist(mt));
        base_json["base_deptimeinterval"] = std::to_string(double_dist(mt));
        data["base_data"] = base_json;

        // 生成运力资源数据
        for (int i = 0; i < scenario.num_resources; ++i) {
            int idx = transport_dist(mt);
            nlohmann::json force_json;
            force_json["force_id"] = "force_" + std::to_string(i);
            force_json["force_position"] = std::to_string(lon_dist(mt)) + "," + std::to_string(lat_dist(mt));
            force_json["force_type"] = transports[idx];
            force_json["weapon_type"] = mount_types[idx];
            force_json["weapon_num"] = mount_nums[idx];
            force_json["max_speed"] = std::to_string(bigger_int_dist(mt));
            force_json["min_speed"] = std::to_string(int_dist(mt));
            force_json["max_distance"] = std::to_string(int_dist(mt));
            force_json["setup_cost"] = std::to_string(double_dist(mt));
            force_json["fuel_cost"] = std::to_string(double_dist(mt));
            force_json["preparation_time"] = std::to_string(int_dist(mt));
            data["forces_list"].push_back(force_json);
        }

        // 生成目标数据
        map<string, string> target_type_max_mount;
        for (int i = 1; i <= scenario.num_target_types; ++i) {
            target_type_max_mount["target_type" + std::to_string(i)] = "3";
        }

        // 生成批次
        int targets_per_batch = scenario.num_targets;
        for (int b = 0; b < 1; b++) {
            string batch_id = "batch" + std::to_string(b);
            for (int i = 0; i < targets_per_batch; ++i) {
                int idx = mountload_need_dist(mt);
                string mount_type = mountload_need[idx];

                nlohmann::json target_json;
                target_json["batch_id"] = batch_id;
                target_json["target_id"] = "target" + std::to_string(b * 100 + i);
                target_json["target_position"] = std::to_string(lon_dist(mt)) + "," + std::to_string(lat_dist(mt));
                string target_type_string = "target_type" + std::to_string(ten_dist(mt) % scenario.num_target_types + 1);
                target_json["target_type"] = target_type_string;
                target_json["target_type_max_mount"] = target_type_max_mount[target_type_string];
                target_json["target_priority"] = std::to_string(double_dist(mt));
                target_json["weapon_type"] = mount_type;
                target_json["weapon_num"] = mountnum[idx];
                target_json["appearance_time"] = "2025-01-01 00:00:00";
                target_json["leadtime"] = std::to_string(int_dist(mt));

                // 根据紧急程度调整时间约束
                if (scenario.urgency_level > 0.7) {
                    target_json["leadtime"] = std::to_string(int_dist(mt) / 2); // 更紧的时间约束
                }

                data["target_list"].push_back(target_json);
            }
        }

        // 添加其他数据
        data["warhead_cost"] = { {"load_type", "0.0"} };
        data["asg_parameter"] = {
            {"grouproundnumlimit", "1"},
            {"grouppertargetnumlimit", "3"},
            {"targetpergroupnumlimit", "5"},
            {"maxthreadnumlimit", "0"}
        };
        data["muilti_plan_parameter"] = {
            {"plan_name", "1"},
            {"plan_type", "economic_first"},
            {"status", "true"},
            {"rate", "1"},
            {"costoil", "0"},
            {"costuse", "0"},
            {"costfair", "0"},
            {"costfailure", "0"}
        };
        data["trainning_frequency"] = {};

        // 生成挂载数据
        for (int i = 1; i <= 10; ++i) {
            nlohmann::json mount_json;
            mount_json["mount_name"] = "mount" + std::to_string(i);
            mount_json["required_time"] = "300";
            mount_json["mount_cost"] = "1600";
            mount_json["range"] = "640";
            mount_json["is_occupypied"] = "0";
            mount_json["stock_num"] = "10000";
            int mount_type_index = pick_unique(0, up_mount_type.size() - 1, 1)[0];
            mount_json["mount_type_name"] = up_mount_type[mount_type_index];
            mount_json["load_list"] = "load" + std::to_string(mount_type_index + 1);
            mount_json["test"] = "1";
            data["mount_attribute"].push_back(mount_json);
        }

        // 生成挂载类型数据
        for (int i = 0; i < up_mount_type.size(); ++i) {
            nlohmann::json mount_type_json;
            mount_type_json["mount_type_name"] = up_mount_type[i];
            mount_type_json["interval_time"] = "40";
            mount_type_json["grp_num_ub"] = "2";
            data["mount_type"].push_back(mount_type_json);
        }

        j["data"] = data;
        j["scenario_features"] = scenario.to_json();

        return j;
    }

    // 生成所有数据文件
    void generate_all_data(const std::string& output_dir = "../data/scenarios/") {
        // 创建目录
        fs::create_directories(output_dir);

        // 生成场景
        auto scenarios = generate_scenarios(200);

        // 保存场景特征
        nlohmann::json scenarios_json;
        for (const auto& scenario : scenarios) {
            scenarios_json.push_back(scenario.to_json());
        }

        std::ofstream features_file(output_dir + "scenario_features.json");
        features_file << scenarios_json.dump(4);
        features_file.close();
        cout << "场景特征已保存到: " << output_dir << "scenario_features.json" << endl;

        // 生成并保存每个场景的数据
        for (const auto& scenario : scenarios) {
            auto json_data = generate_json_data(scenario);

            std::string filename = output_dir + "scenario_" +
                std::to_string(scenario.scenario_id) +
                "_T" + std::to_string(scenario.num_targets) +
                "_R" + std::to_string(scenario.num_resources) +
                ".json";

            std::ofstream file(filename);
            if (file.is_open()) {
                file << json_data.dump(4);
                file.close();
                cout << "生成场景 " << scenario.scenario_id << ": " << filename << endl;
            }
        }

        cout << "共生成 " << scenarios.size() << " 个数据文件" << endl;
    }
    // 从规划结果文件读取历史指标
    nlohmann::json read_planning_result_metrics(const std::string& scenario_id_str) {
        nlohmann::json metrics;

        // 尝试查找对应的规划结果文件
        std::string result_dir = "../modelsForData/results/";

        // 检查目录是否存在
        if (!fs::exists(result_dir)) {
            std::cout << "结果目录不存在: " << result_dir << std::endl;
            return metrics; // 返回空JSON
        }

        bool found_file = false;
        for (const auto& entry : fs::directory_iterator(result_dir)) {
            if (fs::is_regular_file(entry) &&
                entry.path().extension() == ".json" &&
                entry.path().filename().string().find("scenario_"+scenario_id_str) != std::string::npos) {

                found_file = true;
                try {
                    std::ifstream file(entry.path());
                    nlohmann::json result_json;
                    file >> result_json;
                    file.close();

                    // 从规划结果中提取指标
                    if (result_json.contains("result") &&
                        result_json["result"].contains("statistics_parameter")) {

                        const auto& stats = result_json["result"]["statistics_parameter"];

                        // 提取总成本
                        for (const auto& stat : stats) {
                            if (stat.contains("TotalCost")) {
                                metrics["total_cost"] = std::stod(stat["TotalCost"].get<std::string>());
                            }
                        }

                        // 提取总时间
                        for (const auto& stat : stats) {
                            if (stat.contains("CmpTime")) {
                                metrics["total_time"] = std::stod(stat["CmpTime"].get<std::string>());
                            }
                        }

                        // 提取目标覆盖率
                        for (const auto& stat : stats) {
                            if (stat.contains("TargetCoverRate")) {
                                std::string cover_rate_str = stat["TargetCoverRate"].get<std::string>();
                                metrics["target_coverage"] = std::stod(cover_rate_str);
                            }
                        }

                        // 提取资源利用率
                        for (const auto& stat : stats) {
                            if (stat.contains("ResourceUseRate")) {
                                std::string use_rate_str = stat["ResourceUseRate"].get<std::string>();
                                metrics["resource_utilization"] = std::stod(use_rate_str);
                            }
                        }

                        // 如果有未完成目标，调整覆盖率
                        if (result_json["result"].contains("unfinished_target") &&
                            !result_json["result"]["unfinished_target"].is_null()) {

                            std::string unfinished = result_json["result"]["unfinished_target"];
                            if (!unfinished.empty()) {
                                // 假设每个目标同等重要，有未完成目标会降低覆盖率
                                double current_coverage = metrics.value("target_coverage", 0.0);
                                metrics["target_coverage"] = current_coverage * 0.9; // 简单惩罚
                            }
                        }

                        std::cout << "从文件 " << entry.path().filename()
                            << " 读取历史规划指标" << std::endl;
                        return metrics;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "读取规划结果文件 " << entry.path()
                        << " 时出错: " << e.what() << std::endl;
                    continue;
                }
            }
        }

        // 如果找不到文件，返回空JSON对象
        if (!found_file) {
            std::cout << "未找到场景 " << scenario_id_str << " 的规划结果，跳过生成二次规划" << std::endl;
        }
        else {
            std::cout << "场景 " << scenario_id_str << " 的规划结果文件存在但无法读取，跳过生成二次规划" << std::endl;
        }

        return nlohmann::json(); // 返回空JSON
    }

    // 生成二次规划场景
    nlohmann::json generate_secondary_scenario(
        const nlohmann::json& primary_scenario,
        int scenario_id,
        float target_change_ratio = 0.0f) {  // -0.5 到 1.0 表示减少50%到增加100%

        // 先读取历史规划指标
        std::string primary_scenario_id = to_string(
            primary_scenario["scenario_features"]["scenario_id"]);

        nlohmann::json historical_metrics = read_planning_result_metrics(primary_scenario_id);

        // 如果历史指标为空（表示找不到规划结果文件），返回空JSON
        if (historical_metrics.empty()) {
            return nlohmann::json();
        }

        // 复制原场景
        nlohmann::json secondary = primary_scenario;

        // 提取原场景的目标信息
        std::vector<TargetInfo> primary_targets = extract_target_info(primary_scenario);

        if (primary_targets.empty()) {
            return nlohmann::json();  // 没有目标，返回空JSON
        }

        // 随机确定目标数量变化（-50% 到 +100%）
        //std::random_device rd;
        std::mt19937 gen(1234);  // 使用固定的种子
        std::uniform_real_distribution<float> ratio_dist(-0.5f, 1.0f);

        if (target_change_ratio == 0.0f) {
            target_change_ratio = ratio_dist(gen);
        }

        // 计算新的目标数量
        int original_count = primary_targets.size();
        int new_count = static_cast<int>(original_count * (1.0f + target_change_ratio));

        // 确保至少有一个目标
        new_count = std::max(1, new_count);
        // 确保不超过原目标的2倍
        new_count = std::min(original_count * 2, new_count);

        std::cout << "生成二次规划: 原目标数=" << original_count
            << ", 变化比例=" << target_change_ratio
            << ", 新目标数=" << new_count << std::endl;

        // 清空原目标列表
        secondary["data"]["target_list"] = nlohmann::json::array();

        // 决定哪些目标保留（增加、减少或保持不变）
        if (new_count <= original_count) {
            // 减少目标数量
            // 随机选择要保留的目标
            std::vector<int> indices(original_count);
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), gen);

            for (int i = 0; i < new_count; ++i) {
                int idx = indices[i];
                nlohmann::json target_json;
                target_json["batch_id"] = primary_targets[idx].batch_id;
                target_json["target_id"] = primary_targets[idx].target_id;
                target_json["target_position"] = primary_targets[idx].target_position;
                target_json["target_type"] = primary_targets[idx].target_type;
                target_json["target_type_max_mount"] = primary_targets[idx].target_type_max_mount;
                target_json["target_priority"] = primary_targets[idx].target_priority;
                target_json["weapon_type"] = primary_targets[idx].weapon_type;
                target_json["weapon_num"] = primary_targets[idx].weapon_num;
                target_json["appearance_time"] = primary_targets[idx].appearance_time;
                target_json["leadtime"] = primary_targets[idx].leadtime;

                secondary["data"]["target_list"].push_back(target_json);
            }
        }
        else {
            // 增加目标数量
            // 首先保留所有原目标
            for (const auto& target : primary_targets) {
                nlohmann::json target_json;
                target_json["batch_id"] = target.batch_id;
                target_json["target_id"] = target.target_id;
                target_json["target_position"] = target.target_position;
                target_json["target_type"] = target.target_type;
                target_json["target_type_max_mount"] = target.target_type_max_mount;
                target_json["target_priority"] = target.target_priority;
                target_json["weapon_type"] = target.weapon_type;
                target_json["weapon_num"] = target.weapon_num;
                target_json["appearance_time"] = target.appearance_time;
                target_json["leadtime"] = target.leadtime;

                secondary["data"]["target_list"].push_back(target_json);
            }

            // 然后添加新目标（复制原有目标并修改ID）
            int targets_to_add = new_count - original_count;
            std::uniform_int_distribution<int> idx_dist(0, original_count - 1);

            // 用于生成唯一ID
            int id_counter = 1000;  // 从1000开始避免冲突

            for (int i = 0; i < targets_to_add; ++i) {
                // 随机选择一个原目标作为模板
                int template_idx = idx_dist(gen);
                const TargetInfo& template_target = primary_targets[template_idx];

                // 生成新ID（在原ID后添加"_newN"后缀）
                std::string new_id = template_target.target_id + "_new" + std::to_string(id_counter++);

                // 稍微修改位置（在原位置附近）
                std::string position = template_target.target_position;
                size_t comma_pos = position.find(',');
                if (comma_pos != std::string::npos) {
                    double lon = std::stod(position.substr(0, comma_pos));
                    double lat = std::stod(position.substr(comma_pos + 1));

                    // 在±0.01度范围内随机偏移
                    std::uniform_real_distribution<double> offset_dist(-0.01, 0.01);
                    lon += offset_dist(gen);
                    lat += offset_dist(gen);

                    position = std::to_string(lon) + "," + std::to_string(lat);
                }

                // 稍微修改时间窗口
                std::string leadtime = template_target.leadtime;
                if (!leadtime.empty()) {
                    int original_lead = std::stoi(leadtime);
                    std::uniform_int_distribution<int> time_dist(-5, 5);
                    original_lead += time_dist(gen);
                    original_lead = std::max(1, original_lead);  // 确保为正数
                    leadtime = std::to_string(original_lead);
                }

                nlohmann::json new_target_json;
                new_target_json["batch_id"] = template_target.batch_id;
                new_target_json["target_id"] = new_id;
                new_target_json["target_position"] = position;
                new_target_json["target_type"] = template_target.target_type;
                new_target_json["target_type_max_mount"] = template_target.target_type_max_mount;
                new_target_json["target_priority"] = template_target.target_priority;
                new_target_json["weapon_type"] = template_target.weapon_type;
                new_target_json["weapon_num"] = template_target.weapon_num;
                new_target_json["appearance_time"] = template_target.appearance_time;
                new_target_json["leadtime"] = leadtime;

                secondary["data"]["target_list"].push_back(new_target_json);
            }
        }

        // 更新场景特征
        if (secondary.contains("scenario_features")) {
            secondary["scenario_features"]["scenario_id"] = scenario_id;
            secondary["scenario_features"]["num_targets"] = new_count;

            // 标记为二次规划场景
            secondary["scenario_features"]["planning_type"] = "secondary";
            secondary["scenario_features"]["original_scenario_id"] =
                primary_scenario["scenario_features"]["scenario_id"];
            secondary["scenario_features"]["target_change_ratio"] = target_change_ratio;

            // 设置决策偏好为"difference"（最小化差异）
            secondary["scenario_features"]["decision_preference"] = "difference";

            // 增加紧急程度（二次规划通常时间更紧）
            float original_urgency = secondary["scenario_features"]["urgency_level"];
            std::uniform_real_distribution<float> urgency_dist(0.1f, 0.3f);
            secondary["scenario_features"]["urgency_level"] =
                std::min(1.0f, original_urgency + urgency_dist(gen));
        }

        // 添加历史规划信息
        nlohmann::json historical_plan;
        historical_plan["plan_id"] = "plan_" + primary_scenario_id + "_primary";
        historical_plan["total_cost"] = historical_metrics["total_cost"];
        historical_plan["total_time"] = historical_metrics["total_time"];
        historical_plan["resource_utilization"] = historical_metrics["resource_utilization"];
        historical_plan["target_coverage"] = historical_metrics["target_coverage"];

        secondary["historical_plan"] = historical_plan;

        return secondary;
    }

    // 批量生成二次规划场景
    void generate_secondary_scenarios_batch(
        const std::string& primary_scenarios_dir,
        const std::string& output_dir = "../data/secondary_scenarios/",
        int num_secondary_per_primary = 1) {  // 每个原场景生成1个二次规划

        // 创建目录
        fs::create_directories(output_dir);

        // 收集所有原场景文件
        std::vector<fs::path> primary_files;
        for (const auto& entry : fs::directory_iterator(primary_scenarios_dir)) {
            if (entry.path().extension() == ".json" &&
                entry.path().filename().string().find("scenario_") == 0) {
                primary_files.push_back(entry.path());
            }
        }

        int secondary_id = 1000;  // 从1000开始编号
        int generated_count = 0;

        for (const auto& primary_file : primary_files) {
            try {
                // 加载原场景
                std::ifstream file(primary_file.string());
                nlohmann::json primary_scenario;
                file >> primary_scenario;
                file.close();

                // 为每个原场景生成多个二次规划场景
                for (int i = 0; i < num_secondary_per_primary; ++i) {
                    // 使用不同的目标变化比例
                    std::vector<float> change_ratios = { -0.5f, -0.3f, 0.0f, 0.3f, 0.5f, 0.8f, 1.0f };
                    float ratio = change_ratios[i % change_ratios.size()];

                    // 生成二次规划场景
                    auto secondary = generate_secondary_scenario(
                        primary_scenario, secondary_id, ratio);

                    // 如果返回的是空JSON，跳过
                    if (secondary.empty()) {
                        continue;
                    }

                    secondary_id++; // 只有在成功生成时才递增ID
                    generated_count++;

                    // 从原场景文件名提取信息
                    std::string primary_name = primary_file.stem().string();
                    size_t pos = primary_name.find("_T");
                    if (pos != std::string::npos) {
                        primary_name = primary_name.substr(pos);
                    }

                    // 保存二次规划场景
                    int num_targets = secondary["scenario_features"]["num_targets"];
                    int num_resources = secondary["scenario_features"]["num_resources"];

                    std::string filename = output_dir + "secondary_" +
                        std::to_string(secondary_id - 1) + primary_name +
                        "_T" + std::to_string(num_targets) +
                        "_R" + std::to_string(num_resources) + ".json";

                    std::ofstream out_file(filename);
                    if (out_file.is_open()) {
                        out_file << secondary.dump(4);
                        out_file.close();
                        std::cout << "生成二次规划场景: " << filename << std::endl;
                    }
                }

            }
            catch (const std::exception& e) {
                std::cerr << "处理文件 " << primary_file << " 时出错: " << e.what() << std::endl;
                continue;
            }
        }

        std::cout << "共生成 " << generated_count << " 个二次规划场景" << std::endl;
    }
    // 批量生成二次规划场景 - 一对一版本（带随机目标变化）
    void generate_secondary_scenarios_one_to_one_random(
        const std::string& primary_scenarios_dir,
        const std::string& output_dir = "../data/secondary_scenarios/") {

        // 创建目录
        fs::create_directories(output_dir);

        // 收集所有原场景文件
        std::vector<fs::path> primary_files;
        for (const auto& entry : fs::directory_iterator(primary_scenarios_dir)) {
            if (entry.path().extension() == ".json" &&
                entry.path().filename().string().find("scenario_") == 0) {
                primary_files.push_back(entry.path());
            }
        }

        std::cout << "找到 " << primary_files.size() << " 个原场景文件" << std::endl;

        // 设置随机数生成器
        std::random_device rd;
        std::mt19937 gen(23456); // 使用固定种子以获得可重复的结果
        // 目标变化比例分布：-0.5到1.0（-50%到+100%）
        std::uniform_real_distribution<float> ratio_dist(-0.5f, 1.0f);

        // 准备变化比例选项（如果需要特定分布）
        std::vector<float> predefined_ratios = {
            -0.5f, -0.4f, -0.3f, -0.2f, -0.1f,  // 减少
            0.0f,                               // 不变
            0.1f, 0.2f, 0.3f, 0.4f, 0.5f,      // 少量增加
            0.6f, 0.7f, 0.8f, 0.9f, 1.0f       // 大量增加
        };
        std::uniform_int_distribution<size_t> idx_dist(0, predefined_ratios.size() - 1);

        int secondary_id = 1000;
        int generated_count = 0;
        int skipped_count = 0;
        std::map<float, int> ratio_distribution; // 统计各种比例的使用情况

        for (const auto& primary_file : primary_files) {
            try {
                // 加载原场景
                std::ifstream file(primary_file.string());
                nlohmann::json primary_scenario;
                file >> primary_scenario;
                file.close();

                // 提取原场景ID
                int primary_scenario_id = primary_scenario["scenario_features"]["scenario_id"];

                // 检查是否有历史数据
                std::string scenario_id_str = std::to_string(primary_scenario_id);
                nlohmann::json historical_metrics = read_planning_result_metrics(scenario_id_str);

                // 如果没有历史数据，跳过
                if (historical_metrics.empty()) {
                    std::cout << "跳过场景 " << primary_scenario_id
                        << " (无历史数据): " << primary_file.filename() << std::endl;
                    skipped_count++;
                    continue;
                }

                // 生成随机变化比例
                // 方式1: 从预定义比例中随机选择
                float change_ratio = predefined_ratios[idx_dist(gen)];

                // 方式2: 或者使用连续分布
                // float change_ratio = ratio_dist(gen);

                // 统计比例分布
                ratio_distribution[change_ratio]++;

                std::cout << "处理场景 " << primary_scenario_id
                    << ", 使用变化比例: " << (change_ratio * 100) << "%"
                    << " (" << primary_file.filename() << ")" << std::endl;

                // 生成二次规划场景
                auto secondary = generate_secondary_scenario(
                    primary_scenario, secondary_id, change_ratio);

                // 如果生成失败，跳过
                if (secondary.empty()) {
                    std::cout << "警告: 场景 " << primary_scenario_id << " 生成失败" << std::endl;
                    skipped_count++;
                    continue;
                }

                // 获取生成后的实际目标数量
                int original_targets = primary_scenario["scenario_features"]["num_targets"];
                int new_targets = secondary["scenario_features"]["num_targets"];
                float actual_ratio = (new_targets - original_targets) / (float)original_targets;

                secondary_id++;
                generated_count++;

                // 保存文件
                int num_targets = secondary["scenario_features"]["num_targets"];
                int num_resources = secondary["scenario_features"]["num_resources"];

                std::string filename = output_dir + "secondary_" +
                    std::to_string(secondary_id - 1) +
                    "_from_scenario_" + std::to_string(primary_scenario_id) +
                    "_ratio" + std::to_string(static_cast<int>(change_ratio * 100)) +
                    "_T" + std::to_string(num_targets) +
                    "_R" + std::to_string(num_resources) + ".json";

                std::ofstream out_file(filename);
                if (out_file.is_open()) {
                    out_file << secondary.dump(4);
                    out_file.close();
                    std::cout << "  生成: " << filename
                        << " (原目标: " << original_targets
                        << ", 新目标: " << new_targets
                        << ", 实际变化: " << (actual_ratio * 100) << "%)" << std::endl;
                }

            }
            catch (const std::exception& e) {
                std::cerr << "处理文件 " << primary_file << " 时出错: " << e.what() << std::endl;
                continue;
            }
        }

        // 打印统计结果
        std::cout << "\n===== 统计结果 =====" << std::endl;
        std::cout << "原场景总数: " << primary_files.size() << std::endl;
        std::cout << "成功生成二次规划: " << generated_count << std::endl;
        std::cout << "跳过场景（无历史数据）: " << skipped_count << std::endl;

        if (generated_count > 0) {
            std::cout << "\n目标变化比例分布:" << std::endl;
            for (const auto& [ratio, count] : ratio_distribution) {
                std::cout << "  " << (ratio * 100) << "%: " << count << " 个场景" << std::endl;
            }

            std::cout << "\n二次规划ID范围: 1000-" << (secondary_id - 1) << std::endl;
        }

        // 验证生成数量是否匹配历史数据数量
        if (generated_count != 59) {
            std::cout << "\n⚠️  警告: 生成的二次规划数量(" << generated_count
                << ")与历史数据数量(59)不匹配!" << std::endl;
            std::cout << "可能原因:" << std::endl;
            std::cout << "1. 历史数据目录可能存在更多/更少文件" << std::endl;
            std::cout << "2. 原场景与历史数据ID不匹配" << std::endl;
            std::cout << "3. 某些历史数据文件格式不正确" << std::endl;
        }
        else {
            std::cout << "\n✅ 完美匹配: 成功生成了所有59个有历史数据的二次规划场景!" << std::endl;
        }
    }
};

//int main() {
//    DatasetGenerator generator;
//    //generator.generate_all_data();
//    generator.generate_secondary_scenarios_one_to_one_random("../data/test_scenarios/", "../data/secondary_scenarios/");
//    return 0;
//}