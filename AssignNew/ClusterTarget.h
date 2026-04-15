#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <cmath>
#include <random>
#include <nlohmann/json.hpp>  // JSON解析库
#include <nanoflann.hpp>      // KD-tree实现库
// 任务结构体定义
// 表示一个需要执行的任务，包含空间位置、时间信息和所需设备类型
struct Task {
    int id;                   // 任务唯一标识符
    double x, y;              // 任务的空间坐标（经纬度或平面坐标）
    //time_t start, end;        // 任务的时间窗口（开始时间和结束时间）
    std::set<int> mount_types; // 任务所需的挂载类型ID集合（用于指定所需设备类型）
};

uint64_t encode_payload(const std::set<int>& types);
std::vector<std::vector<int>> dbscan(const std::vector<Task>& tasks,
    double eps, int minPts);