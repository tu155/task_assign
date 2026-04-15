#ifndef TSP_SOLVER_H
#define TSP_SOLVER_H
#include "Util.h"
#include <vector>


// 地理坐标点
struct GeoPoint {
    double lng;  // 经度
    double lat;  // 纬度
    // 重载 < 运算符
    bool operator<(const GeoPoint& other) const {
        return lng < other.lng || (lng == other.lng && lat < other.lat);
    };
    GeoPoint(double longitude = 0, double latitude = 0) : lng(longitude), lat(latitude) {}
};

// 路径结果
struct PathResult {
    double distance;  // 总距离（公里）
    std::vector<GeoPoint> bestPath;  // 最优路径

    // 构造函数：在创建PathResult对象时自动调用, 初始化距离distance为0.0
    PathResult() : distance(0.0) {}
};

class TSPSolver {
public:
    // 计算两点间地理距离（封装计算地理距离的函数）
    static inline double calculateDistance(const GeoPoint& a, const GeoPoint& b) {
        return getDistance(a.lng, a.lat, b.lng, b.lat);
    }

    // 求解最短路径（只返回距离）
    static double findShortestPath(const GeoPoint& start, const GeoPoint& end,
        const std::vector<GeoPoint>& viaPoints);

    // 求解最短路径（返回完整路径信息）
    static PathResult findShortestPathWithRoute(const GeoPoint& start, const GeoPoint& end,
        const std::vector<GeoPoint>& viaPoints);

    // 验证输入是否有效
    static inline bool validateInput(const std::vector<GeoPoint>& viaPoints) {
        return viaPoints.size() <= 5;  // 不超过4个中间点
    }

    // 计算路径总长度
    static double calculatePathLength(const std::vector<GeoPoint>& path);
};

#endif