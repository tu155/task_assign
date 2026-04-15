#include "Schedule.h"
#include "Util.h"
#include "ResModel.h"
#include "Model.h"
using std::cout;
Schedule::Schedule() {}
Schedule::~Schedule() {}
Mount* Schedule::getMount(string mountName)
{
	for (int i = 0; i < _mountList.size(); i++)
	{
		if (_mountList[i]->getName() == mountName)
		{
			return _mountList[i];
		}
	}
	return NULL;
}

Load* Schedule::getLoad(string loadName)
{
	for (int i = 0; i < _loadList.size(); i++)
	{
		if (_loadList[i]->getName() == loadName)
		{
			return _loadList[i];
		}
	}
	return NULL;
}


Fleet* Schedule::getFleet(string fleetName)
{
	for (int i = 0; i < _fleetList.size(); i++)
	{
		if (_fleetList[i]->getName() == fleetName)
		{
			return _fleetList[i];
		}
	}
	return NULL;
}

FleetType* Schedule::getFleetType(string fleetTypeName)
{
	for (int i = 0; i < _fleetTypeList.size(); i++)
	{
		if (_fleetTypeList[i]->_name == fleetTypeName)
		{
			return _fleetTypeList[i];
		}
	}
	return NULL;
}

//Scenario* Schedule::getScenario(string scenName)
//{
//	for (int i = 0; i < _scenList.size(); i++)
//	{
//		if (_scenList[i]->_name == scenName)
//		{
//			return _scenList[i];
//		}
//	}
//	return NULL;
//}

TargetType* Schedule::getTargetType(string name)
{
	for (int i = 0; i < _targetTypeList.size(); i++)
	{
		if (_targetTypeList[i]->getName() == name)
		{
			return _targetTypeList[i];
		}
	}
	return NULL;
}


MountLoad* Schedule::getMountLoad(Mount* mount, Load* load)
{
	for (int i = 0; i < _mountLoadList.size(); i++)
	{
		if (_mountLoadList[i]->_mount == mount && _mountLoadList[i]->_load == load)
		{
			return _mountLoadList[i];
		}
	}
	return NULL;
}
MountLoad* Schedule::getMountLoadStr(string mountLoad)
{
	for (int i = 0; i < _mountLoadList.size(); i++)
	{
		if ((_mountLoadList[i]->_mount->getName() + _mountLoadList[i]->_load->getName()) == mountLoad)
		{
			return _mountLoadList[i];
		}
	}
	return NULL;
}


Tour* Schedule::getTour(int tourId)
{
	if (_tourMap.find(tourId) != _tourMap.end())
	{
		return _tourMap[tourId];
	}
	return NULL;
}

Scheme* Schedule::getScheme(Fleet* fleet, int aftNum)
{
	for (int i = 0; i < _schemeList.size(); i++)
	{
		if (_schemeList[i]->getFleet() == fleet && _schemeList[i]->getAftNum() == aftNum)
		{
			return _schemeList[i];
		}
	}
	return NULL;
}

Target* Schedule::getTarget(string targetName)
{
	for (int i = 0; i < _targetList.size(); i++)
	{
		if (_targetList[i]->getName() == targetName)
		{
			return _targetList[i];
		}
	}
	return NULL;
}

Target* Schedule::getTargetById(int id)
{
	for (int i = 0; i < _targetList.size(); i++)
	{
		if (_targetList[i]->getId() == id)
		{
			return _targetList[i];
		}
	}
	return NULL;
}
TargetPlan* Schedule::getTargetPlanByMountLoad(vector<pair<MountLoad*, int>>mountLoadList)
{
	for (int i = 0; i < _targetPlanList.size(); i++)
	{
		map<MountLoad*, int> mountLoadMap = _targetPlanList[i]->_mountLoadPlan;
		if (mountLoadList.size() != mountLoadMap.size())
		{
			continue;
		}
		if (mountLoadList.empty())
		{
			return nullptr;
		}
		bool isSame = true;
		for (int j = 0; j < mountLoadList.size(); j++)
		{
			if (mountLoadList[j].second != mountLoadMap[mountLoadList[j].first])
			{
				isSame = false;
			}
		}
		if (isSame)
		{
			return _targetPlanList[i];
		}
	}
	return nullptr;
}

TargetPlan* Schedule::getTargetPlanByName(string name)
{
	for (int i = 0; i < _targetPlanList.size(); i++)
	{
		if (_targetPlanList[i]->name == name)
		{
			return _targetPlanList[i];
		}
	}
	return NULL;
}

MountType* Schedule::getMountType(string name)
{
	for (int i = 0; i < _mountTypeList.size(); i++)
	{
		if (_mountTypeList[i]->name == name)
		{
			return _mountTypeList[i];
		}
	}
	return NULL;
}
//原版，注释
void Schedule::generateSchemes()
{
	if (Util::ifAutoGeneTours)
	{
		for (int i = 0; i < _fleetTypeList.size(); i++)
		{
			cout << "———— Transport: " << _fleetTypeList[i]->getName() << " ————" << endl;
			Scheme* scheme = new Scheme(_fleetTypeList[i]->getFleetList()[0], _targetTypeList, this);
			/*生成总装载量计划*/
			scheme->generateTotalLoadPlan();

			int tarNumPerGrpUb = Util::targetPerGroupNumLimit;//参数，每个编队完成的最多目标数量
			if (Util::ifEvaTargetNumPerGrpUb)
			{
				string configStr = _fleetList[i]->getName();
				if (_tnpgUbMap.find(configStr) != _tnpgUbMap.end())
				{
					tarNumPerGrpUb = _tnpgUbMap[configStr];
				}
			}
			/*依次生成目标类型计划、、*/
			scheme->generateTargetTypePlan(tarNumPerGrpUb);
			scheme->generateAftAssignTotalTour();
			//scheme->generateTimeAssignTotalTour(_slotList);
			cout << "************【Model】Num of tour is " << scheme->getTourList().size() << "**********" << endl;
			_schemeList.push_back(scheme);
			//这里是只拷贝每类fleet的版本
			// --------------------------------------------
			//在末尾插入fleet 类型飞行一圈可选择经过目标组合的所有Tour
			//vector<Tour*> fleet_tour_vector = scheme->getTourList();
			//for (Tour* original_tour : fleet_tour_vector) {
			//	// 创建新的Tour对象，拷贝内容
			//	Tour* new_tour = new Tour(*original_tour);  // 需要实现拷贝构造函数
			//	_tourList.push_back(new_tour);
			//}
			// -----------------------------------------------
			//这里是把所有资源都加入tour的版本，按照资源类型进行扩展
			////在末尾插入fleet 类型飞行一圈可选择经过目标组合的所有Tour
			//vector<Tour*> fleet_tour_vector = scheme->getTourList();

			//for (Tour* original_tour : fleet_tour_vector) {
			//	Fleet*original_fleet =original_tour->getFleet();
			//	FleetType* fleetType= original_fleet->getType();
			//	vector<Fleet*> fleetList = fleetType->getFleetList();
			//	for (Fleet* fleet : fleetList) {
			//		// 创建新的Tour对象，拷贝内容
			//		Tour* new_tour = new Tour(*original_tour);  // 需要实现拷贝构造函数
			//		new_tour->setFleet(fleet);
			//		_tourList.push_back(new_tour);
			//	}
			//}
			// 因为一个资源只能飞一次，所以在这里把force0扩展到其他force
			vector<Tour*> fleet_tour_vector = scheme->getTourList();
			setFleetTypeTourNumMap(_fleetTypeList[i], fleet_tour_vector.size());
			//初始化
			if (fleet_tour_vector.size() == 0) {
				continue;
			}
			Fleet* fleetFlag = fleet_tour_vector[0]->getFleet();
			int fleetFlagIdx = 0;
			int num = 0;
			vector<Fleet*> fleetList = fleetFlag->getType()->getFleetList();
			// 创建新的Tour对象，拷贝内容
			Tour* new_tour = new Tour(*fleet_tour_vector[0]);  // 需要实现拷贝构造函数
			_tourList.push_back(new_tour);
			for (int i = 1; i < fleet_tour_vector.size(); i++) {
				Tour* original_tour = fleet_tour_vector[i];
				Fleet*original_fleet = original_tour->getFleet();
				if (original_fleet == fleetFlag) {
					num++;
					// 创建新的Tour对象，拷贝内容
					Tour* new_tour = new Tour(*original_tour);  // 需要实现拷贝构造函数
					//找到fleetFlag在对应类型的fleetlist中的索引，设置下一个索引的资源为新tour的资源					
					new_tour->setFleet(fleetList[fleetFlagIdx+num]);
					_tourList.push_back(new_tour);
				}
				else {
					fleetFlag = original_fleet; 
                    fleetFlagIdx = i;
					int num = 0;
					fleetList.clear();
					fleetList = fleetFlag->getType()->getFleetList();
					// 创建新的Tour对象，拷贝内容
					Tour* new_tour = new Tour(*original_tour);  // 需要实现拷贝构造函数
					_tourList.push_back(new_tour);
				}
			}
			//_tourList.insert(_tourList.end(), fleet_tour_vector.begin(), fleet_tour_vector.end());
			//_tourList.insert(_tourList.end(), scheme->getTourList().begin(), scheme->getTourList().end());
		}
	}

	//vector<Tour*> givenTourList = geneGivenTourList();

	cout << "===========【Model】Total Num of tour is:" << _tourList.size() << "============" << endl;
	//cout << "#Add Given Tours: " << givenTourList.size() << endl;
	//_tourList.insert(_tourList.end(), givenTourList.begin(), givenTourList.end());
	for (int i = 0; i < _tourList.size(); i++)
	{
		_tourList[i]->computeLoadNum();
		_tourList[i]->computeMountNum();
		_tourList[i]->computeMountLoadNum();
		//_tourList[i]->computeCost();
		_tourList[i]->computeTimeMap();
		_tourList[i]->computeCostNew();
	}
}

//void Schedule::generateSchemes()
//{
//	if (Util::ifAutoGeneTours)
//	{
//		for (int i = 0; i < _fleetTypeList.size(); i++)
//		{
//			cout << "———— Transport: " << _fleetTypeList[i]->getName() << " ————" << endl;
//			for (int j = 0; j < _fleetTypeList[i]->getFleetList().size(); j++) {
//				Fleet* fleet = _fleetTypeList[i]->getFleetList()[j];
//				cout << "———— Resource: " << fleet->getName() << " ————" << endl;
//				Scheme* scheme = new Scheme(fleet, _targetTypeList, this);
//				/*生成总装载量计划*/
//				scheme->generateTotalLoadPlan();
//
//				int tarNumPerGrpUb = Util::targetPerGroupNumLimit;//参数，每个编队完成的最多目标数量
//				if (Util::ifEvaTargetNumPerGrpUb)
//				{
//					string configStr = _fleetList[i]->getName();
//					if (_tnpgUbMap.find(configStr) != _tnpgUbMap.end())
//					{
//						tarNumPerGrpUb = _tnpgUbMap[configStr];
//					}
//				}
//				/*依次生成目标类型计划、、*/
//				scheme->generateTargetTypePlan(tarNumPerGrpUb);
//				scheme->generateAftAssignTotalTour();
//				//scheme->generateTimeAssignTotalTour(_slotList);
//				cout << "************【Model】Num of tour is " << scheme->getTourList().size() << "**********" << endl;
//				_schemeList.push_back(scheme);
//				//加入tourlist
//				for (Tour* tour : scheme->getTourList()) {
//					_tourList.push_back(tour);
//				}
//			}
//		}
//	}
//	cout << "===========【Model】Total Num of tour is:" << _tourList.size() << "============" << endl;
//	//cout << "#Add Given Tours: " << givenTourList.size() << endl;
//	//_tourList.insert(_tourList.end(), givenTourList.begin(), givenTourList.end());
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		_tourList[i]->computeLoadNum();
//		_tourList[i]->computeMountNum();
//		_tourList[i]->computeMountLoadNum();
//		//_tourList[i]->computeCost();
//		_tourList[i]->computeTimeMap();
//		_tourList[i]->computeCostNew();
//	}
//}

int Schedule::computeSlotNum()
{//TO DO 基于已经被occupy的频点和突击区重新估计最大的时间范围
	int totalNum = 0;
	//获取schedule的targetlist
	for (int i = 0; i < _targetList.size(); i++)
	{
		Target* target = _targetList[i];
		vector<TargetPlan* > targetPlanList = target->getType()->getTargetPlanList();
		int maxOccupyWeaponNum = 0;
		//遍历该目标的所有计划方案
		for (int j = 0; j < targetPlanList.size(); j++)
		{
			map<MountLoad*, int> mountLoadMap = targetPlanList[j]->_mountLoadPlan;
			map<MountLoad*, int>::iterator iter = mountLoadMap.begin();
			int occupyWeaponNum = 0;
			// 计算当前方案需要的武器数量
			for (iter; iter != mountLoadMap.end(); iter++)
			{
				{
					occupyWeaponNum += (*iter).second;
				}
			}
			if (occupyWeaponNum > maxOccupyWeaponNum)
			{
				maxOccupyWeaponNum = occupyWeaponNum;
			}
		}
		// 累加所有目标的最大挂载需求
		totalNum += maxOccupyWeaponNum;
	}

	int maxWeaponNumPerAft = 0;
	int maxContiWaitTime = 0;
	//遍历所有飞机（运力）
	for (int i = 0; i < _fleetList.size(); i++)
	{
		Fleet* fleet = _fleetList[i];
		vector<pair<Mount*, int>> mountPlanList = fleet->getMountPlanList();
		//遍历运力的所有装载计划
		for (int j = 0; j < mountPlanList.size(); j++)
		{
			{
				if (mountPlanList[j].second > maxWeaponNumPerAft)
				{
					// 找出装载计划中的飞机最大挂载数量
					maxWeaponNumPerAft = mountPlanList[j].second;
				}
			}
		}
	}
	if (maxWeaponNumPerAft > 0)
	{
		//飞机释放挂载的最大次数
		maxContiWaitTime = max((int)ceil((double)maxWeaponNumPerAft / 2.0), 1);
	}
	if (Util::ifContiOrderSameTime)
	{
		//每次释放需要7个时间槽？
		return 7 * maxContiWaitTime + totalNum / (2 * Util::generalSlotLimit.first);
	}
	else
	{
		return 7 * maxContiWaitTime + totalNum / (2 * Util::generalSlotLimit.first) + _targetList.size();
	}
}
//逆向排序，计算每个主目标的最晚开始时间槽
//void Schedule::computeSlotUB()
//{
//	cout << "************************ Generate Schemes for preModel *****************************" << endl;
//
//	map<string, int> batchMap;// 记录每个批次的最晚开始时间
//	vector<Target* > targetList = _targetList;// 获取目标列表
//	sort(targetList.begin(), targetList.end(), Target::cmpByPrio);//按照目标优先级升序排列
//	map<Slot*, int> remainSlotNum;// 每个时间槽剩余的飞机容量
//	for (int i = 0; i < _slotList.size(); i++)
//	{
//		remainSlotNum[_slotList[i]] = _slotList[i]->getCap();//初始化为最大容量
//	}
//
//	map<Slot*, map<MountType*, int>> remainRegionGrpNum = _regionGrpCapMap; // 每个时间槽每种挂载类型的剩余编组容量
//
//	int preMTBgnInd = 0;
//	int crtMTBgnInd = 0;
//	for (int i = 0; i < targetList.size(); i++)
//	{
//		bool ifSameMT = true;
//		if (i > 0 && targetList[i]->getMainTarget() != targetList[i - 1]->getMainTarget())
//		{
//			ifSameMT = false;
//			preMTBgnInd = crtMTBgnInd;
//		}
//		TargetPlan* longTimeConsumePlan = targetList[i]->getType()->getLongTimeConsumePlan();
//
//		map<MountLoad*, int> mountLoadPlan = longTimeConsumePlan->_mountLoadPlan;
//		map<MountLoad*, int>::iterator iter = mountLoadPlan.begin();
//		for (iter; iter != mountLoadPlan.end(); iter++)
//		{
//			MountLoad* mountLoad = (*iter).first;
//			Mount* mount = mountLoad->_mount;
//			int mountNum = (*iter).second;
//			int minMountNumPerAft = 40;
//			for (int q = 0; q < _fleetList.size(); q++)
//			{
//				if (_fleetList[q]->getMountNum(mount) > 0 && _fleetList[q]->getMountNum(mount) < minMountNumPerAft)
//				{
//					minMountNumPerAft = _fleetList[q]->getMountNum(mount);//根据机型获取飞机的挂载数量
//				}
//			}
//			vector< int> occupyList;
//			int bufferTime = mount->getMountType()->intervalTime / Util::generalSlotLimit.second;
//			int aftNum = ceil((double)mountNum / (double)minMountNumPerAft);//根据挂载数量计算需要多少架飞机,并向上取整
//			int remainMount = mountNum;//初始剩余挂载数量为挂载计划中的数量
//			while (remainMount > 0)
//			{
//				int aftNumSub = min(aftNum, (int)ceil((double)remainMount / 2));//计算一次卸货的飞机数量
//				occupyList.push_back(aftNumSub);//记录卸载的飞机数量
//				remainMount -= aftNumSub * 2;//每次每个飞机卸掉两个挂载
//			}
//			for (int j = 0; j < occupyList.size(); j++)
//			{
//				if (mount->isOccupy())
//				{
//				}
//			}
//			for (int j = preMTBgnInd; j < _slotList.size(); j++)
//			{
//				bool ifPush = true;
//				//遍历卸货次数
//				for (int q = 0; q < occupyList.size(); q++)
//				{
//					if (mount->isOccupy() && remainSlotNum[_slotList[j + bufferTime * q]] < occupyList[q])//剩余的slot小于卸载的挂载数量
//					{
//						ifPush = false;
//					}
//				}
//				if (remainRegionGrpNum[_slotList[j]][mount->getMountType()] < 1)
//				{
//					ifPush = false;
//				}
//
//				if (ifPush)
//				{
//					for (int q = 0; q < occupyList.size(); q++)
//					{
//						if (mount->isOccupy())
//						{
//							remainSlotNum[_slotList[j + bufferTime * q]] = remainSlotNum[_slotList[j + bufferTime * q]] - occupyList[q];
//						}
//					}
//					remainRegionGrpNum[_slotList[j]][mount->getMountType()] = remainRegionGrpNum[_slotList[j]][mount->getMountType()] - 1;
//
//					if (batchMap.find(targetList[i]->getMainTarget()->getBatchId()) != batchMap.end())
//					{
//						if (batchMap[targetList[i]->getMainTarget()->getBatchId()] < j)
//						{
//							batchMap[targetList[i]->getMainTarget()->getBatchId()] = j;
//						}
//					}
//					else
//					{
//						batchMap[targetList[i]->getMainTarget()->getBatchId()] = j;
//					}
//					if (!ifSameMT && iter == mountLoadPlan.begin())
//					{
//						crtMTBgnInd = j;
//					}
//					else if (j < crtMTBgnInd)
//					{
//						crtMTBgnInd = j;
//					}
//					break;
//				}
//			}
//		}
//	}
//
//	int preMTOrder = 0;
//	for (int i = 0; i < _mainTargetList.size(); i++)
//	{
//		if (i != 0 && batchMap[targetList[i]->getMainTarget()->getBatchId()] < preMTOrder)
//		{
//		}
//		else
//		{
//			preMTOrder = batchMap[targetList[i]->getMainTarget()->getBatchId()];
//		}
//		_mainTargetList[i]->setLatestBgnTime(preMTOrder);
//	}
//	//for (int i = 0; i < targetList.size(); i++)
//	//{
//	//	targetList[i]->getMainTarget()->setLatestBgnTime(batchMap[targetList[i]->getMainTarget()->getBatchId()]);
//	//}
//
//	for (int i = 0; i < targetList.size(); i++)
//	{
//		cout << targetList[i]->getName() << " " << targetList[i]->getMainTarget()->getId() << " " << targetList[i]->getMainTarget()->getBatchId() << " " << targetList[i]->getMainTarget()->getLatestBgnTime() << endl;
//	}
//}
//
//// 计算每个具体目标的“最早可开始时间下界”
//void Schedule::computeSlotLB()
//{
//	// 建一张“主目标 → 顺序号”映射，用来快速查号
//	map<MainTarget*, int> mainTargetOrder;
//	int order = 0;
//
//	// 遍历已经排好序的主目标列表（_mainTargetList 已提前按业务规则排序）
//	// 注意：只遍历到 size-1，因为每次比较 i 与 i+1
//	for (int i = 0; i < _mainTargetList.size() - 1; i++)
//	{
//		// 先把当前主目标标记为当前 order顺序
//		mainTargetOrder[_mainTargetList[i]] = order;
//
//		// 如果下一个主目标的 order 与当前不同，则递增序号
//		if (_mainTargetList[i]->getOrder() != _mainTargetList[i + 1]->getOrder())
//		{
//			order++;
//		}
//	}
//
//	// 把刚刚算出的顺序号回写到每个具体目标
//	// mainTargetOrder[...] 得到的就是该目标所属主目标的“排序序号”
//	for (int i = 0; i < _targetList.size(); i++)
//	{
//		_targetList[i]->setEarliestBgnTime(mainTargetOrder[_targetList[i]->getMainTarget()]);
//	}
//}

void Schedule::generateSlotList()
{
	int numSlot = computeSlotNum();
	cout << "Num of slot is " << numSlot << endl;
	if (numSlot > 0)
	{
		for (int i = 0; i < numSlot; i++)
		{
			Slot* slot = new Slot(Util::generalSlotLimit.first, i * Util::generalSlotLimit.second, (i + 1) * Util::generalSlotLimit.second);
			for (int j = 0; j < slot->getCap(); j++)
			{
				slot->pushPoint(j + 1);
			}
			_slotList.push_back(slot);
		}
	}

	for (int i = 0; i < _slotList.size(); i++)
	{
		for (int j = 0; j < _mountTypeList.size(); j++)
		{
			_regionGrpCapMap[_slotList[i]][_mountTypeList[j]] = _mountTypeList[j]->grpNumUb;
		}
	}
}

void Schedule::initResAssign()
{
	cout << "************************ Init Resouce Assign *****************************" << endl;
	vector<Scheme* > schemeList;
	vector<Tour*> totalTourList;
	for (int i = 0; i < _fleetTypeList.size(); i++)
	{
		cout << "———— Transport: " << _fleetTypeList[i]->getName() << " ————" << endl;
		//构造函数，把机型和编队数量传入scheme类中，保存为成员变量，目标类型列表，目标点列表，this是指向当前对象(即ZZSchedule)的指针,
		Scheme* scheme = new Scheme(_fleetTypeList[i]->getFleetList()[0], _targetTypeList, this);
		scheme->generateTotalLoadPlan();
		scheme->generateTargetTypePlan(Util::targetPerGroupNumLimit);
		scheme->generateAftAssignTotalTour();

		cout << "**********【ResModel】Num of tour is " << scheme->getTourList().size() << "**********" << endl;
		schemeList.push_back(scheme);
		//在末尾插入fleet 类型飞行一圈可选择经过目标组合的所有Tour
		vector<Tour*> fleet_tour_vector = scheme->getTourList();
		totalTourList.insert(totalTourList.end(), fleet_tour_vector.begin(), fleet_tour_vector.end());
	}

	//totalTourList.insert(totalTourList.end(), _givenTourList.begin(), _givenTourList.end());
	cout << "===========【ResModel】Total Num of tour is:" << totalTourList.size() << "============" << endl;

	vector<Tour*> newTourList;
	for (int i = 0; i < totalTourList.size(); i++)
	{
		totalTourList[i]->computeLoadNum();
		totalTourList[i]->computeMountNum();
		totalTourList[i]->computeMountLoadNum();
		totalTourList[i]->computeCost();

		Mount* mount = totalTourList[i]->getOperTargetList()[0]->getMountLoadPlan()[0].first->_mount;
		vector<OperTarget*> operTargetList = totalTourList[i]->getOperTargetList();
		int mountNum = 0;

		int totalMountNum = totalTourList[i]->getFleet()->getMountNum(mount);

		map<MountLoad*, int> remainMList = totalTourList[i]->getRemainMLList();
		map<MountLoad*, int>::iterator iter = remainMList.begin();
		for (iter; iter != remainMList.end(); iter++)
		{
			mountNum += (*iter).second;
		}

		newTourList.push_back(totalTourList[i]);
	}
	cout << "************************ Solve ResModel *****************************" << endl;
	ResModel* resModel = new ResModel(this);
	vector<Tour*> solnTourList;
	resModel->init();
	resModel->addColsByGrp(newTourList);
	resModel->solveIP();
	solnTourList = resModel->setSolnInfor();//返回选中的轮次
	cout << "SolnTourList " << solnTourList.size() << endl;
	cout << "************************* Solve ResModel Finish # Start to clear ******************************************" << endl;

	for (int i = 0; i < schemeList.size(); i++)
	{
		vector<Tour*> tourList = schemeList[i]->getTourList();
		for (int j = 0; j < tourList.size(); j++)
		{
			vector<OperTarget* > operTargetList = tourList[j]->getOperTargetList();
			for (int q = 0; q < operTargetList.size(); q++)
			{
				delete operTargetList[q];
			}
			delete tourList[j];
		}
		delete schemeList[i];
		schemeList[i]->getTourList().clear();
	}
}

bool Schedule::checkSlotMapFeasible(Slot* slot, int aftNum, Mount* mount)
{
	if (slot->getCap() < aftNum || _regionGrpCapMap[slot][mount->getMountType()] < 1)
	{
		return false;
	}
	return true;
}

/*不需要目标偏好计划，所以不需要删除tour，注释这个函数*/
//void Schedule::reduceTours()
//{
//	vector<Tour*> tourListAftReduce;
//	vector<Tour*> backUpTourList;
//
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		vector<OperTarget*> operTargetList = _tourList[i]->getOperTargetList();
//		bool ifRemain = true;
//		for (int j = 0; j < operTargetList.size(); j++)
//		{
//			TargetPlan* targetPlan = operTargetList[j]->getTarget()->getPrefTargetPlan();
//			if (targetPlan != NULL)
//			{
//				vector<pair<MountLoad*, int>> mountLoadPlan = operTargetList[j]->getMountLoadPlan();
//				for (int q = 0; q < mountLoadPlan.size(); q++)
//				{
//					if (targetPlan->getMountLoadNum(mountLoadPlan[q].first) == 0)
//					{
//						ifRemain = false;
//						break;
//					}
//				}
//				if (!ifRemain)
//				{
//					break;
//				}
//			}
//			//if (operTargetList[j]->getTarget()->getPreferBase() != NULL)
//			//{
//			//	if (operTargetList[j]->getTarget()->getPreferBase() != _tourList[i]->getBase())
//			//	{
//			//		ifRemain = false;
//			//	}
//			//}
//		}
//		if (ifRemain)
//		{
//			tourListAftReduce.push_back(_tourList[i]);
//		}
//		else
//		{
//			backUpTourList.push_back(_tourList[i]);
//		}
//	}
//	_tourList = tourListAftReduce;
//	_backUpTourList = backUpTourList;
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		//_tourList[i]->print();
//	}
//	cout << "After reduce: " << tourListAftReduce.size() << endl;
//	cout << "Backup tour size: " << _backUpTourList.size() << endl;
//}
void Schedule::clearSoln()
{
	_uncoverTList.clear();
	_coverTList.clear();

	for (int t = 0; t < _targetList.size(); t++)
	{
		_targetList[t]->clearSoln();
	}

	for (int t = 0; t < _mainTargetList.size(); t++)
	{
		_mainTargetList[t]->clearSoln();
	}

	for (int i = 0; i < _schemeList.size(); i++)
	{
		vector<Tour*> tourList = _schemeList[i]->getTourList();
		for (int j = 0; j < tourList.size(); j++)
		{
			vector<OperTarget* > operTargetList = tourList[j]->getOperTargetList();
			for (int q = 0; q < operTargetList.size(); q++)
			{
				delete operTargetList[q];
			}
			delete tourList[j];
		}
		_schemeList[i]->getTourList().clear();
		delete _schemeList[i];
	}
	_schemeList.clear();

	for (int i = 0; i < _tourList.size(); i++)
	{
		if (_tourList[i]->getTourType() != BOTHFIX_TOUR && _tourList[i]->getTourType() != BOTHFIX_CP_TOUR)
		{
			delete _tourList[i];
		}
	}
	_tourList.clear();

	//for (int i = 0; i < _backUpTourList.size(); i++)
	//{
	//	delete _backUpTourList[i];
	//}
	//_backUpTourList.clear();
}
