#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp> // JSON库，用于处理JSON数据
#include <random>
#include <string>
#include <sstream>

using namespace std;

// 生成随机字符串的函数
static std::string generateRandomString(const std::string& prefix, int length) {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd; // 随机设备，用于生成随机数种子
    std::mt19937 mt(rd()); // Mersenne Twister随机数生成器
    std::uniform_int_distribution<int> dist(0, sizeof(alphanum) - 2); // 均匀分布，用于从字符数组中选择字符

    std::string s = prefix; // 初始化字符串，添加前缀
    for (int i = 0; i < length; ++i) { // 循环生成指定长度的随机字符串
        s += alphanum[dist(mt)]; // 从字符数组中随机选择一个字符并添加到字符串
    }
    return s; // 返回生成的随机字符串
}

// 生成一个在 [min, max] 范围内的随机偶数
static int generate_even_number(int min, int max) {
    // 使用静态变量，避免重复创建
    static std::mt19937 gen(std::random_device{}());

    // 调整边界，确保它们是偶数
    if (min % 2 != 0) ++min;
    if (max % 2 != 0) --max;

    // 如果调整后最小值反而大于最大值，说明范围内没有偶数
    if (min > max) {
        // 可以抛出异常，或者返回一个错误值
        // 这里简单返回最小值
        return min;
    }

    // 创建分布器，在转换后的范围内生成
    std::uniform_int_distribution<int> dist(min / 2, max / 2);

    //“放大”返回偶数
    return dist(gen) * 2;
}

// 从 [min, max] 中不重复地随机取出 k 个数字
static vector<int> pick_unique(int min, int max, std::size_t k)
{
    if (min > max) std::swap(min, max);
    std::size_t n = static_cast<std::size_t>(max - min + 1);
    if (k > n) k = n;                         // 防止越界

    // 1. 生成区间列表
    std::vector<int> pool;
    pool.reserve(n);
    for (int i = min; i <= max; ++i) pool.push_back(i);

    // 2. 随机打乱
    std::mt19937 rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::shuffle(pool.begin(), pool.end(), rng);

    // 3. 取前 k 个
    pool.resize(k);
    return pool;
}

//int main() {
//    //运力类型
//    //vector<std::string> transports = {"plane1", "plane2", "ship1","ship2", "vehical1","vehical2"};
//    vector<std::string> transports = { "plane1", "plane2","vehical1","vehical2" };
//    //挂载类型
//    /*vector<std::string> mount_types = {"mount1", "mount2,mount3","mount4","mount5" };
//    vector<std::string> mount_nums = { "60", "60,56","30","32","36" };*/
//    //vector<std::string> mount_types = { "mount1", "mount2,mount3","mount4","mount5" ,"mount4","mount3" };
//    //vector<std::string> mount_nums = { "30", "50,30","40","30","40","50" };
//    vector<std::string> mount_types = { "mount1", "mount2,mount3","mount4","mount5" };
//    vector<std::string> mount_nums = { "30", "50,50","40","30" };
//    vector<std::string> up_mount_type = { "mount1","mount2", "mount3","mount4","mount5" };
//    //vector<std::string> up_mount_type = { "mount1", "mount2","mount3" };
//    //按照挂载类型+load类型划分目标类型
//    vector<std::string> mountload_need = { "mount1#load1", "mount2#load2","mount3#load3","mount4#load4","mount5#load5" };
//    vector<std::string> mountnum={"120","200","300","240","150"};
//    //vector<std::string> mountload_need = { "mount1#load1", "mount2#load2","mount3#load3" };
//    //vector<std::string> mountnum = { "100","200","300"};
//    std::mt19937 mt(42); // mersenne twister随机数生成器
//
//    uniform_int_distribution<size_t> transport_dist(0, transports.size() - 1);// 均匀分布，用于从transports中选择一个索引
//    uniform_int_distribution<size_t> mountload_need_dist(0, mountload_need.size() - 1);// 均匀分布，用于从mountload_need中选择一个索引
//     //创建均匀分布对象，用于生成纬度范围
//    std::uniform_real_distribution<double> lat_dist(39.9093 - 0.13, 39.9093 + 0.13);
//
//     //创建均匀分布对象，用于生成经度范围
//    std::uniform_real_distribution<> lon_dist(116.3974 - 0.09, 116.3974 + 0.09);
//
//    std::uniform_int_distribution<int> int_dist(20, 40); // 均匀分布，用于生成20到40之间的整数
//    std::uniform_int_distribution<int> bigger_int_dist(40, 60); // 均匀分布，用于生成20到40之间的整数
//    std::uniform_int_distribution<int> ten_dist(1, 10); // 均匀分布，用于生成1到10之间的整数
//    std::uniform_int_distribution<int> two_dist(1, 2); // 均匀分布，用于生成1到2之间的整数
//    std::uniform_real_distribution<double> double_dist(0.0, 1.0); // 均匀分布，用于生成0.0到1.0之间的浮点数
//    //目标类型对应的单目标最大运力数量
//    map<string, string> target_type_max_mount = {{"target_type1", "3"}, {"target_type2", "3"}, {"target_type3", "3"}, {"target_type4", "3"}, {"target_type5", "3"}, {"target_type6", "3"}, {"target_type7", "3"}, {"target_type8", "3"}, {"target_type9", "3"}, {"target_type10", "3"}};
//    nlohmann::json j;
//    nlohmann::json data;
//    //生成基地数据
//    nlohmann::json base_json;
//    base_json["base_name"] = "base1";
//    base_json["base_position"] = std::to_string(lon_dist(mt)) + "," + std::to_string(lat_dist(mt));
//    base_json["base_deptimeinterval"] = std::to_string(double_dist(mt));
//    data["base_data"]=base_json;
//    //vector<string> weapon_types;
//    //vector<string> weapon_nums;
//    //for (int i = 0; i < up_mount_type.size(); ++i) {
//    //    string mount_type;
//    //    string mount_num;
//    //     //生成可挂载的mount总数
//    //    //int weapon_sum = two_dist(mt);
//    //    int weapon_sum = 1;
//    //    if (i == 0){
//    //        int weapon_sum = 2;
//    //        vector<int>pool = pick_unique(1, up_mount_type.size(), weapon_sum);
//    //        for (int k = 0; k < weapon_sum; ++k) {
//    //             //拼接类型
//    //            mount_type += "mount" + std::to_string(pool[k]); // mount1
//    //             //可挂载数量
//    //            mount_num += std::to_string(generate_even_number(30, 40));     // 生成10-100之间的偶数作为挂载数量
//
//    //            if (k < weapon_sum - 1) { // 如果不是最后一个，就加逗号连接
//    //                mount_type += " ,";
//    //                mount_num += ",";
//    //            }
//    //        }
//    //    }
//    //    else {
//    //        mount_type="mount"+std::to_string(i);
//    //        mount_num += std::to_string(generate_even_number(20, 30));
//    //    }
//
//    //    
//    //    weapon_types.push_back(mount_type);
//    //    weapon_nums.push_back(mount_num);
//    //}
//    // 生成运力资源数据
//    for (int i = 0; i < 500; ++i) {
//        int idx = transport_dist(mt);
//        nlohmann::json force_json;
//        force_json["force_id"] = "force_"+std::to_string(i);
//        force_json["force_position"] = std::to_string(lon_dist(mt)) + "," + std::to_string(lat_dist(mt));
//        force_json["force_type"] = transports[idx];
//        force_json["weapon_type"] = mount_types[idx];
//        force_json["weapon_num"] = mount_nums[idx];
//        force_json["max_speed"] = std::to_string(bigger_int_dist(mt));
//        force_json["min_speed"] = std::to_string(int_dist(mt));
//        force_json["max_distance"] = std::to_string(int_dist(mt));
//        force_json["setup_cost"] = std::to_string(double_dist(mt));             //单次启动成本
//        force_json["fuel_cost"] = std::to_string(double_dist(mt));
//        force_json["preparation_time"] = std::to_string(int_dist(mt));          //连续两次使用的间隔准备时间
//        data["forces_list"].push_back(force_json);
//    }
//
//     //生成目标数据
//    //生成批次
//    for (int b = 0; b < 1; b++) {
//        //string batch_id = generaterandomstring("batch_", 20);
//        string batch_id = "batch0";
//        for (int i = 0; i < 41; ++i) {
//            //int weapon_sum = ten_dist(mt);
//            string mount_type;
//            //string mount_num;
//            //vector<int>pool = pick_unique(1, 10, weapon_sum);//随机打乱1-10内的顺序，选择前weapon_num个
//            //vector<int>load_pool=pick_unique(1,10,weapon_sum);
//            //for (int k = 0; k < weapon_sum; ++k) {
//            //    // 拼接类型
//            //    mount_type = mountload_need[mountload_need_dist(mt)]; // "mount1#load1"
//            //    mount_num = std::to_string(generate_even_number(20, 40));     // 生成20-40之间的偶数作为挂载需求数量
//
//            //    if (k < weapon_sum - 1) { // 如果不是最后一个，就加逗号连接
//            //        mount_type += " ,";
//            //        mount_num += ",";
//            //    }
//            //}
//            int idx = mountload_need_dist(mt);
//            mount_type = mountload_need[idx]; // "mount1#load1"
//            //mount_num = std::to_string(generate_even_number(20, 40));     // 生成20-40之间的偶数作为挂载需求数量
//            nlohmann::json target_json;
//            target_json["batch_id"] =batch_id;
//            target_json["target_id"] = "target" + std::to_string(b*10+i);
//            target_json["target_position"] = std::to_string(lon_dist(mt)) + "," + std::to_string(lat_dist(mt));
//            string target_type_string= "target_type" + std::to_string(ten_dist(mt));
//            target_json["target_type"] =target_type_string;
//            target_json["target_type_max_mount"] = target_type_max_mount[target_type_string];
//            target_json["target_priority"] = std::to_string(double_dist(mt));
//            target_json["weapon_type"] = mount_type;
//            target_json["weapon_num"] = mountnum[idx];
//            target_json["appearance_time"] = "2025-01-01 00:00:00";
//            target_json["leadtime"] = std::to_string(int_dist(mt));
//            data["target_list"].push_back(target_json);
//        }
//    }
//    // 添加其他数据
//    data["warhead_cost"] = { {"load_type", "0.0"} };
//    data["asg_parameter"] = {
//        {"grouproundnumlimit", "1"},
//        {"grouppertargetnumlimit", "3"},
//        {"targetpergroupnumlimit", "5"},
//        {"maxthreadnumlimit", "0"}
//    };
//    data["muilti_plan_parameter"] = {
//        {"plan_name", "1"},
//        {"plan_type", "economic_first" },
//        {"status", "true"},
//        {"rate", "1"},
//        {"costoil", "0"},
//        {"costuse", "0"},
//        {"costfair", "0"},
//        {"costfailure", "0"}
//    };
//    data["trainning_frequency"] = {};
//    //生成挂载数据
//    for (int i = 1; i <= 10; ++i) {
//        nlohmann::json mount_json;
//        mount_json["mount_name"] = "mount" + std::to_string(i);
//        mount_json["required_time"] = "300";
//        mount_json["mount_cost"] = "1600";
//        mount_json["range"] = "640";
//        mount_json["is_occupypied"] = "0";
//        mount_json["stock_num"] = "10000";
//        int mount_type_index = pick_unique(0, up_mount_type.size() - 1, 1)[0];
//        mount_json["mount_type_name"] = up_mount_type[mount_type_index];
//        data["mount_attribute"].push_back(mount_json);
//    }
//    //生成挂载类型数据
//    for (int i = 0; i < up_mount_type.size(); ++i) {
//        nlohmann::json mount_type_json;
//        mount_type_json["mount_type_name"] = up_mount_type[i];
//        mount_type_json["interval_time"] = "40";
//        mount_type_json["grp_num_ub"] = "2";
//        data["mount_type"].push_back(mount_type_json);
//    }
//    j["data"] = data;
//    // 将生成的json数据保存到文件
//    //std::ofstream file("../data/datar550t45.json");
//    std::ofstream file("../data/newcometarget1.json");
//    if (file.is_open()) {
//        file << j.dump(4);
//        cout << "data generated successfully." << endl;
//    }
//    else {
//        std::cerr << "failed to open file for writing." << std::endl;
//    }
//    file.close();
//
//    return 0;
//}