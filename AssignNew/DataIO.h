#ifndef DATAIO_H
#define DATAIO_H

#include "Util.h"
#include "Schedule.h"
#include "MountType.h"
#include "ClusterTarget.cpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using std::string;
using json = nlohmann::json;

class DataLoader_Nlohmann {
private:
    //static Schedule* _schedule;
    // 声明+定义一次性完成，每个翻译单元看到的都是同一实体
    Schedule* _schedule;
    string _getUTF8Str(const string& str) {
        return str; // 如果已经是UTF-8，直接返回
    }
    //// 字符串连接函数
    //string join(const vector<string>& strings, const string& delimiter) {
    //    if (strings.empty()) return "";

    //    string result= strings[0];
    //    for (size_t i = 1; i < strings.size(); i++) {
    //        result+= delimiter + strings[i];
    //    }
    //    return result;
    //}
public:
    //初始化
    DataLoader_Nlohmann(Schedule* schedule) :_schedule(schedule) {} //目标构造函数
    ~DataLoader_Nlohmann() {}
    static void _split(string& str, vector<string>& subStrs, const string& c)
    {
        int count = 0;
        for (int i = 0; i < str.size(); i++)
        {
            if (str[i] == '"')
            {
                count++;
            }
            if (str[i] == ',' && count % 2 != 0)
            {
                str[i] = ';';
            }
        }

        subStrs.clear();
        string::size_type pos1, pos2, pos3;
        pos2 = str.find(c);
        pos1 = 0;
        while (string::npos != pos2)
        {
            pos3 = pos2;
            subStrs.push_back(str.substr(pos1, pos2 - pos1));
            pos1 = pos2 + c.size();
            pos2 = str.find(c, pos1);
        }
        if (pos1 != str.length() || pos3 == str.length() - c.size())
        {
            subStrs.push_back(str.substr(pos1));
        }

        count = 0;
        for (int i = 0; i < subStrs.size(); i++)
        {
            for (int j = 0; j < subStrs[i].size(); j++)
            {
                if (subStrs[i][j] == '"')
                {
                    count++;
                }
                if (subStrs[i][j] == ',' && count % 2 != 0)
                {
                    subStrs[i][j] = ',';
                }
            }
        }
    }

    // 时间字符串转换为time_t
    time_t stringToTime(const string& timeStr) {
        std::tm tm = {};
        std::istringstream ss(timeStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            throw std::runtime_error("时间格式解析失败: " + timeStr);
        }
        return mktime(&tm);
    }

    // 解析经纬度字符串
    pair<double, double> parsePosition(const string& positionStr) {
        size_t commaPos = positionStr.find(',');
        if (commaPos == string::npos) {
            return make_pair(0.0, 0.0);
        }

        double longitude = stod(positionStr.substr(0, commaPos));
        double latitude = stod(positionStr.substr(commaPos + 1));
        return make_pair(longitude, latitude);
    }

    // 解析武器类型字符串
    vector<string> parseWeaponTypes(const string& weaponTypeStr) {
        vector<string> weaponTypes;
        stringstream ss(weaponTypeStr);
        string item;

        while (getline(ss, item, ',')) {
            // 去除空格
            item.erase(0, item.find_first_not_of(' '));
            item.erase(item.find_last_not_of(' ') + 1);
            if (!item.empty()) {
                weaponTypes.push_back(item);
            }
        }
        return weaponTypes;
    }

    // 解析武器数量字符串
    vector<int> parseWeaponNums(const string& weaponNumStr) {
        vector<int> weaponNums;
        stringstream ss(weaponNumStr);
        string item;

        while (getline(ss, item, ',')) {
            if (!item.empty()) {
                weaponNums.push_back(stoi(item));
            }
        }
        return weaponNums;
    }

    void loadFromFile(const string& filename) {
        // 重置所有静态计数器
        _schedule->resetStaticCounters();

        vector<Fleet*> fleets;

        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("无法打开文件: " + filename);
        }

        json doc;
        file >> doc;
        //解析base数据
        cout << "****************************Read base*****************************" << endl;
        const auto& baseData = doc["data"]["base_data"];
        string positionStr = baseData["base_position"];
        auto position = parsePosition(positionStr);
        Base* base = new Base(baseData["base_name"].get<string>(), position.first, position.second, time_t(stoi(baseData["base_deptimeinterval"].get<string>())));
        base->print();
        _schedule->setBase(base);
        //解析mount_type数据
        cout << "****************************Read mount_type*****************************" << endl;
        const auto& mountTypeList = doc["data"]["mount_type"];
        int i = 0;
        for (const auto& mountTypeData : mountTypeList) {
            MountType* mountType = new MountType(
                mountTypeData["mount_type_name"].get<string>(), // name
                stoi(mountTypeData["interval_time"].get<string>()), // intervalTime
                stoi(mountTypeData["grp_num_ub"].get<string>())     //最大成组数量
            );
            _schedule->pushMountType(mountType);
            i++;
            mountType->setId(i);
        }
        //输出mounttype
        if (!_schedule) return;
        auto& mtl = _schedule->getMountTypeList(); // 引用，避免拷贝
        for (size_t i = 0; i < mtl.size(); ++i) {
            MountType* mt = mtl[i];
            if (!mt) continue;                // 再防一级空指针
            cout << mt->getId() << " intervalTime: " << mt->intervalTime << " grpNumUb: " << mt->grpNumUb << '\n';
        }
        //vector<MountType*>mountTypeList = _schedule->getMountTypeList();
        //cout << "mountTypeList size= " << mountTypeList.size() << endl;
        //for (int i = 0; i < mountTypeList.size(); i++)
        //{
        //    cout << mountTypeList[i]->getId() << " intervalTime: " << mountTypeList[i]->intervalTime << " grpNumUb: " << mountTypeList[i]->grpNumUb << endl;
        //}
        // 解析 mount_attribute 数组
        cout << "****************************Read mount_attribute*****************************" << endl;
        const auto& mountsList = doc["data"]["mount_attribute"];
        for (const auto& mountData : mountsList) {
            time_t req_time = stringToTime(mountData["required_time"].get<string>());
            Mount* mount = new Mount(
                mountData["mount_name"].get<string>(),              // name
                req_time,                                                                    // reqTime
                stod(mountData["mount_cost"].get<string>()),           // costUse
                stoi(mountData["is_occupypied"].get<string>()),   // isOccupy
                stod(mountData["range"].get<string>()),                // range
                stoi(mountData["stock_num"].get<string>())         // stock_num
                );

            // 解析loadList
            string loadList = mountData["load_list"].get<string>();
            vector<string> loadNames;
            _split(loadList, loadNames, ",");

            for (const string& loadName : loadNames) {
                Load* load = _schedule->getLoad(loadName);
                if (load == NULL)
                {
                    load = new Load(loadName, 0);
                    _schedule->pushLoad(load);
                }
                if (load != NULL)
                {
                    vector<Load*> subLoadList = mount->getLoadList();
                    bool ifFind = false;
                    for (int l = 0; l < subLoadList.size(); l++)
                    {
                        if (subLoadList[l] == load)
                        {
                            ifFind = true;
                        }
                    }
                    if (!ifFind)
                    {
                        mount->pushLoad(load);
                        mount->setLoadCost(load, 0);
                    }
                    MountLoad* mountLoad = _schedule->getMountLoad(mount, load);
                    if (mountLoad == NULL)
                    {
                        mountLoad = new MountLoad(mount, load);
                        //_schedule->pushMountLoad(mountLoad);
                    }

                }
            }
            
            MountType* mountType = _schedule->getMountType(mountData["mount_type_name"].get<string>());
            mount->setMountType(mountType);
            mountType->pushMount(mount);
            _schedule->pushMount(mount);//将挂载加入schedule对象的_mountlist中
        }
        //输出读取的前五行
        for (int i = 0; i < 5; i++) {
            _schedule->getMountList()[i]->print();
            cout << "------------------------" << endl;
        }

        // 解析 forces_list 数组
        cout << "****************************Read forces_list*****************************" << endl;
        const auto& forcesList = doc["data"]["forces_list"];
        for (const auto& force : forcesList) {
            Fleet* fleet = new Fleet(
                force["force_id"].get<string>(),          // name
                stod(force["max_distance"].get<string>()), // range
                (stod(force["min_speed"].get<string>()) + stod(force["max_speed"].get<string>())) / 2, // optSpeed
                make_pair(
                    stod(force["min_speed"].get<string>()),
                    stod(force["max_speed"].get<string>())
                ),                                        // speedRange
                stod(force["fuel_cost"].get<string>()),   // fuel
                stoi(force["preparation_time"].get<string>()), // prepareTime
                stod(force["setup_cost"].get<string>()),  // costUse
                0.1                                       // costSpeed
            );


            //解析运力类型
            string typeStr = force["force_type"];
            bool found = false;
            // 获取或创建fleetType
            FleetType* fleetType = _schedule->getFleetType(typeStr);
            //如果FleetType中不存在这个类型，则新建一个
            if (fleetType == NULL)
            {
                fleetType = new FleetType(typeStr);
                _schedule->pushFleetType(fleetType);
                fleetType->setId(_schedule->getFleetTypeList().size());
                fleetType->pushFleet(fleet);
            }
            else
            {
                found = true;
                fleetType->pushFleet(fleet);
            }
            fleetType->addAftNum(1);
            fleet->setFleetType(fleetType);
            // 解析挂载,形成MountPLan，依次加入到MountPlanList中
            string weaponTypeStr = force["weapon_type"];
            string weaponNumStr = force["weapon_num"];
            vector<string> weaponTypes = parseWeaponTypes(weaponTypeStr);

            vector<int> weaponNums = parseWeaponNums(weaponNumStr);
            for (int j = 0; j < weaponTypes.size(); j++)
            {

                Mount* mount = _schedule->getMount(weaponTypes[j]);//根据挂载名称获取挂载指针
                fleet->pushMountPlan(mount, weaponNums[j]);//将挂载加入fleet

                //解析运力位置
                string positionStr = force["force_position"];
                auto position = parsePosition(positionStr);
                fleet->setPos(position.first, position.second);
            }
            _schedule->pushFleet(fleet);
        }

        //输出读取的前五行
        for (int i = 0; i < 5; i++) {

            _schedule->getFleetList()[i]->print();
            cout << "------------------------" << endl;
        }
        //输出forceType及其数量
        cout << "****************************Read forceType*****************************" << endl;

        for (int i = 0; i < _schedule->getFleetTypeList().size(); i++)
        {
            _schedule->getFleetTypeList()[i]->print();
            vector <Fleet*> fleets = _schedule->getFleetTypeList()[i]->getFleetList();
            fleets[0]->print();

            cout << "------------------------" << endl;
        }

        // 解析 targets_list 数组
        cout << "****************************Read targets_list*****************************" << endl;
        const auto& targetList = doc["data"]["target_list"];
        map<string, int> mount_index;
        //按照目标类型对目标聚类
        map<TargetType*, vector<Task>> taskMap;
        //vector<Task> tasks;
        for (const auto& targetData : targetList) {
            Task task;
            // 解析目标数据
            string targetId = targetData["target_id"];
            string targetTypeStr = targetData["target_type"];
            string target_type_max_mount_str = targetData["target_type_max_mount"];
            string appearanceTime = targetData["appearance_time"];
            string positionStr = targetData["target_position"];
            string priorityStr = targetData["target_priority"];
            string leadtimeStr = targetData["leadtime"];
            string weaponTypeStr = targetData["weapon_type"];
            string weaponNumStr = targetData["weapon_num"];

            // 转换数据类型
            int target_type_max_mount = stoi(target_type_max_mount_str);
            time_t startTime = stringToTime(appearanceTime);
            time_t endTime = startTime + stoi(leadtimeStr) * 3600; // leadtime转换为秒
            //task.start = startTime;
            //task.end = endTime;
            auto position = parsePosition(positionStr);
            task.x = position.first;
            task.y = position.second;
            double priority = stod(priorityStr);
            bool found = false;
            // 获取或创建TargetType
            TargetType* targetType = _schedule->getTargetType(weaponTypeStr + "-" + weaponNumStr);
            //如果targetType中不存在这个类型，则新建一个

            if (targetType == NULL)
            {
                targetType = new TargetType(weaponTypeStr + "-" + weaponNumStr);
                _schedule->pushTargetType(targetType);
                targetType->initTotalNum();
                taskMap[targetType] = {};
            }
            else
            {
                found = true;
                targetType->addTotalNum();
            }
            // 创建Target对象
            Target* target = new Target(
                targetId,           // name
                targetType,         // type
                target_type_max_mount,            // 单目标的运力数量上限
                make_pair(startTime, endTime), // timeRange
                static_cast<int>(priority * 100), // priority (转换为整数)
                position.first,             // longitude
                position.second        // latitude
            );
            task.id = target->getId();
            taskMap[targetType].push_back(task);
            targetType->pushTarget(target);
            //新增target
            _schedule->pushTarget(target);

            vector<string> weaponTypesWithLoad = parseWeaponTypes(weaponTypeStr);
            vector<int> weaponNums = parseWeaponNums(weaponNumStr);

            vector<pair<MountLoad*, int>> mountLoadPlan;
            for (int j = 0; j < weaponTypesWithLoad.size(); j++)
            {
                int mountNum = 0;
                TargetPlan* targetPlan;
                vector<Load* > loadList;
                map<Load*, int> loadPlan;
                map<Load*, double> loadDamageProbablity;
                map<MountLoad*, int> mountLoadPlan;
                vector<string> subStrs;
                _split(weaponTypesWithLoad[j], subStrs, "#");

                Mount* mount = _schedule->getMount(subStrs.front());//根据挂载名称获取挂载指针
                task.mount_types.insert(mount->getMountType()->getId());
                Load* load = _schedule->getLoad(subStrs.back());//根据挂载名称获取挂载指针
                if (mount == NULL)
                {
                    mount = new Mount(subStrs.front(), 0);
                    _schedule->pushMount(mount);
                }

                if (load == NULL)
                {
                    load = new Load(subStrs.back(), 0);
                    _schedule->pushLoad(load);
                }
                if (load != NULL)
                {
                    vector<Load*> subLoadList = mount->getLoadList();
                    bool ifFind = false;
                    for (int l = 0; l < subLoadList.size(); l++)
                    {
                        if (subLoadList[l] == load)
                        {
                            ifFind = true;
                        }
                    }
                    if (!ifFind)
                    {
                        mount->pushLoad(load);
                        mount->setLoadCost(load, 0);
                    }
                    MountLoad* mountLoad = _schedule->getMountLoad(mount, load);
                    if (mountLoad == NULL)
                    {
                        mountLoad = new MountLoad(mount, load);
                        _schedule->pushMountLoad(mountLoad);
                    }
                    mountLoadPlan[mountLoad] = weaponNums[j];
                    loadPlan[load] = weaponNums[j];
                    loadDamageProbablity[load] = 1 / weaponNums[j];
                    mountNum += weaponNums[j];

                    int num = 0;
                    if (weaponTypesWithLoad[j] != "")
                    {
                        num = weaponNums[j];
                    }

                }
                targetPlan = new TargetPlan(mount, loadDamageProbablity);
                targetPlan = new TargetPlan(mount, loadPlan);//构造函数，执行目标的挂载-载荷计划对形成目标计划
                targetPlan->setMountLoadPlan(mountLoadPlan);
                targetPlan->setMountNum(mountNum);

                vector<TargetPlan* > targetPlanList = targetType->getTargetPlanList();
                //如果targetType的目标计划为空，则加入
                if (targetPlanList.size() == 0) {
                    targetType->pushTargetPlan(targetPlan);
                    _schedule->pushTargetPlanList(targetPlan);
                }
                else
                {
                    //如果targetType的目标计划不为空，则判断是否已经存在
                    bool found = false;
                    for (int i = 0; i < targetPlanList.size(); i++)
                    {
                        if (targetPlanList[i]->getName() == targetPlan->getName()) {
                            found = true;
                        }
                    }
                    if (!found) {
                        targetType->pushTargetPlan(targetPlan);
                        _schedule->pushTargetPlanList(targetPlan);
                    }
                }

            }

        }

        //// 输出前5条目标信息
        //for (int i = 0; i < 5; i++) {
        //    _schedule->getTargetList()[i]->print();
        //    cout << "------------------------" << endl;
        //}
        //// 输出目标类型信息
        //for (int i = 0; i < _schedule->getTargetTypeList().size(); i++) {
        //    _schedule->getTargetTypeList()[i]->print();
        //    cout << "------------------------" << endl;
        //}
        /*
        //对目标聚类
        cout<<"-------------开始对目标聚类---------------"<<endl;
        // 打印数据范围
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();

        for (const auto& [targetTypeStar, tasks] : taskMap) {
            for (const auto& task : tasks) {
                min_x = std::min(min_x, task.x);
                max_x = std::max(max_x, task.x);
                min_y = std::min(min_y, task.y);
                max_y = std::max(max_y, task.y);
            }

            std::cout << "X范围: [" << min_x << ", " << max_x << "]" << std::endl;
            std::cout << "Y范围: [" << min_y << ", " << max_y << "]" << std::endl;
            std::cout << "X跨度: " << (max_x - min_x) << std::endl;
            std::cout << "Y跨度: " << (max_y - min_y) << std::endl;

            // 根据数据范围动态计算EPS
            double x_range = max_x - min_x;
            double y_range = max_y - min_y;

            double dynamic_eps = std::sqrt(x_range * x_range + y_range * y_range) * 0.2; // 数据跨度的20%
            double targetMaxDistance = dynamic_eps * 3;//执行目标之间最大距离为数据跨度的60%
            //Util::targetMaxDistance = targetMaxDistance;
            //const double EPS = dynamic_eps;  // 或者手动设置一个更大的值，如50.0, 100.0
            //const int N_TASKS = 100;
            //const double EPS = 15.0;      // 4-D 欧氏距离阈值,//   eps: 邻域半径阈值，用于定义"邻近"的概念
            const int MIN_PTS = 2; //minPts: 最小点数阈值，用于定义核心点
            auto clusters = dbscan(tasks, dynamic_eps, MIN_PTS);

            cout << "-------------目标类型" << targetTypeStar->getName() << "聚类结束,形成了" << clusters.size() << "个任务包--------------- " << endl;
            //    vector<string>strs;
            //    string name = targetTypeStar->getName();
            //    _split(name, strs, "-");
            //    cout<<"数量是" << strs[0] << endl;
            //    int weaponNum =stoi(strs[1]);
            //    //记录任务包的名称、优先级、资源需求总量、任务包中的任务列表（任务包->主任务->任务列表）
            //    for (int i = 0; i < clusters.size(); i++) {
            //        vector<int> cluster = clusters[i];
            //        vector<Target*> targets;//任务包中的任务列表
            //        string name = "cluster" + to_string(i)+"of"+targetTypeStar->getName();//任务包名称
            //        int sum_priority = 0;//任务包优先级
            //        int clusterMountNum = 0;//任务包资源需求总量
            //        for (int j = 0; j < cluster.size(); j++) {
            //            int taskIndex = cluster[j];
            //            Target* target = _schedule->getTargetById(taskIndex);
            //            sum_priority += target->getPriority();
            //            targets.push_back(target);
            //            clusterMountNum += weaponNum;
            //        }
            //        //计算聚类后任务包的优先级,并添加主目标
            //        int priority = std::round(sum_priority / cluster.size());
            //        MainTarget* mainTarget = new MainTarget(name, priority, targets);
            //        _schedule->pushMainTarget(mainTarget);
        }
        //        //将聚类后的任务包标记为一个目标类型
        //        TargetType* targetType = new TargetType(strs[0] + "-" + to_string(clusterMountNum));
        //        _schedule->pushTargetType(targetType);
        //        targetType->initTotalNum();
        //
        //        //将聚类后的任务包形成新目标
        //        Target* target = new Target(
        //            name,           // name
        //            targetType,         // type
        //            static_cast<int>(priority * 100) // priority (转换为整数)
        //        );
        //        target->setMainTarget(mainTarget);
        //        _schedule->pushTarget(target);
        //        //for (int j = 0; j < cluster.size(); j++) {
        //        //    int taskIndex = cluster[j];
        //        //    Target* target = _schedule->getTargetById(taskIndex);
        //        //    target->setMainTarget(mainTarget);
        //        //    targetType->pushTarget(target);
        //        //}
        //        //删除_schedule中任务列表中属于任务包中的任务，由聚合后的任务替换。
        //        vector<Target*> targetsList=_schedule->getTargetList();
        //        for (int j = 0; j < cluster.size(); j++) {
        //            int taskIndex = cluster[j];
        //            Target* target = _schedule->getTargetById(taskIndex);
        //            targetsList.erase(std::remove(targetsList.begin(), targetsList.end(), target), targetsList.end());
        //        }
        //    }
        //}
        ////输出聚类形成的目标组
        //for (int i = 0; i < _schedule->getMainTargetList().size(); i++) {
        //    _schedule->getMainTargetList()[i]->print();
        //    cout << "------------------------" << endl;
        //}
        */
        //如果目标没有分配到目标组，就更新目标组为这个目标本身
        for (int i = 0; i < _schedule->getTargetList().size(); i++) {
            Target* target = _schedule->getTargetList()[i];
            if (target->getMainTarget() == nullptr) {
                MainTarget* mainTarget = new MainTarget(target->getName(), target->getPriority(), { target });
                _schedule->pushMainTarget(mainTarget);
                target->setMainTarget(mainTarget);
            }
        }

    }
    map<string, float> loadFeaturesFromFile(const string& filename) {

        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("无法打开文件: " + filename);
        }

        json doc;
        file >> doc;
        //解析特征数据
        cout << "****************************Read features*****************************" << endl;
        const auto& Data = doc["scenario_features"];
        map<string, float> features;
        map<string, float> decision_preferance_map;
        decision_preferance_map["time"] = 0.0;
        decision_preferance_map["cost"] = 1.0;
        decision_preferance_map["difference"] = 2.0;
        features["decision_preferance"] = decision_preferance_map[Data["decision_preference"]];
        features["num_resource_types"] = Data["num_resource_types"];
        features["num_resources"] = Data["num_resources"];
        features["num_target_types"] = Data["num_target_types"];
        features["num_targets"] = Data["num_targets"];
        features["urgency_level"]= Data["urgency_level"];
        return features;
    }
    string writeJsonOutput(vector<Tour*> tourList,float calculateTime)
    {
        cout << "SolnTour num is " << tourList.size() << endl;
        Base* base = _schedule->getBase();
        // 更新资源使用情况
        for (auto tour : tourList) {
            //更新基地运力使用情况
            base->pushUsedFleetMap(tour->getFleet(), 1);
            //更新目标匹配的飞机
            for (auto operTarget : tour->getOperTargetList()) {
                Target* target = operTarget->getTarget();
                if (target->getName() == "target2") {
                    cout << "" << endl;
                }
                target->pushResourse(tour, operTarget->getMountLoadPlan());
                //更新基地资源使用情况
                for (auto& mlPair : operTarget->getMountLoadPlan()) {
                    base->pushUsedMountMap(mlPair.first->_mount, mlPair.second);
                    base->pushUsedLoadMap(mlPair.first->_load, mlPair.second);
                }
            }
            //计算攻击时间
            tour->computeTimeMap();
            auto timeMap = tour->getTargetTimeMap();
            for (auto it = timeMap.begin(); it != timeMap.end(); ++it) {
                Target* target = it->first;
                double completeTime = it->second;//飞机执行目标的时间点
                target->pushAsgTime(tour->getFleet(), completeTime);
                if (target->getName() == "target2") {
                    cout << "" << endl;
                }
                double orignalTime = target->getEarliestCmpTime();
                double earliestCmpTime = target->getEarliestCmpTime();
                //目标的最晚完成时间
                if (earliestCmpTime == 0) {
                    target->setEarliestCmpTime(completeTime);
                }
                else if (completeTime > earliestCmpTime) {
                    target->setEarliestCmpTime(completeTime);
                }
            }
        }

        // 创建主JSON对象
        nlohmann::ordered_json document;
        nlohmann::ordered_json dateObj;

        // 攻击计划部分
        vector<Target*> coverTargetList = _schedule->getCoverTList();
        nlohmann::ordered_json attack_plan = json::array();
        double maxCmpTime = 0;
        for (auto target : coverTargetList) {
            auto asgResourse = target->getAsgResourse();

            vector<string> airportStrs, fleetStrs, fleetNumStrs, mountLoadStrs, mountLoadNumStrs;
            int count = 0;

            // 攻击频率点
            nlohmann::ordered_json attackPointList = json::array();

            // 排序攻击时间
            vector<pair<Fleet*, double>> sortedTime = target->getSortedByValue();
            if (sortedTime.back().second > maxCmpTime) {
                maxCmpTime = sortedTime.back().second;
            }
            for (auto& [fleet, time] : sortedTime) {
                auto& [fleetNum, mountLoadMap] = asgResourse[fleet];// fleetNum: 飞机数量, mountLoadMap: 挂载信息，用于拆解fleetData
                for (auto& [mountLoad, num] : mountLoadMap) {
                    //mlNames.push_back(mountLoad->_mount->getName() + "#" + mountLoad->_load->getName());
                    //mlNums.push_back(std::to_string(num));


                    //mountLoadStrs.push_back(join(mlNames, "+"));
                    //mountLoadNumStrs.push_back(join(mlNums, "+"));
                    count++;
                    nlohmann::ordered_json attackPoint;
                    attackPoint["fleet"] = fleet->getName();
                    attackPoint["force_type"] = fleet->getFleetType()->getName();
                    attackPoint["attack_time"] = std::to_string(time);
                    attackPoint["weapon_type"] = _getUTF8Str(mountLoad->_mount->getName() + "#" + mountLoad->_load->getName());
                    attackPoint["weapon_num"] = std::to_string(num);
                    //attackPoint["force_cost"] = std::to_string(target->getAirforceCost());
                    //attackPoint["oil_cost"] = std::to_string(target->getOilCost());
                    attackPointList.push_back(attackPoint);
                }
            }

            nlohmann::ordered_json attacktargetObj;
            attacktargetObj["target_name"] = _getUTF8Str(target->getName());
            //auto position = target->getPosition();
            //attacktargetObj["position"] = {
            //    {"x", position.first},
            //    {"y", position.second}
            //};
            attacktargetObj["targetType"] = _getUTF8Str(target->getType()->getName());
            attacktargetObj["attackPlanNum"] = std::to_string(count);
            attacktargetObj["attackTargetPlan"] = attackPointList;

            attack_plan.push_back(attacktargetObj);
        }
        dateObj["attack_plan"] = attack_plan;

        // 已使用空军资源部分
        nlohmann::ordered_json used_airforce = json::array();
        //auto baseList = _schedule->getBaseList();
        auto fleetList = _schedule->getFleetList();
        auto mountList = _schedule->getMountList();
        auto loadList = _schedule->getLoadList();

        nlohmann::ordered_json baseObj;
        baseObj["airport"] = _getUTF8Str(base->getName());

        // 飞机类型和数量
        vector<string> fleetNames, fleetNums, fleetTypes;
        for (auto fleet : fleetList) {
            int num = base->getUsedFleetNum(fleet);
            if (num > 0) {
                fleetNames.push_back(fleet->getName());
                fleetNums.push_back(std::to_string(num));
                fleetTypes.push_back(fleet->getFleetType()->getName());
            }
        }
        baseObj["used_plane_name"] = _getUTF8Str(join(fleetNames, ","));
        baseObj["used_plane_num"] = join(fleetNums, ",");
        baseObj["used_plane_type"] = join(fleetTypes, ",");

        // 武器类型和数量
        vector<string> mountNames, mountNums;
        for (auto mount : mountList) {
            int num = base->getUsedMountNum(mount);
            if (num > 0) {
                mountNames.push_back(mount->getName());
                mountNums.push_back(std::to_string(num));
            }
        }
        baseObj["used_weapon_type"] = _getUTF8Str(join(mountNames, ","));
        baseObj["used_weapon_num"] = join(mountNums, ",");

        // 弹头类型和数量
        vector<string> loadNames, loadNums;
        for (auto load : loadList) {
            int num = base->getUsedLoadNum(load);
            if (num > 0) {
                loadNames.push_back(load->getName());
                loadNums.push_back(std::to_string(num));
            }
        }
        baseObj["used_warhead_type"] = _getUTF8Str(join(loadNames, ","));
        baseObj["used_warhead_num"] = join(loadNums, ",");

        used_airforce.push_back(baseObj);
        dateObj["used_airforce"] = used_airforce;

        // 资源分配计划部分
        nlohmann::ordered_json assign_plan = json::array();
        double totalTourCost = 0;
        for (auto tour : tourList) {
            nlohmann::ordered_json tourObj;
            //tourObj["grp_id"] = Util::getUUID();
            //tourObj["batch_id"] = _getUTF8Str(tour->getOperTargetList()[0]->getTarget()->getMainTarget()->getBatchId());
            //tourObj["airport"] = _getUTF8Str(tour->getBase()->getName());
            tourObj["force_name"] = _getUTF8Str(tour->getFleet()->getName());
            tourObj["force_type"] = _getUTF8Str(tour->getFleet()->getFleetType()->getName());
            vector<string> mountStrs, mountNumStrs;
            for (auto [mount, num] : tour->getFleet()->getMountPlanList()) {
                mountStrs.push_back(mount->getName());
                mountNumStrs.push_back(std::to_string(num));
            }
            tourObj["mount"] = _getUTF8Str(join(mountStrs, ","));
            tourObj["capacity"] = _getUTF8Str(join(mountNumStrs, ","));
            //tourObj["plane_num"] = tour->getAftNum();
            tourObj["total_time"] = std::to_string(tour->getTotalTime());
            tourObj["total_cost"] = std::to_string(tour->getCost());
            totalTourCost += tour->getCost();
            // 编组类型
            //string tourTypeName;
            //switch (tour->getTourType()) {
            //case BOTHFIX_TOUR: tourTypeName = "2"; break;
            //case BOTHFIX_HALF_TOUR:
            //case BOTHFIX_CP_TOUR: tourTypeName = "3"; break;
            //case GROUPFIX_TOUR: tourTypeName = "4"; break;
            //default: tourTypeName = "1"; break;
            //}
            //tourObj["grp_type"] = tourTypeName;
            map<Target*, double>targetTimeMap = tour->getTargetTimeMap();
            // 目标列表
            nlohmann::ordered_json targets = json::array();
            for (auto operTarget : tour->getOperTargetList()) {
                nlohmann::ordered_json targetObj;
                Target* target = operTarget->getTarget();

                targetObj["target_name"] = _getUTF8Str(target->getName());
                targetObj["target_distance"] = std::to_string(tour->getTargetDst(operTarget));
                targetObj["arrival_time"] = std::to_string(targetTimeMap[target]);
                if (operTarget->getMountPlanList().size() == 1) {
                    targetObj["weapon_type"] = _getUTF8Str(operTarget->getMountPlanList().begin()->first->getName());
                    targetObj["weapon_num"] = std::to_string(operTarget->getMountPlanList().begin()->second);
                }
                /*json attackPoints = json::array();
                for (auto& [mount, num] : operTarget->getMountPlanList()){
                    json attackPoint;
                    attackPoint["weapon_type"] = _getUTF8Str(mount->getName());
                    attackPoint["weapon_num"] = std::to_string(num);

                    attackPoints.push_back(attackPoint);
                }

                targetObj["assign_plan"] = attackPoints;*/
                targets.push_back(targetObj);
            }

            tourObj["targetsList"] = targets;
            assign_plan.push_back(tourObj);
        }

        dateObj["assign_plan"] = assign_plan;

        // 未完成目标
        auto uncvrTList = _schedule->getUncoverTList();
        vector<string> uncvrNames;
        for (auto target : uncvrTList) {
            uncvrNames.push_back(target->getName());
        }
        dateObj["unfinished_target"] = _getUTF8Str(join(uncvrNames, ","));

        // 统计参数
        nlohmann::ordered_json statistics_parameter = json::array();

        // 总成本和最大完成时间
        statistics_parameter.push_back({ {"TotalCost", std::to_string(totalTourCost)} });
        statistics_parameter.push_back({ {"CmpTime", std::to_string(maxCmpTime)} });
        // 目标覆盖率
        double targetCoverRate = (double)coverTargetList.size() / (double)_schedule->getTargetList().size();
        statistics_parameter.push_back({ {"TargetCoverRate", std::to_string(targetCoverRate)} });
        //资源的总使用次数
        statistics_parameter.push_back({ {"TourNum", std::to_string(tourList.size())} });
        //资源使用率
        double resourceUseRate = (double)tourList.size() / (double)_schedule->getFleetList().size();
        statistics_parameter.push_back({ {"ResourceUseRate", std::to_string(resourceUseRate)} });
        //计算时间
        statistics_parameter.push_back({ {"CalculateTime", std::to_string(calculateTime)} });

        // 重要目标覆盖率
        //string impTargetCoverRateStr;

        //statistics_parameter.push_back({ {"ImpTargetCoverRate", _getUTF8Str(impTargetCoverRateStr)} });

        // 其他统计参数（训练次数、同基地机队数量等）


        dateObj["statistics_parameter"] = statistics_parameter;

        // 添加到主文档
        document["result"] = dateObj;

        // 返回格式化JSON字符串
        return document.dump(4); // 4空格缩进
    }

    void loadPlan(const string& filename) {
        //读取已有计划
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("无法打开文件: " + filename);
        }

        json doc;
        file >> doc;
        //解析base数据
        cout << "****************************Read Plan*****************************" << endl;
        const auto& planData = doc["result"]["attack_plan"];

        for (const auto& targetPlanData : planData) {
            string targetName = targetPlanData["target_name"].get<string>();
            Target* target = _schedule->getTarget(targetName);
            //有新增目标，如果目标为空，则跳过
            if (target == nullptr) {
                cout << "目标不存在，跳过" << endl;
                continue;
            }
            double originalTime = 0;
            for (const auto& tourData : targetPlanData["attackTargetPlan"]) {
                double attackTime = stod(tourData["attack_time"].get<string>());
                if (attackTime > originalTime) {
                    originalTime = attackTime;
                }
            }
            target->setLastPlanCmpTime(originalTime);
        }
    }

    // 辅助函数：连接字符串
    string join(const vector<string>& strings, const string& delimiter) {
        if (strings.empty()) return "";

        string result = strings[0];
        for (size_t i = 1; i < strings.size(); i++) {
            result += delimiter + strings[i];
        }
        return result;
    }

    // 辅助函数：连接整数向量
    string join(const vector<int>& integers, const string& delimiter) {
        if (integers.empty()) return "";

        string result = std::to_string(integers[0]);
        for (size_t i = 1; i < integers.size(); i++) {
            result += delimiter + std::to_string(integers[i]);
        }
        return result;
    }
};
#endif