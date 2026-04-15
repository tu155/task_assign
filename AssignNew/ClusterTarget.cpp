//#include "ClusterTarget.h"
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
//using namespace std;
using json = nlohmann::json;

// 任务结构体定义
// 表示一个需要执行的任务，包含空间位置、时间信息和所需设备类型
struct Task {
    int id;                   // 任务唯一标识符
    double x, y;              // 任务的空间坐标（经纬度或平面坐标）
    //time_t start, end;        // 任务的时间窗口（开始时间和结束时间）
    std::set<int> mount_types; // 任务所需的挂载类型ID集合（用于指定所需设备类型）
};

// --------------------------
// 编码 payload 为 bitset
// 功能：将一组整数编码成一个64位的无符号整数
// 原理：每个整数对应bits中的一个位，如果整数在集合中，则对应的位被设置为1，否则为0
// 优势：这种编码方式可以有效地将多个整数压缩成一个64位的值，便于存储和计算距离
// 限制：只适用于类型ID在1-64范围内的情况
// --------------------------
inline uint64_t encode_payload(const std::set<int>& types) {
    uint64_t bits = 0;  // 初始化64位全0
    for (int p : types)
        bits |= 1ULL << (p - 1);  // 将第(p-1)位设置为1
    return bits;
}

// --------------------------
// KD-Tree 适配器结构体
// 功能：为nanoflann库提供接口，用于构建和维护KD-tree数据结构
// KD-tree用途：高效地进行空间范围搜索和最近邻查询
// --------------------------
struct PointCloud {
    // 点云数据存储：每个点是一个4维数组 [x, y, time_center, payload_bits]
    // x, y: 空间坐标
    // time_center: 时间中心点 = (start + end) / 2
    // payload_bits: 编码后的挂载类型位图
    std::vector<std::array<double, 3>> pts;

    // 获取点云中点的数量
    inline size_t kdtree_get_point_count() const {
        return pts.size();
    }

    // 获取指定索引点的指定维度值
    // idx: 点的索引
    // dim: 维度(0=x, 1=y, 2=time_center, 3=payload_bits)
    inline double kdtree_get_pt(const size_t idx, int dim) const {
        return pts[idx][dim];
    }

    // 边界框查询函数（此处未实现边界框功能）
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const {
        return false;
    }

    // 类型别名，用于模板编程中引用自身类型
    using self_t = PointCloud;
};

// --------------------------
// DBSCAN 聚类算法实现
// 功能：基于密度进行任务聚类，发现密集的任务群体
// 参数：
//   tasks: 输入的任务集合
//   eps: 邻域半径阈值，用于定义"邻近"的概念
//   minPts: 最小点数阈值，用于定义核心点
// 返回：聚类结果，每个子向量包含同一簇中的任务索引
// --------------------------
inline std::vector<std::vector<int>> dbscan(const std::vector<Task>& tasks,
    double eps, int minPts)
{
    // 步骤1: 构建点云数据，将任务转换为4维空间中的点
    PointCloud cloud;
    for (const auto& t : tasks) {
        //double tc = (t.start + t.end) * 0.5;  // 计算时间中心点
        uint64_t pb = encode_payload(t.mount_types);  // 编码挂载类型为位图
        // 将任务转换为4维点 [x, y, 时间中心, 编码后的挂载类型](已删掉时间)
        //cloud.pts.push_back({ t.x, t.y, tc, static_cast<double>(pb) });
        cloud.pts.push_back({ t.x, t.y, static_cast<double>(pb) });
    }

    // 步骤2: 构建KD-tree索引，用于加速范围搜索
    // 使用L2_Simple_Adaptor（欧氏距离）和4维空间
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<double, PointCloud>,
        PointCloud, 3>;
    KDTree index(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    index.buildIndex();  // 构建KD-tree索引

    // 步骤3: 初始化聚类标签
    // -1: 未访问, -2: 噪声点, >=0: 簇ID
    std::vector<int> labels(tasks.size(), -1);
    int cluster_id = 0;  // 簇计数器

    // 步骤4: DBSCAN核心算法
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (labels[i] != -1) continue;  // 跳过已访问的点

        // 查找当前点的eps邻域内的所有点
        std::vector<nanoflann::ResultItem<uint32_t, double>> ret_matches;
        nanoflann::SearchParameters params;
        const double query_pt[3] = {
            cloud.pts[i][0],  // x坐标
            cloud.pts[i][1],  // y坐标
            cloud.pts[i][2],  // 时间中心-->编码后的挂载类型
            //cloud.pts[i][3]   // 编码后的挂载类型
        };
        // 执行范围搜索，找到所有距离小于eps的点
        index.radiusSearch(query_pt, eps * eps, ret_matches, params);

        // 步骤5: 判断是否为核心点
        if (ret_matches.size() < minPts) {
            labels[i] = -2;  // 标记为噪声点
            continue;
        }

        // 步骤6: 发现新簇，使用广度优先搜索扩展簇
        std::queue<int> q;
        labels[i] = cluster_id;  // 标记为当前簇
        q.push(i);  // 将当前点加入队列

        while (!q.empty()) {
            int cur = q.front(); q.pop();  // 取出队列头部点

            // 查找当前点的邻域
            nanoflann::SearchParameters params2;
            const double qp[3] = {
                cloud.pts[cur][0],
                cloud.pts[cur][1],
                cloud.pts[cur][2],
                //cloud.pts[cur][3]
            };
            std::vector<nanoflann::ResultItem<uint32_t, double>> nn;
            index.radiusSearch(qp, eps * eps, nn, params2);

            // 处理邻域中的点
            for (const auto& nxt : nn) {
                int j = nxt.first;  // 邻域点的索引
                if (labels[j] == -1 || labels[j] == -2) {
                    // 将未访问或噪声点加入当前簇
                    labels[j] = cluster_id;
                    q.push(j);  // 加入队列继续扩展
                }
            }
        }
        ++cluster_id;  // 簇ID递增
    }

    // 步骤7: 整理聚类结果
    std::vector<std::vector<int>> clusters(cluster_id);
    for (size_t i = 0; i < labels.size(); ++i) {
        if (labels[i] >= 0) {  // 只处理属于簇的点（排除噪声点）
            clusters[labels[i]].push_back(i);  // 按簇ID分组
        }
    }

    // 移除空簇（如果有的话）
    clusters.erase(
        std::remove_if(clusters.begin(), clusters.end(),
            [](const auto& c) { return c.empty(); }),
        clusters.end());

    return clusters;
}