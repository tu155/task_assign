#include "Tour.h"
//#include "Schedule.h"

int Tour::_countId = 0;

Tour::Tour(vector<OperTarget*> targetList)
	:_operTargetList(targetList)
{
	_id = _countId++;
	_cost = 0.0;
	_totalOilCost = 0.0;
	_totalCost = 0.0;
	_roundId = 0;
	_earliestExploTime = 0;
	_fixed = false;
	_isVisit = false;
	_value = 0;
	_tourType = NOTFIX_TOUR;
}
Tour::Tour(Fleet* fleet, vector<OperTarget*> targetList)
	:_operTargetList(targetList), _fleet(fleet)
{
	_id = _countId++;
	_cost = 0.0;
	_totalOilCost = 0.0;
	_totalCost = 0.0;
	_roundId = 0;
	_earliestExploTime = 0;
	_fixed = false;
	_isVisit = false;
	_value = 0;
	_tourType = NOTFIX_TOUR;
}
Tour:: ~Tour()
{
}
void Tour::computeTimeMap() {
	
	_targetTimeMap.clear();
	_totalTime= 0.0;
	//遍历运力的服务目标，调用TSPSolver得到配送顺序和距离
	Base* base = getBase();
	GeoPoint start(base->getLongitude(), base->getLatitude());
	GeoPoint end(base->getLongitude(), base->getLatitude());

	// 中间途径点
	std::vector<GeoPoint> viaPoints;
	map<GeoPoint, Target*> genPointTargetMap;
	for (int i = 0; i < _operTargetList.size(); i++) {
		OperTarget* operTarget = _operTargetList[i];
		Target* target = operTarget->getTarget();
		viaPoints.push_back(GeoPoint(target->getLongitude(), target->getLatitude()));
        genPointTargetMap[GeoPoint(target->getLongitude(), target->getLatitude())] = target;
	}

	//// 计算最短距离
	//double distance = TSPSolver::findShortestPath(start, end, viaPoints);
	//std::cout << "最短距离: " << distance << " 公里" << std::endl;
	if (viaPoints.size() > 4) {
		cout << "途径点数量大于1" << endl;
	}
	// 获取完整路径
	PathResult result = TSPSolver::findShortestPathWithRoute(start, end, viaPoints);
	//std::cout << "最优路径距离: " << result.distance << " 公里" << std::endl;
	//std::cout << "路径包含 " << result.bestPath.size() << " 个点" << std::endl;

	//// 验证路径长度
	//double calculatedLength = TSPSolver::calculatePathLength(result.bestPath);
	//std::cout << "验证路径长度: " << calculatedLength << " 公里" << std::endl;
	
	// 安全地创建 viaOrderList
	vector<GeoPoint> viaOrderList;
	if (result.bestPath.size() >= 3) {
		// 只有至少3个元素时才能这样切片
		viaOrderList.assign(result.bestPath.begin() + 1, result.bestPath.end() - 1);
	}
	else {
		// 空路径
		viaOrderList.clear();
		std::cout << "警告: bestPath (包含起终点)的节点数量小于3" << std::endl;
	}
	//vector<GeoPoint> viaOrderList (result.bestPath.begin()+1, result.bestPath.end()-1);

	//计算途经点之间的配送时间，并更新时间表
	double cumulativeTime = 0.0;
	for (int i = 0; i < viaOrderList.size(); i++) {
		GeoPoint point= viaOrderList[i];
        Target* target = genPointTargetMap[point];
		double subDistance;
		if (i == 0) {
			subDistance= TSPSolver::calculateDistance(start, point);//起点到第一个途径点
		}
		else if (i == viaOrderList.size() - 1) {
			subDistance = TSPSolver::calculateDistance(point, end);//最后一个途径点到终点
		}
		else {
            subDistance = TSPSolver::calculateDistance(viaOrderList[i - 1], point);//途径点之间的距离
		}
		cumulativeTime += subDistance / getFleet()->getOptSpeed();
		//秒
		_targetTimeMap[target] = cumulativeTime;
	}
	//计算终点时间，更新tour的总时间
	Fleet* fleet = getFleet();
	double speed= fleet->getOptSpeed();
	_totalTime += result.distance / speed;
}

void Tour::computeCostNew() {
	_cost = 0.0;
	_cost+= _totalTime * _fleet->getOilPerMin();
	_cost += _fleet->	getCostUse();
}
void Tour::computeCost()
{
	_cost = 0.0;
	_totalCost = 0.0;
	_totalOilCost = 0.0;

	computeCostAvgDistance();//平均距离费
	computeCostAftOil();//燃油费

	double costUseLoad = 0;
	double costUseMount = 0;
	double costMountLoadMatch = 0;

	//double costBaseTarPref = 0.0;
	int totalLoadNum = 0;
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		OperTarget* operTarget = _operTargetList[i];
		vector<pair<MountLoad*, int >> mountLoadList = operTarget->getMountLoadPlan();
		for (int j = 0; j < mountLoadList.size(); j++)
		{
			costUseMount += mountLoadList[j].first->_mount->getCostUse() * mountLoadList[j].second * operTarget->getTarget()->getCoef();
			costUseLoad += mountLoadList[j].first->_load->getCostUse() * mountLoadList[j].second * operTarget->getTarget()->getCoef();
			costMountLoadMatch += mountLoadList[j].first->_mount->getLoadCost(mountLoadList[j].first->_load);
			totalLoadNum += mountLoadList[j].second;
		}
	}
	

	_cost += costUseLoad * Util::costUseWeight;
	_cost += costUseMount * Util::costUseWeight;
	_cost += costMountLoadMatch * Util::costUseWeight;
	//_cost += costBaseTarPref;
	_cost += getFleet()->getCostUse()  * Util::costUseWeight;

	_totalCost += costUseLoad;
	_totalCost += costUseMount;
	_totalCost += getFleet()->getCostUse();

	if (Util::ifEvaTotalLoadNum)
	{
		_cost += totalLoadNum;
	}

	if (Util::ifEvaGrpLoadFactor)
	{
		double avgLoadFactor = 0;
		int count = 0;
		Mount* mount = _operTargetList[0]->getMountLoadPlan()[0].first->_mount;
		map<MountLoad*, int>::iterator iter = _remainMLList.begin();
		int mountNum = 0;
		for (iter; iter != _remainMLList.end(); iter++)
		{
			mountNum += (*iter).second;
		}
		int totalMountNum =  getFleet()->getMountNum(mount);
		if (mountNum > 0)
		{
			_cost += ((double)mountNum / (double)totalMountNum) * Util::costGrpUnLoad;
		}
	}

	double costWait = 0;
	map<Slot*, int>::iterator iter = _timeMap.begin();
	for (iter; iter != _timeMap.end(); iter++)
	{
		costWait += (*iter).first->getId() * 50;
	}
	//if (_tourType == BOTHFIX_TOUR)
	//{
	//	costWait = 0;
	//}
	_cost += costWait;


	if (Util::isDebug)
	{
		Util::tourCostStruct.costUseLoad = Util::tourCostStruct.costUseLoad + costUseLoad * Util::costUseWeight;
		Util::tourCostStruct.costUseMount = Util::tourCostStruct.costUseMount + costUseMount * Util::costUseWeight;
		Util::tourCostStruct.costMountLoadMatch = Util::tourCostStruct.costMountLoadMatch + costMountLoadMatch * Util::costUseWeight;
		Util::tourCostStruct.costUseAft = Util::tourCostStruct.costUseAft + getFleet()->getCostUse() * Util::costUseWeight;
		Util::tourCostStruct.costWait = Util::tourCostStruct.costWait + costWait;
		//Util::tourCostStruct.costAftFailure = Util::tourCostStruct.costAftFailure + _grpType->getFleet()->getFailureRate() * Util::costAftFailure * _grpType->getAftNum();
		//Util::tourCostStruct.costFleetPrefer = Util::tourCostStruct.costFleetPrefer + costFleetPrefer;

		//cout << "costUseWeight-mount " << costUseMount * Util::costUseWeight << endl;
		//cout << "costUseWeight-fleet " << getFleet()->getCostUse() * Util::costUseWeight << endl;
		//cout << "costWait " << costWait << endl;
		//cout << "costAftFailure " << _grpType->getFleet()->getFailureRate() * Util::costAftFailure * _grpType->getAftNum() << endl;
	}
}

void Tour::computeLoadNum()
{
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		vector<pair<MountLoad*, int>> mountLoadPlanList = _operTargetList[i]->getMountLoadPlan();
		for (int j = 0; j < mountLoadPlanList.size(); j++)
		{
			if (_loadMap.find(mountLoadPlanList[j].first->_load) == _loadMap.end())
			{
				_loadMap[mountLoadPlanList[j].first->_load] = mountLoadPlanList[j].second;
			}
			else
			{
				_loadMap[mountLoadPlanList[j].first->_load] += mountLoadPlanList[j].second;
			}
		}
	}
}

int Tour::getLoadNum(Load* load)
{
	if (_loadMap.find(load) != _loadMap.end())
	{
		return _loadMap[load];
	}
	return 0;
}

void Tour::computeMountNum()
{
	_mountMap.clear();
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		vector<pair<Mount*, int>> mountPlanList = _operTargetList[i]->getMountPlanList();
		for (int j = 0; j < mountPlanList.size(); j++)
		{
			if (_mountMap.find(mountPlanList[j].first) == _mountMap.end())
			{
				_mountMap[mountPlanList[j].first] = mountPlanList[j].second;
				//cout << "mount name is " << mountPlanList[j].first->getName() << endl;
				//cout<<"mount num is " << mountPlanList[j].second << endl;
			}
			else
			{
				//cout << "出现累加" << endl;
				//cout<<"现有数量是："<<_mountMap[mountPlanList[j].first]<<endl;
				//cout<<"累加数量是："<<mountPlanList[j].second<<endl;
				_mountMap[mountPlanList[j].first] += mountPlanList[j].second;
			}
		}
	}
}
//计算所有操作目标列表中mountload的总数量
void Tour::computeMountLoadNum()
{
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		vector<pair<MountLoad*, int>> mountLoadList = _operTargetList[i]->getMountLoadPlan();

		for (int j = 0; j < mountLoadList.size(); j++)
		{
			if (_mountLoadMap.find(mountLoadList[j].first) == _mountLoadMap.end())
			{
				_mountLoadMap[mountLoadList[j].first] = mountLoadList[j].second;
			}
			else
			{
				_mountLoadMap[mountLoadList[j].first] += mountLoadList[j].second;
			}
		}
	}
}

/**
 * 计算行程的平均成本和距离
 * 该函数遍历目标列表，计算从基地到每个目标的距离总和，
 * 然后根据权重计算平均成本并更新总成本
 */
void Tour::computeCostAvgDistance()
{
    // 初始化总距离为0
    double totalDistance = 0;
    // 获取基地对象
    Base* base = getBase();
    // 获取成本距离权重
    double costWeight = Util::costDistWeight;
    
    // 遍历操作目标列表
    for (int i = 0; i < _operTargetList.size(); i++)
    {
        // 获取当前目标对象
        Target* target = _operTargetList[i]->getTarget();
        // 累加从基地到当前目标的距离
        totalDistance += getDistance(base->getLongitude(), base->getLatitude(), target->getLongitude(), target->getLatitude());
    }
    
    // 计算平均成本并更新总成本
    _cost += totalDistance * costWeight / (double)_operTargetList.size();
    
    // 如果处于调试模式，则更新调试结构中的成本距离数据
    if (Util::isDebug)
    {
        // 将当前计算的成本距离添加到调试结构中
        Util::tourCostStruct.costDst = Util::tourCostStruct.costDst + totalDistance * costWeight / (double)_operTargetList.size();
        // 注释掉的输出语句，用于调试输出
        //cout << totalDistance * costWeight / (double)_operTargetList.size() << endl;
    }
}

/**
 * 计算航线调整后的燃油费用
 * 该方法用于计算航线调整后的总燃油费用，包括基础距离计算、燃油费用计算以及总费用更新
 */
void Tour::computeCostAftOil()
{
	// 燃油费用权重系数
	double costWeight =Util::costOilWeight;
	// 总距离初始化为0
	double totalDistance = 0;
    // 获取基地信息
	Base* base = getBase();
	// 获取机队信息
	Fleet* fleet = getFleet();
	// 遍历操作目标列表，计算总距离
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		// 获取当前目标
		Target* target = _operTargetList[i]->getTarget();
		// 累加从基地到各目标的距离
		totalDistance += getDistance(base->getLongitude(), base->getLatitude(), target->getLongitude(), target->getLatitude());
	}
	// 计算总燃油费用：2倍的总距离除以(目标数量*最优速度)，再乘以60（转换为小时）和每分钟燃油消耗量
	_totalOilCost += 2 * totalDistance / ((double)_operTargetList.size() * fleet->getOptSpeed()) * 60 * fleet->getOilPerMin();//到所有目标的平均距离/机型速度 * 该机型燃油费（单位：/每分钟）* 飞机数量

	// 更新总费用，加入燃油费用及其权重
	_cost += _totalOilCost * costWeight;//*燃油费用的系数
	_totalCost += _totalOilCost;

	// 如果处于调试模式，输出燃油费用信息
	if (Util::isDebug)
	{
		Util::tourCostStruct.costOil = Util::tourCostStruct.costOil + _totalOilCost * costWeight;
		cout << "totalOilCost " << _totalOilCost * costWeight << endl;
	}
}

int Tour::getMountNum(Mount* mount)
{
	if (_mountMap.find(mount) != _mountMap.end())
	{
		return _mountMap[mount];
	}
	return 0;
}

void Tour::print()
{
	cout << "#TourId-" << _id << " cost- " << _cost << " " << endl;
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		_operTargetList[i]->print();
	}
	if (_timeMap.size() > 0)
	{
		map<Slot*, int>::iterator iter = _timeMap.begin();
		for (iter; iter != _timeMap.end(); iter++)
		{
			cout << (*iter).first->getStTime() << " " << (*iter).second << endl;
		}
	}
}

double Tour::computeAvgPrio()
{
	double totalPrio = 0;
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		totalPrio += _operTargetList[i]->getTarget()->getPriority();
	}
	return totalPrio / (double)_operTargetList.size();
}

OperTarget* Tour::getOperTarget(string targetName)
{
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		if (_operTargetList[i]->getTarget()->getName() == targetName)
		{
			return _operTargetList[i];
		}
	}
	return NULL;
}

void Tour::pushRemainML(MountLoad* mountLoad, int num)
{
	if (_remainMLList.find(mountLoad) != _remainMLList.end())
	{
		_remainMLList[mountLoad] = _remainMLList[mountLoad] + num;
	}
	else
	{
		_remainMLList[mountLoad] = num;
	}
}

void Tour::setTimeMap(map<Slot*, int> timeMap)
{
	_timeMap = timeMap;
	map<Slot*, int>::iterator iter = _timeMap.begin();
	if (_timeMap.size() > 0)
	{
		time_t eet = (*iter).first->getStTime();
		for (iter; iter != _timeMap.end(); iter++)
		{
			if (eet > (*iter).first->getStTime())
			{
				eet = (*iter).first->getStTime();
			}
		}
		_earliestExploTime = eet;
	}
}

bool Tour::isOccupy()
{
	map<MountLoad*, int>::iterator  iter = _mountLoadMap.begin();
	for (iter; iter != _mountLoadMap.end(); iter++)
	{
		if ((*iter).first->_mount->isOccupy())
		{
			return true;
		}
	}
	return false;
}

void Tour::clearSoln()
{
	for (int i = 0; i < _operTargetList.size(); i++)
	{
		_operTargetList[i]->clearSoln();
	}
	_cost = 0;
	_fixed = false;
	_isVisit = false;
	_value = 0;
}

/**
 * 为子目标分配时间
 * @param fleet 舰队指针
 * @param aftNum 舰队数量
 */
void Tour::assignTimeForSubTargets(Fleet* fleet, int aftNum)
{
    // 获取操作目标列表
    vector<OperTarget*> operTargetList = this->getOperTargetList();
    // 获取第一个操作目标的第一个装载计划的挂载
    Mount* mount = operTargetList[0]->getMountLoadPlan()[0].first->_mount;
    // 获取舰队中该挂载的数量
    int mountNumPerAft = fleet->getMountNum(mount);
    // 按顺序对操作目标列表进行排序
    sort(operTargetList.begin(), operTargetList.end(), OperTarget::cmpByOrder);
    // 获取时间映射表
    map<Slot*, int> timeMap = this->getTimeMap();
    // 创建时间映射表的迭代器
    map<Slot*, int>::iterator iter = timeMap.begin();
    // 创建剩余槽位列表
    vector<pair<Slot*, int>> remainSlotList;
    // 计算每个时间段可分配的挂载数量
    int mountNumPerTime = mountNumPerAft;
    // 如果挂载类型有间隔时间，则每个时间段最多分配2个挂载
    if (mount->getMountType()->intervalTime != 0)
    {
        mountNumPerTime = 2;
    }
    // 遍历时间映射表，构建剩余槽位列表
    for (iter; iter != timeMap.end(); iter++)
    {
        if (mount->isOccupy())
        {
            // 如果挂载被占用，则按挂载数量计算剩余槽位
            remainSlotList.push_back(make_pair((*iter).first, (*iter).second * mountNumPerTime));
        }
        else
        {
            // 如果挂载未被占用，则按尾翼数量计算剩余槽位
            remainSlotList.push_back(make_pair((*iter).first, aftNum * mountNumPerTime));
        }
    }
    // 按时间对剩余槽位列表进行排序
    sort(remainSlotList.begin(), remainSlotList.end(), Slot::cmpByTime);
    // 保存原始槽位列表
    vector<pair<Slot*, int>> originSlotList = remainSlotList;

    // 遍历操作目标列表
    for (int m = 0; m < operTargetList.size(); m++)
    {
        // 获取操作目标的装载计划
        vector<pair<MountLoad*, int>> mountLoadPlan = operTargetList[m]->getMountLoadPlan();
        // 遍历装载计划
        for (int j = 0; j < mountLoadPlan.size(); j++)
        {
            // 获取装载信息和数量
            MountLoad* mountLoad = mountLoadPlan[j].first;
            int num = mountLoadPlan[j].second;
            // 遍历剩余槽位列表
            for (int q = 0; q < remainSlotList.size(); q++)
            {
                // 如果槽位还有剩余容量
                if (remainSlotList[q].second > 0)
                {
                    // 如果所需数量为0，跳出循环
                    if (num == 0)
                    {
                        break;
                    }
                    // 如果所需数量小于等于剩余槽位容量
                    if (num <= remainSlotList[q].second)
                    {
                        // 将槽位、装载和数量添加到解决方案中
                        operTargetList[m]->pushSolnTimeMount(remainSlotList[q].first, mountLoad, num);
                        // 更新剩余槽位容量
                        remainSlotList[q].second = remainSlotList[q].second - num;
                        num = 0;
                        break;
                    }
                    else
                    {
                        // 将剩余槽位容量全部分配给当前装载
                        operTargetList[m]->pushSolnTimeMount(remainSlotList[q].first, mountLoad, remainSlotList[q].second);
                        // 更新所需数量
                        num = num - remainSlotList[q].second;
                        // 将槽位容量置为0
                        remainSlotList[q].second = 0;
                    }
                }
            }
        }
    }
    // 更新操作目标列表
    _operTargetList = operTargetList;

    // 遍历操作目标列表，检查最早开始时间
    for (int m = 0; m < operTargetList.size(); m++)
    {
        // 如果目标的最早开始时间大于主目标的最早解决方案开始时间
        if (operTargetList[m]->getEarliestSlotInd() > operTargetList[m]->getTarget()->getMainTarget()->getSolnEarliesBgnTime())
        {
            // 此处为空，可能是未完成的代码
        }
    }
}

bool Tour::cmpByCost(Tour* tourA, Tour* tourB)
{
	return tourA->getCost() < tourB->getCost();
}

bool Tour::cmpByOrder(Tour* tourA, Tour* tourB)
{
	return tourA->getOperTargetList()[0]->getTarget()->getMainTarget()->getOrder() < tourB->getOperTargetList()[0]->getTarget()->getMainTarget()->getOrder();
}

double Tour::getTargetDst(OperTarget* operTarget)
{
	return getDistance(getBase()->getLongitude(), getBase()->getLatitude(), operTarget->getTarget()->getLongitude(), operTarget->getTarget()->getLatitude());
}