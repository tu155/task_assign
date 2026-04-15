#include "Individual.h"
#include "DataIO.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numeric>
using std::make_pair;
// 定义静态成员变量

Schedule* Individual::_schedule = nullptr;

// 其他成员函数实现...
//贪心算法思路：按照目标优先级，从可选运力类型中，优先可供量高的运力，成本低的运力
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


int Individual::_countId = 0;

Individual::Individual(string flag, Schedule* schedule) {
	_schedule = schedule;
	_id = _countId++;
	_flag = flag;
	_objCount = 3;
	_SumTime = 0.0;
	_SumCost = 0.0;
	_SumDifference = 0.0;
	_rank = 0;                         // 非支配排序等级
	_crowdingDistance = 0.0;         // 拥挤度
	_dominatedCount = 0;
}
Individual:: ~Individual()
{
}
//个体编码函数
void Individual::encode() {
	//1.找到目标可选的资源
	vector<map<Target*, map<Fleet*, int>>> resultFrom;
	vector <Target*> target_list = _schedule->getTargetList();
	map<Target*, vector<Fleet*>> targetFleetMap;
	int startIdx = 0;
	map<pair<Fleet*,Mount*>, int>fleetRemainMountsMap;				//存储运力剩余货物数量

	//遍历目标
	for (int i = 0; i < target_list.size(); i++) {
		Target* target = target_list[i];
		vector<TargetPlan*>targetPlanList = target->getType()->getTargetPlanList();
		//遍历目标的货物需求计划
		for (int k = 0; k < targetPlanList.size(); k++) {
			TargetPlan* targetPlan = targetPlanList[k];
			int targetNeedNum;
			//遍历运力类型，如果运力可以装载对应的目标Mount，则该运力类型能够加入目标可选集合
			for (int j = 0; j < _schedule->getFleetTypeList().size(); j++) {
				FleetType* fleetType = _schedule->getFleetTypeList()[j];
				vector<Fleet*> fleetList = fleetType->getFleetList();
				for (int j = 0; j < fleetList.size(); j++) {
					Fleet* fleet = fleetList[j];
					vector<pair<Mount*, int>>mountPlanList = fleet->getMountPlanList();
					//遍历运力类型装载的货物,
					for (int k = 0; k < mountPlanList.size(); k++) {
						Mount* mount = mountPlanList[k].first;
						//初始化运力剩余的可装载货物数量为输入的装载量
						auto it = fleetRemainMountsMap.find(make_pair(fleet, mount));
						if (it == fleetRemainMountsMap.end()) {
							fleetRemainMountsMap[make_pair(fleet, mount)] = fleet->getMountNum(mount);
						}
					}
				}
				vector<pair<Mount*, int>>mountPlanList = fleetList[0]->getMountPlanList();
				//遍历运力类型装载的货物,
				for (int k = 0; k < mountPlanList.size(); k++) {
					Mount* mount = mountPlanList[k].first;
					//目标计划中有这种货物需求
					if (targetPlan->getMountNum(mount) != 0) {
						//如果没有记录过target的需求数量，则记录
						if (_targetNeedNum.find(target) == _targetNeedNum.end()) {
							targetNeedNum = targetPlan->getMountNum(mount);
							_targetNeedNum[target] = targetPlan->getMountNum(mount);//赋值这个个体的目标需求
						}
						vector<int> numList(fleetList.size(), 0);
						int endIdx = numList.size();
						for (int j = 0; j < fleetList.size(); j++) {
							Fleet* fleet = fleetList[j];

							//设置索引含义
							tuple<Target*, Mount*, Fleet*> threeElements = { target, mount, fleet };
							//如果指针不为空，才加入
							_idxMeaning.push_back(threeElements);
							_meaningIdx[threeElements] = _idxMeaning.size()-1;
							
						}
						//按照偏好配置货物数量
						vector<Fleet*> NewfleetList = fleetList;
						
						while (targetNeedNum > 0) {
							Fleet* fleet;
							//如果NewfleetList为空，跳过这个类型的资源
							if (NewfleetList.size() == 0) {
								break;
							}
							if (_flag == "timeShortest") {
								fleet = Individual::findTimeShortestFleet(target, NewfleetList);
							}
							else if (_flag == "costLowest") {
								fleet = Individual::findCostLowestFleet(target, NewfleetList);
							}
							else {
								fleet= NewfleetList[rand() % NewfleetList.size()];
								//fleet = NewfleetList[pick_unique(0, NewfleetList.size() - 1, 1)[0]];
							}
							//如果资源装过其他mount，直接跳过
							bool choose_other_mount = false;
							//cout<<fleet->getName()<<endl;
                            vector<pair<Mount*, int>>chooseMountPlanList = fleet->getMountPlanList();
							//cout<<fleet->getName()<<" mountPlanNum: "<<chooseMountPlanList.size()<<endl;
							for (int z = 0; z < chooseMountPlanList.size(); z++) {
								if (chooseMountPlanList[z].first != mount) {
									/*cout<<"chooseMountPlanList[z].first: "<<chooseMountPlanList[z].first->getName()<<"num: "<<fleetRemainMountsMap[make_pair(fleet, mountPlanList[z].first)] << endl;
									cout<<"capacity: "<<fleet->getMountNum(chooseMountPlanList[z].first)<<endl;
									cout<<"mount: "<<mount->getName()<<endl;*/
									//如果其他资源没加入到fleetRemainMountsMap中，bool choose_other_mount = false;
									if (fleetRemainMountsMap.find(make_pair(fleet, chooseMountPlanList[z].first)) == fleetRemainMountsMap.end()) {
										break;
									}
									//查看这个mount是否已经加入fleetRemainMountsMap
									else if (fleetRemainMountsMap[make_pair(fleet, mountPlanList[z].first)] != fleet->getMountNum(chooseMountPlanList[z].first)) {
										choose_other_mount = true;
										//删除这个不可用资源
										auto fleetIdxChange = find(NewfleetList.begin(), NewfleetList.end(), fleet) - NewfleetList.begin();
										NewfleetList.erase(fleetIdxChange + NewfleetList.begin());
										//cout << NewfleetList.size() << endl;
										break;
									}
								}
							}
							if (choose_other_mount) {
								continue;
							}
							int num = min(fleetRemainMountsMap[make_pair(fleet,mount)], targetNeedNum);
							auto fleetIdx = find(fleetList.begin(), fleetList.end(), fleet) - fleetList.begin();
							numList[fleetIdx] += num;
							auto fleetIdxChange = find(NewfleetList.begin(), NewfleetList.end(), fleet) - NewfleetList.begin();
							////更新剩余可选运力，即删除刚刚选择的fleet
							//NewfleetList.erase(fleetIdx + NewfleetList.begin());
							//更新运力的剩余能力
							fleetRemainMountsMap[make_pair(fleet,mount)] -= num;
							////如果运力的剩余能力为0，则删除该运力
							//if (fleetRemainMountsMap[make_pair(fleet,mount)] == 0) {
							//	NewfleetList.erase(fleetIdxChange + NewfleetList.begin());
							//	//cout<<NewfleetList.size()<<endl;
							//}
							//运力只能给一个目标提供货物或者毁伤
							NewfleetList.erase(fleetIdxChange + NewfleetList.begin());
							//如果可用运力为0，跳出循环
							if (NewfleetList.size() == 0) {
								break;
							}
							//更新目标需求的货物数量
							targetNeedNum -= num;
							//cout<<"减少的数量是: "<<num<<endl;
							//cout<<"targetNeedNum: "<<targetNeedNum<<endl;


						}
						//更新result中的实数编码
						_variables.insert(_variables.end(), numList.begin(), numList.end());
						
						targetFleetMap[target].insert(targetFleetMap[target_list[i]].end(), fleetList.begin(), fleetList.end());

					}
				}
			}
		}
		_partPosition.push_back(_variables.size() - 1);
	}
}

void Individual::decode() {
	//getAllocation();因为加上之后时间变长，所以注释
	//清除_solnTourList
	_solnTourList.clear();
	for (int i = 0; i < _variables.size(); i++) {
		if (_variables[i] != 0) {
			Target* target = get<0>(_idxMeaning[i]);
			Mount* mount = get<1>(_idxMeaning[i]);
			Fleet* fleet = get<2>(_idxMeaning[i]);
			
			vector<TargetPlan*>targetPlanList = target->getType()->getTargetPlanList();
			for (int j = 0; j < targetPlanList.size(); j++) {
				TargetPlan* targetPlan = targetPlanList[j];
				Load* load;
				//找到目标需求计划中需要mount的目标需求计划（本项目中的mount和load是一一对应的）
				if (targetPlan->getMount() == mount) {
					map<Load*, int>LoadPlan = targetPlan->getLoadPlan();
					load = LoadPlan.begin()->first;
					//OperTarget::OperTarget(Target * target, vector<pair<Mount*, int>> mountPlanList, vector<pair<Load*, int>> loadPlanList);
					vector<pair<Mount*, int>> mountPlanList = { make_pair(mount, _variables[i]) };
					vector<pair<Load*, int>> loadPlanList = { make_pair(load, _variables[i]) };
					MountLoad* mountLoad = _schedule->getMountLoad(mount, load);
					vector<pair<MountLoad*, int>> mountLoadPlanList = { make_pair(mountLoad, _variables[i]) };
					OperTarget* operTarget = new OperTarget(target, mountPlanList, loadPlanList);
					operTarget->setMountLoadPlan(mountLoadPlanList);
					vector<Tour*> tourList = getsolnTourList();
					//记录运力类型是否被规划过
					bool fleetPlanned = false;
					for (int k = 0; k < tourList.size(); k++) {
						Tour* tour = tourList[k];
						//如果运力已经有了tour
						if (tour->getFleet() == fleet) {
							fleetPlanned = true;
							tour->computeMountNum();
							//检查当前运力是否已经达到最大容量:
							if (tour->getMountNum(mount) + _variables[i] <= tour->getFleet()->getMountNum(mount)) {
								tour->pushOperTarget(operTarget);
							}
							else {
								/*Tour* newTour = new Tour(fleet, { operTarget });
								Base* base = _schedule->getBase();
								newTour->setBase(base);
								pushsolnTourList(newTour);*/
								//cout << "Error: 解码中Fleet "<< tour->getFleet()->getName()<<"capacity exceeded" << endl;
								//cout<<"tour中的数量是"<< tour->getMountNum(mount) << endl;
								//cout<<"当前要加入的数量是"<< _variables[i] << endl;
        //                        cout<<"fleet的容量是"<< tour->getFleet()->getMountNum(mount) << endl;
							}
						}
					}
					//如果运力还没有被规划过
					if (!fleetPlanned) {
						Tour* tour = new Tour(fleet, { operTarget });
						if (fleet->getMountNum(mount) < _variables[i]) {
							//cout<<"编码问题"<<endl;
							//cout<<"fleet的容量是"<< tour->getFleet()->getMountNum(mount) << endl;
       //                     cout<<"分配的数量是"<< _variables[i] << endl;
						}
						Base* base = _schedule->getBase();
						tour->setBase(base);
						pushsolnTourList(tour);
						//_schedule->pushTour(tour);
					}
				}
			}
		}
	}
}

Fleet* Individual::findTimeShortestFleet(Target* target, vector<Fleet*>fleetList) {
	//计算飞机列表中距离目标最近的飞机
	//遍历所有飞机，计算飞机到目标的距离，除以平均速度，得到飞机到目标的时间
	double minTime = DBL_MAX;
	Fleet* shortestFleet = nullptr;
	for (Fleet* fleet : fleetList) {
		double distance = getDistance(fleet->getPos().first, fleet->getPos().second, target->getLongitude(), target->getLatitude());
		double time = distance / fleet->getOptSpeed();
		if (time < minTime) {
			minTime = time;
			shortestFleet = fleet;
		}
	}
	return shortestFleet;
}

Fleet* Individual::findCostLowestFleet(Target* target, vector<Fleet*>fleetList) {
	//计算飞机列表中完成目标配送任务成本最低的飞机
	//成本包括燃油费用和飞机单次使用的成本
	double minCost = DBL_MAX;
	Fleet* lowestCostFleet = nullptr;
	for (Fleet* fleet : fleetList) {
		double distance = getDistance(fleet->getPos().first, fleet->getPos().second, target->getLongitude(), target->getLatitude());
		double time = distance / fleet->getOptSpeed();
		double cost = time * fleet->getOilPerMin() + fleet->getCostUse();

		if (cost < minCost) {
			minCost = cost;
			lowestCostFleet = fleet;
		}
	}
	return lowestCostFleet;
}
Fleet* Individual::findDifferenceMinFleet(Target* target, vector<Fleet*>fleetList) {
	//计算飞机列表中完成目标配送任务对当前在执行目标影响最小的飞机
	//如果目标已经分配过，那么尽量选择原飞机
	//如果目标没有分配过，那么随机选择空闲飞机
	map<Fleet*, pair<int, map<MountLoad*, int>>>asgResourse= target->getAsgResourse();
	//遍历飞机，如果asgResourse中存在，则返回该飞机
	for (Fleet* fleet : fleetList) {
		if (asgResourse.find(fleet) != asgResourse.end()) {
			return fleet;
		}
	}
    return fleetList[rand() % fleetList.size()];
}

void Individual::cocuSumCostTime() {
	_SumTime=0;
    _SumCost=0;
	std::uniform_int_distribution<size_t> dist(0, _solnTourList.size() - 1);
	//随机选取一半的tour计算目标函数
	vector<Tour*> solnTourList;
	for (int i = 0; i < _solnTourList.size() / 2; i++) {
		solnTourList.push_back(_solnTourList[dist(gen)]);
	}
	for (int i = 0; i < solnTourList.size(); i++) {
		Tour* tour = solnTourList[i];
		tour->computeTimeMap();
		double timeTour = solnTourList[i]->getTotalTime();
		if (timeTour > _SumTime) {
			_SumTime = timeTour;
		}
		tour->computeCostNew();
		_SumCost += tour->getCost();
	}
}


void Individual::cocuSumDifference() {
	_SumDifference = 0.0;
	std::uniform_int_distribution<size_t> dist(0, _solnTourList.size() - 1);
	//随机选取一半的tour计算目标函数
	vector<Tour*> solnTourList;
	for (int i = 0; i < _solnTourList.size() / 2; i++) {
		solnTourList.push_back(_solnTourList[dist(gen)]);
	}
	//计算所有目标的延迟完成时间
	map<Target*, double>orignalTimeMap;
	map<Target*, double>newTimeMap;
	map<Target*, double>delayTimeMap;
	
	//遍历所有的tour,找到目标所在的tour，目标在所有tour中的完成时间最大值为目标的完成时间
	for (int j = 0; j < solnTourList.size(); j++) {
		Tour* tour = solnTourList[j];
		if (tour == nullptr) {
			//cout << "Error: 计算difference时tour为空" << endl;
			continue;  // 或者记录日志
		}
		//cocuSumCostTime中已经计算过该项，注释，直接使用值
		//tour->computeTimeMap();

		// 假设 getTargetTimeMap() 返回 std::map<Target*, double>& 或 const std::map<Target*, double>&
		auto timeMap = tour->getTargetTimeMap();

		for (auto it = timeMap.begin(); it != timeMap.end(); ++it) {
			Target* target = it->first;
			if (target == nullptr) {
				//cout << "Error: 计算difference时target为空" << endl;
				continue;
			}

			double newTime = it->second;
			double originalTime = target->getLastPlanCmpTime();

			if (originalTime == 0) {
				continue;
			}
			else {
				if (newTimeMap.find(target) == newTimeMap.end()) {
					newTimeMap[target] = newTime;
					orignalTimeMap[target] = originalTime;
				}
				else {
					if (newTimeMap.find(target) != newTimeMap.end()) {
						if (newTimeMap[target] < newTime) {
							newTimeMap[target] = newTime;
						}
					}
				}
			}
		}
	}
	//计算所有目标的延迟完成时间
	for (auto it = newTimeMap.begin(); it != newTimeMap.end(); ++it) {
		Target* target = it->first;
		double delayTime = max(newTimeMap[target] - orignalTimeMap[target], 0.0);
		delayTimeMap[target] = delayTime;
		_SumDifference += delayTime;
	}
}

void Individual::computeObjectives() {
	_objectives.clear();
	cocuSumCostTime();
	//cocuSumTime();
	cocuSumDifference();
	_objectives.push_back(_SumTime);
	_objectives.push_back(_SumCost);
	if (_objCount == 3) {
		_objectives.push_back(_SumDifference);
	}
}

void Individual::cocuSumCostTimeTotal() {
	_SumTime = 0;
	_SumCost = 0;
	
	for (int i = 0; i < _solnTourList.size(); i++) {
		Tour* tour = _solnTourList[i];
		tour->computeTimeMap();
		double timeTour = _solnTourList[i]->getTotalTime();
		if (timeTour > _SumTime) {
			_SumTime = timeTour;
		}
		tour->computeCostNew();
		_SumCost += tour->getCost();
	}
}


void Individual::cocuSumDifferenceTotal() {
	_SumDifference = 0.0;
	
	//计算所有目标的延迟完成时间
	map<Target*, double>orignalTimeMap;
	map<Target*, double>newTimeMap;
	map<Target*, double>delayTimeMap;

	//遍历所有的tour,找到目标所在的tour，目标在所有tour中的完成时间最大值为目标的完成时间
	for (int j = 0; j < _solnTourList.size(); j++) {
		Tour* tour = _solnTourList[j];
		if (tour == nullptr) {
			//cout << "Error: 计算difference时tour为空" << endl;
			continue;  // 或者记录日志
		}
		//cocuSumCostTime中已经计算过该项，注释，直接使用值
		//tour->computeTimeMap();

		// 假设 getTargetTimeMap() 返回 std::map<Target*, double>& 或 const std::map<Target*, double>&
		auto timeMap = tour->getTargetTimeMap();

		for (auto it = timeMap.begin(); it != timeMap.end(); ++it) {
			Target* target = it->first;
			if (target == nullptr) {
				//cout << "Error: 计算difference时target为空" << endl;
				continue;
			}

			double newTime = it->second;
			double originalTime = target->getLastPlanCmpTime();

			if (originalTime == 0) {
				continue;
			}
			else {
				if (newTimeMap.find(target) == newTimeMap.end()) {
					newTimeMap[target] = newTime;
					orignalTimeMap[target] = originalTime;
				}
				else {
					if (newTimeMap.find(target) != newTimeMap.end()) {
						if (newTimeMap[target] < newTime) {
							newTimeMap[target] = newTime;
						}
					}
				}
			}
		}
	}
	//计算所有目标的延迟完成时间
	for (auto it = newTimeMap.begin(); it != newTimeMap.end(); ++it) {
		Target* target = it->first;
		double delayTime = max(newTimeMap[target] - orignalTimeMap[target], 0.0);
		delayTimeMap[target] = delayTime;
		_SumDifference += delayTime;
	}
}

void Individual::computeObjectivesTotal() {
	_objectives.clear();
	cocuSumCostTimeTotal();
	//cocuSumTime();
	cocuSumDifferenceTotal();
	_objectives.push_back(_SumTime);
	_objectives.push_back(_SumCost);
	if (_objCount == 3) {
		_objectives.push_back(_SumDifference);
	}
}


bool Individual::dominates(const Individual& other) const {
	// 检查向量是否已初始化
	if (_objectives.empty() || other._objectives.empty()) {
		//std::cerr << "错误: _objectives 向量未初始化!" << std::endl;
		return false;
	}

	// 检查向量大小是否一致
	if (_objectives.size() != other._objectives.size()) {
		//std::cerr << "错误: _objectives 向量大小不一致! "<< _objectives.size() << " vs " << other._objectives.size() << std::endl;
		return false;
	}
	bool better_in_one = false;

	// 假设所有目标都是最小化
	for (size_t i = 0; i < _objectives.size(); ++i) {
		// 如果在一个目标上比另一个差，则不支配
		if (_objectives[i] > other._objectives[i]) {
			return false;
		}
		// 如果在一个目标上比另一个好，标记
		if (_objectives[i] < other._objectives[i]) {
			better_in_one = true;
		}
	}

	return better_in_one;
}

void Individual::printObjectives() const {
	std::cout << "(";
	for (size_t i = 0; i < _objectives.size(); ++i) {
		std::cout << _objectives[i];
		if (i < _objectives.size() - 1) {
			std::cout << ", ";
		}
	}
	std::cout << ")"<<endl;
}


// 修复可行性
void Individual::repairFeasibility() {
	bool repaired;                    // 状态标志：是否已完全修复
	int max_repair_iterations = 10;   // 最大修复迭代次数
	int iterations = 0;               // 当前迭代计数
	//computeTargetDemand();		// 计算目标需求

	do {
		repaired = true;              // 假设本轮迭代已修复

		// 检查1：需求满足性
		if (!checkDemandSatisfaction()) {
			repairDemandViolation();  // 执行修复
			repaired = false;         // 标记为"未完全修复"
		}

		// 检查2：容量约束  
		if (!checkCapacityConstraints()) {
			repairCapacityViolation(); // 执行修复
			repaired = false;          // 标记为"未完全修复"
		}

		iterations++;
	} while (!repaired && iterations < max_repair_iterations);
	// 继续循环条件：未修复 且 未超最大迭代次数
}

// 检查需求是否满足
bool Individual::checkDemandSatisfaction() {
	getAllocation();
	//vector <Target*> target_list = _schedule->getTargetList();
	//for (const auto& target : target_list) {
	for (auto& [key, value] : _targetAllocation) {
		if (value < _targetNeedNum[key.first]) {
			//cout << "Error: 目标需求未满足" << endl;
			//cout << "target:" << key.first->getName() << " need:" << _targetNeedNum[key.first] << " allocation:" << value << endl;
			return false;
		}
	}
	return true;
}

// 检查容量约束
bool Individual::checkCapacityConstraints() {
	getAllocation();
	for (const auto& [key, value] : _fleetAllocation) {
		int fleetCapacity = key.first->getMountNum(key.second);
		if (value > fleetCapacity) {
            //cout << "Error: 资源容量超出" << endl;
           //cout << "fleet:" << key.first->getName() << " mount:" << key.second->getName() << " allocation:" << value << " capacity"<< fleetCapacity << endl;
			return false;
		}
	}
	return true;
}
// 修复因容量超出导致的目标需求未覆盖
void Individual::repairDemandViolationforCapacity(vector <Target*> uncoverTarget_list, Mount* mount) {
	for (const auto& target : uncoverTarget_list) {
		int total_allocation = _targetAllocation[std::make_pair(target, mount)];
		int shortage = _targetNeedNum[target] - total_allocation;
		//找到可用的资源
		vector<Fleet*> available_resources;
		map<Fleet*, int> fleetRemain;
		if (shortage > 0) {
			for (FleetType* fleettype : _schedule->getFleetTypeList()) {
				if (fleettype->getFleetList()[0]->getMountNum(mount) > 0) {
					for (Fleet* fleet : fleettype->getFleetList()) {
						//如果飞机还有剩余容量
						int remaining_capacity = fleet->getMountNum(mount) - _fleetAllocation[make_pair(fleet, mount)];
						if (remaining_capacity > 0) {
							available_resources.push_back(fleet);
						}
						fleetRemain[fleet] = remaining_capacity;
					}
				}
			}
		}
		// 如果没有可用的资源，跳过
		if (available_resources.empty()) {
			//cout<<"Error: 没有可用的资源" << endl;
			continue;
		}

		// 分配短缺的货物
		while (shortage > 0 && !available_resources.empty()) {
			Fleet* fleet;
			fleet = available_resources[rand() % available_resources.size()];
			int remaining_capacity = fleetRemain[fleet];
			int allocation_amount = min(shortage, remaining_capacity);
			if (allocation_amount > 0) {
				_fleetAllocation[make_pair(fleet, mount)] += allocation_amount;
				_targetAllocation[make_pair(target, mount)] += allocation_amount;
				//更新_variables
				tuple<Target*, Mount*, Fleet*> meaning = { target, mount, fleet };
				int idx = _meaningIdx[meaning];
				//cout << _variables[idx] << endl;
				//cout<<allocation_amount<<endl;
				_variables[idx] += allocation_amount;
				//cout <<fleet->getName() << "增加" << allocation_amount << endl;
				//更新目标的剩余需求与资源剩余容量
				shortage -= allocation_amount;
				fleetRemain[fleet] -= allocation_amount;
				//如果飞机剩余容量为0，则从可用资源中移除
				if (fleetRemain[fleet] == 0) {
					available_resources.erase(std::remove(available_resources.begin(), available_resources.end(), fleet), available_resources.end());
				}
			}
		 }
	}
}

// 修复需求违反
void Individual::repairDemandViolation() {
	for (auto& [key, value] : _targetAllocation) {
		Target* target = key.first;
		int total_allocation = value;
		int shortage = _targetNeedNum[target] - total_allocation;
		//找到可用的资源
		vector<Fleet*> available_resources;
		map<Fleet*, int> fleetRemain;
		Mount* mount = key.second;
		if (shortage > 0) {
			for (FleetType* fleettype : _schedule->getFleetTypeList()) {
				if (fleettype->getFleetList()[0]->getMountNum(mount) > 0) {
					for (Fleet* fleet : fleettype->getFleetList()) {
						//如果飞机还有剩余容量
						int remaining_capacity = fleet->getMountNum(mount) - _fleetAllocation[make_pair(fleet, mount)];
						if (remaining_capacity > 0) {
							available_resources.push_back(fleet);
						}
						fleetRemain[fleet] = remaining_capacity;
					}
				}
			}
		}
		// 如果没有可用的资源，跳过
		if (available_resources.empty()) continue;

		// 分配短缺的货物
		while (shortage > 0 && !available_resources.empty()) {
			Fleet* fleet;
			fleet = available_resources[rand() % available_resources.size()];
			int remaining_capacity = fleetRemain[fleet];
			int allocation_amount = min(shortage, remaining_capacity);
			if (allocation_amount > 0) {
				_fleetAllocation[make_pair(fleet, mount)] += allocation_amount;
				_targetAllocation[make_pair(target, mount)] += allocation_amount;
				//更新_variables
                tuple<Target*, Mount*, Fleet*> meaning = {target, mount, fleet};
				int idx = _meaningIdx[meaning];
				//cout << "需求不足" << endl;
				//cout<< _variables[idx] << endl;
    //            cout<<allocation_amount<<endl;
				_variables[idx] += allocation_amount;
                //更新目标的剩余需求与资源剩余容量
				shortage -= allocation_amount;
				fleetRemain[fleet] -= allocation_amount;
				//如果飞机剩余容量为0，则从可用资源中移除
				if (fleetRemain[fleet] == 0) {
					available_resources.erase(std::remove(available_resources.begin(), available_resources.end(), fleet), available_resources.end());
				}
			}
		 }
	}
}

// 修复容量违反
void Individual::repairCapacityViolation() {
	for (const auto& [key, value] : _fleetAllocation) {
		Fleet* fleet= key.first;
		Mount* mount = key.second;
		int fleet_allocation = value;
		int fleetCapacity = fleet->getMountNum(mount);
		if (value > fleetCapacity) {
			// 计算需要减少的总量
			int total_reduction = fleet_allocation - fleetCapacity;
			vector<Target*> allocated_targets = _fleetTargetList[fleet];
			vector<Target*> uncoveredTargetsBecauseCapacity;
			while (total_reduction > 0) {
				//随机选择一个目标
				int target_index = rand() % allocated_targets.size();
				Target* target = allocated_targets[target_index];
				// 从allocated_targets中移除该目标
                allocated_targets.erase(allocated_targets.begin() + target_index);
				uncoveredTargetsBecauseCapacity.push_back(target);
				//计算需要减少的总量
				int reduction = min(total_reduction, _fleetAllTargetNum[fleet][target]);
				//更新_variables
				tuple<Target*, Mount*, Fleet*> meaning = {target, mount, fleet};

				
				//if (_meaningIdx.find(meaning) != _meaningIdx.end()) {
				//	cout << "找到了" << endl;
				//}
				//else {
				//	cout << "没找到" << endl;
				//}
				int idx = _meaningIdx[meaning];
				//cout<< "超载" << endl;
				//cout<< _variables[idx] << endl;
				//if (_variables[idx] == 0) {
				//	cout << "!!!!!!!!!!error!!!!!!!!!!" << endl;
				//}
                //cout<<reduction<<endl;
                _variables[idx] -= reduction;
				//cout<<fleet->getName()<<"减少"<<reduction<<endl;
				// 更新分配数量
				_targetAllocation[make_pair(target, mount)] -= reduction;
				_fleetAllocation[make_pair(fleet, mount)] -= reduction;
                _fleetAllTargetNum[fleet][target] -= reduction;
				total_reduction -= reduction;
			}
			// 递归调用需求修复，因为减少分配可能导致需求不满足
			//getAllocation();
			repairDemandViolationforCapacity(uncoveredTargetsBecauseCapacity,mount);
		}
	}
}


void Individual::computeTargetDemand() {
	//存储目标对货物的需求量
	vector <Target*> target_list = _schedule->getTargetList();
	for (int i = 0; i < target_list.size(); i++) {
		Target* target = target_list[i];
		TargetType* type = target->getType();
		vector<TargetPlan*> targetPlanList = type->getTargetPlanList();
		if (targetPlanList.size() == 1) {
			Mount* mount = targetPlanList[0]->getMount();
			int needNum = targetPlanList[0]->getMountNum(mount);
			_targetNeedNum[target] = needNum;
		}
	}
}

void Individual::getAllocation() {
	//清空前序计算结果
	_targetAllocation.clear();
    _fleetAllocation.clear();
    _targetAllFleet.clear();
	_fleetAllTargetNum.clear();
	_targetMount.clear();
	_fleetMount.clear();
	//计算目标收到的货物总数量
	for (int i = 0; i < _variables.size(); i++) {
		Target* target = get<0>(_idxMeaning[i]);
		Mount* mount = get<1>(_idxMeaning[i]);
		Fleet* fleet = get<2>(_idxMeaning[i]);
		if (_variables[i] != 0) {
			//if (_variables[i] > 60) {
			//	cout << "error" << endl;
			//	cout<< fleet ->getName()<<"单机装载量超过最大载重量"<<endl;
			//	cout<< _variables[i]<< endl;
			//}
			//如果目标分配map中没有该目标，则添加该目标
			if (_targetAllocation.find(make_pair(target, mount)) == _targetAllocation.end()) {
				_targetAllocation[make_pair(target, mount)] = _variables[i];
			}
			//如果目标分配map中有该目标，则增加该目标的分配数量
			else {
				_targetAllocation[make_pair(target, mount)] += _variables[i];
			};
			if (_fleetAllocation.find(make_pair(fleet, mount)) == _fleetAllocation.end()) {
				_fleetAllocation[make_pair(fleet, mount)] = _variables[i];
				
			}
			else {
				_fleetAllocation[make_pair(fleet, mount)] += _variables[i];
			};
			_targetAllFleet[target].insert({fleet,_variables[i]});
			_fleetAllTargetNum[fleet].insert({target,_variables[i]} );
			_fleetTargetList[fleet].push_back(target);
			_targetMount[target] = mount;
			_fleetMount[fleet] = mount;
		}
		//else {
		//	_fleetAllocation[make_pair(fleet, mount)] = 0;
		//}
	}
}