#include "tspSolver.h"
#include <algorithm>
#include <cmath>
#include <limits>

// 用于排序的比较函数
bool compareGeoPoints(const GeoPoint& a, const GeoPoint& b) {
    return a.lng < b.lng || (a.lng == b.lng && a.lat < b.lat);
}

double TSPSolver::findShortestPath(const GeoPoint& start, const GeoPoint& end,
    const std::vector<GeoPoint>& viaPoints) {
    int n = viaPoints.size();
    if (n == 0) {
        return calculateDistance(start, end);
    }

    if (!validateInput(viaPoints)) {
        return std::numeric_limits<double>::max();  // 返回极大值表示无效
    }

    double minDistance = std::numeric_limits<double>::max();
    std::vector<GeoPoint> points = viaPoints;  // 创建可修改的副本

    // 排序以便生成所有排列
    std::sort(points.begin(), points.end(), compareGeoPoints);

    do {
        double totalDistance = 0;

        // 起点到第一个中间点
        totalDistance += calculateDistance(start, points[0]);

        // 中间点之间
        for (int i = 0; i < n - 1; i++) {
            totalDistance += calculateDistance(points[i], points[i + 1]);
        }

        // 最后一个中间点到终点
        totalDistance += calculateDistance(points[n - 1], end);

        if (totalDistance < minDistance) {
            minDistance = totalDistance;
        }

    } while (std::next_permutation(points.begin(), points.end(), compareGeoPoints));//生成下一个字典序更大的组合

    return minDistance;
}

PathResult TSPSolver::findShortestPathWithRoute(const GeoPoint& start, const GeoPoint& end,
    const std::vector<GeoPoint>& viaPoints) {
    int n = viaPoints.size();
    PathResult result;
    result.distance = std::numeric_limits<double>::max();

    if (n == 0) {
        result.distance = calculateDistance(start, end);
        result.bestPath = { start, end };
        return result;
    }

    if (!validateInput(viaPoints)) {
        return result;  // 返回无效结果
    }

    std::vector<GeoPoint> points = viaPoints;
    std::sort(points.begin(), points.end(), compareGeoPoints);

    do {
        double totalDistance = calculateDistance(start, points[0]);

        for (int i = 0; i < n - 1; i++) {
            totalDistance += calculateDistance(points[i], points[i + 1]);
        }
        totalDistance += calculateDistance(points[n - 1], end);

        if (totalDistance < result.distance) {
            result.distance = totalDistance;
            result.bestPath = { start };
            result.bestPath.insert(result.bestPath.end(), points.begin(), points.end());
            result.bestPath.push_back(end);
        }

    } while (std::next_permutation(points.begin(), points.end(), compareGeoPoints));

    return result;
}

double TSPSolver::calculatePathLength(const std::vector<GeoPoint>& path) {
    if (path.size() < 2) {
        return 0.0;
    }

    double totalLength = 0.0;
    for (size_t i = 0; i < path.size() - 1; i++) {
        totalLength += calculateDistance(path[i], path[i + 1]);
    }
    return totalLength;
}