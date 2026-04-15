#pragma once  // 防止头文件被重复包含

#include"Util.h"  // 包含工具类头文件

/**
 * @brief 时间槽类，用于管理特定时间段内的资源分配
 */
class Slot
{
private:
    static int _countId;  // 静态成员变量，用于生成唯一ID
    int _id;              // 时间槽的唯一标识符
    int _cap;             // 时间槽的总容量
    time_t _stTime;       // 时间槽的开始时间
    time_t _endTime;      // 时间槽的结束时间
    vector<int> _pointList;  // 存储分配点的列表

    int _remainCap;       // 时间槽的剩余容量

public:
    /**
     * @brief 构造函数
     * @param cap 时间槽的总容量
     * @param stTime 开始时间
     * @param endTime 结束时间
     */
    Slot(int cap, time_t stTime, time_t endTime);

    /**
     * @brief 析构函数
     */
    ~Slot();

    // Getter和Setter方法
    int getId() { return _id; }  // 获取时间槽ID
    void setId(int id) { _id = id; }  // 设置时间槽ID

    int getRemainCap() { return _remainCap; }  // 获取剩余容量
    void setRemainCap(int remainCap) { _remainCap = remainCap; }  // 设置剩余容量

    vector<int> getPointList() { return _pointList; }  // 获取分配点列表
    void pushPoint(int p) { _pointList.push_back(p); }  // 添加分配点

    // 获取时间槽的基本属性
    int getCap() { return _cap; }  // 获取总容量
    time_t getStTime() { return _stTime; }  // 获取开始时间
    time_t getEndTime() { return _endTime; }  // 获取结束时间

    /**
     * @brief 比较函数，用于按时间排序
     * @param pA 第一个时间槽对
     * @param pB 第二个时间槽对
     * @return bool 比较结果
     */
    static bool cmpByTime(pair<Slot*, int> pA, pair<Slot*, int> pB);

    /**
     * @brief 初始化计数器
     */
    static void initCountId() { _countId = 0; }

    /**
     * @brief 删除指定分配点
     * @param point 要删除的分配点
     */
    void deletePoint(int point);

    /**
     * @brief 检查点是否存在
     * @param point 要检查的点列表
     * @return bool 是否存在
     */
    bool checkExist(vector<int> point);
};
#pragma once  // 防止头文件被重复包含
