// 包含必要的头文件
#include "OperTarget.h"
#include "Mount.h"
#include "Load.h"
#include "MainTarget.h"
#include "Slot.h"
#include "MountType.h"
//#include "Schedule.h"

// 静态成员变量初始化，用于生成唯一ID
int OperTarget::_countId = 0;

// 构造函数1：通过分别指定挂载计划和装载计划来创建操作目标
// target: 要操作的目标对象
// mountPlanList: 挂载计划列表，包含挂载类型和对应的数量
// loadPlanList: 装载计划列表，包含装载类型和对应的数量
OperTarget::OperTarget(Target* target, vector<pair<Mount*, int>> mountPlanList, vector<pair<Load*, int>> loadPlanList)
    :_target(target), _mountPlanList(mountPlanList), _loadPlanList(loadPlanList)
{
    _id = _countId++; // 分配唯一ID并递增计数器
}

// 构造函数2：通过直接指定挂载-装载组合计划来创建操作目标
// target: 要操作的目标对象  
// mountLoadPlan: 挂载-装载组合计划列表，包含挂载装载组合和对应的数量
OperTarget::OperTarget(Target* target, vector<pair<MountLoad*, int>> mountLoadPlan)
    : _target(target), _mountLoadPlan(mountLoadPlan)
{
    _id = _countId++; // 分配唯一ID并递增计数器
}

// 构造函数3：仅指定目标，创建空的操作目标（后续需要添加计划）
// target: 要操作的目标对象
OperTarget::OperTarget(Target* target)
    : _target(target)
{
    _id = _countId++; // 分配唯一ID并递增计数器
}
// 实现析构函数
OperTarget::~OperTarget() {}
// 静态比较函数：用于对OperTarget对象进行排序
// ota: 第一个操作目标对象
// otb: 第二个操作目标对象
// 返回值: true表示ota应该排在otb前面，false表示ota应该排在otb后面
bool OperTarget::cmpByOrder(OperTarget* ota, OperTarget* otb)
{
    // 如果两个操作目标属于同一个主目标
    if (ota->getTarget()->getMainTarget() == otb->getTarget()->getMainTarget())
    {
        // 按目标自身的顺序号升序排序
        return ota->getTarget()->getOrder() < otb->getTarget()->getOrder();
    }
    // 如果两个操作目标属于不同主目标，且主目标的顺序号不同
    else if (ota->getTarget()->getMainTarget()->getOrder() != otb->getTarget()->getMainTarget()->getOrder())
    {
        // 按主目标的顺序号升序排序
        return ota->getTarget()->getMainTarget()->getOrder() < otb->getTarget()->getMainTarget()->getOrder();
    }
    // 其他情况（主目标顺序号相同但不是同一个主目标）
    else
    {
        // 按主目标的ID升序排序
        return ota->getTarget()->getMainTarget()->getId() < otb->getTarget()->getMainTarget()->getId();
    }
}

// 打印操作目标的详细信息
void OperTarget::print()
{
    // 输出目标基本信息
    cout << " target: " << _target->getName() << " ";
    //_target->print(); // 注释掉的代码，可能用于打印更详细的目标信息

    // 输出挂载-装载计划详情
    cout << " MountLoadPlan:";
    for (int i = 0; i < _mountLoadPlan.size(); i++)
    {
        // 输出每个挂载-装载组合的详细信息：挂载名称、装载名称、数量
        cout << _mountLoadPlan[i].first->_mount->getName() << " "
            << _mountLoadPlan[i].first->_load->getName() << "-"
            << _mountLoadPlan[i].second << ";";
    }
    cout << endl; // 换行
}

// 添加解决方案中的时间-挂载分配信息
// slot: 时间槽对象，表示操作发生的时间段
// mountLoad: 挂载-装载组合对象
// num: 在该时间槽使用该挂载-装载组合的数量
void OperTarget::pushSolnTimeMount(Slot* slot, MountLoad* mountLoad, int num)
{
    // 检查是否已存在该时间槽的记录
    if (_solnTimeMountMap.find(slot) != _solnTimeMountMap.end())
    {
        // 检查是否已存在该挂载-装载组合的记录
        if (_solnTimeMountMap[slot].find(mountLoad) != _solnTimeMountMap[slot].end())
        {
            // 如果已存在，增加数量
            _solnTimeMountMap[slot][mountLoad] = _solnTimeMountMap[slot][mountLoad] + num;
        }
        else
        {
            // 如果时间槽存在但该挂载-装载组合不存在，创建新记录
            _solnTimeMountMap[slot][mountLoad] = num;
        }
    }
    else
    {
        // 如果时间槽不存在，创建新记录
        _solnTimeMountMap[slot][mountLoad] = num;
    }
}

// 添加解决方案中的时间-飞机分配信息
// slot: 时间槽对象，表示操作发生的时间段
// mountLoad: 挂载-装载组合对象
// numList: 飞机数量列表，表示在该时间槽使用该挂载-装载组合的飞机数量分布
void OperTarget::pushSolnTimeAft(Slot* slot, MountLoad* mountLoad, vector<int> numList)
{
    _solnTimeAftMap[slot][mountLoad] = numList;
}

// 清空解决方案信息
// 用于重置操作目标的分配结果，以便重新计算
void OperTarget::clearSoln()
{
    _solnTimeAftMap.clear(); // 清空时间-飞机分配映射
    // 注意：这里只清除了_solnTimeAftMap，可能需要同时清除_solnTimeMountMap
}

// 获取最早的时间槽索引
// 返回值: 最早的时间槽ID，如果没有分配时间槽则返回0
int OperTarget::getEarliestSlotInd()
{
    // 使用迭代器遍历时间-挂载映射
    map<Slot*, map<MountLoad*, int>>::iterator iter = _solnTimeMountMap.begin();
    time_t earliestTime = 0;

    // 如果有分配的时间槽
    if (_solnTimeMountMap.size() > 0)
    {
        // 初始化最早时间为第一个时间槽的ID
        earliestTime = (*iter).first->getId();
    }

    // 遍历所有时间槽，找到最早的ID
    for (iter; iter != _solnTimeMountMap.end(); iter++)
    {
        if ((*iter).first->getId() < earliestTime)
        {
            earliestTime = (*iter).first->getId();
        }
    }

    return earliestTime; // 返回最早的时间槽ID
}