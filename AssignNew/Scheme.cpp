#include "Scheme.h"
#include "Fleet.h"
#include "TargetType.h"
#include "Target.h"
#include "Mount.h"
#include "Load.h"
#include "Tour.h"
#include "OperTarget.h"
#include "Schedule.h"
#include "OperTarget.h"
//#include "DataIO.h"
using std::cout;
template<typename T>
ostream& operator<<(ostream& os, const vector<T>& v) {
	os << "[";
	for (size_t i = 0; i < v.size(); ++i) {
		os << v[i] << (i + 1 == v.size() ? "" : ", ");
	}
	os << "]";
	return os;
}
// 构造函数，初始化Scheme对象
Scheme::Scheme(Fleet* fleet, vector<TargetType* > targetTypeList, Schedule* schedule)
	:_fleet(fleet), _targetTypeList(targetTypeList),  _schedule(schedule)
{
	// 遍历目标类型列表
	for (int i = 0; i < _targetTypeList.size(); i++)
	{
		TargetType* targetType = _targetTypeList[i];
		// 获取目标计划列表
		vector<TargetPlan* > targetPlanList = targetType->getTargetPlanList();
		// 遍历目标计划列表
		for (int j = 0; j < targetPlanList.size(); j++)
		{
			TargetPlan* targetPlan = targetPlanList[j];
			// 遍历装载计划
			map<MountLoad*, int>::iterator  iter = (targetPlan->_mountLoadPlan).begin();
			for (iter; iter != targetPlan->_mountLoadPlan.end(); iter++)
			{
				// 获取装载和装载的ID，并插入到_targetDmd中
				string dmdName = to_string((*iter).first->_mount->getId()) + ";" + to_string((*iter).first->_load->getId());
				_targetDmd.insert(dmdName);
			}
		}
	}
}

Scheme::~Scheme()
{
}

bool Scheme::cmp(pair<TargetType*, map<string, int>> pair1, pair<TargetType*, map<string, int>> pair2)
{	//比较函数，输入目标类型的货物需求计划，输出布尔值True或者False
	bool prio = false;
	//比较逻辑：如果计划1的目标类型ID比计划2的小，更好
	//如果计划1的目标类型ID和计划2的相等，涉及的货物类型更多，则更好
	if (pair1.first->getId() < pair2.first->getId())
	{
		prio = true;
	}
	else if (pair1.first->getId() == pair2.first->getId())
	{
		if (pair1.second.size() > pair2.second.size())
		{
			prio = true;
		}
	}
	return prio;
}



void Scheme::generateTotalLoadPlan()
{
	// 找到机型对应的挂载计划列表：数量为可挂载位置的最大数量
	//vector<pair<ZZMount*, int>> mountPlanList = _fleet->getMountPlanList();
	int numMount = _fleet->getMountPlanList().size();//运力的挂载计划的数量
	vector<int> mountList;
	for (int i = 0; i < numMount; i++)//i就是索引，表示第i种挂载计划
	{
		// 定义一个空的挂载计划
		vector<int > mountPlan;//元素为可挂载的位置数量
		// 将挂载计划添加到挂载计划列表中
		mountPlan.push_back(i);
		std::cout << "mountplan" << endl;
		std:: cout << mountPlan << endl;
		_mountPlanList.push_back(mountPlan);
		std::cout << "mountplanlist" << endl;
		std::cout << _mountPlanList << endl;
	}


	// 如果是调试模式
	if (Util::isDebug)
	{
		// 输出挂载计划列表的大小
		cout << "generateTotalLoadPlan::findMount " << _mountPlanList.size() << endl;
	}

	for (int i = 0; i < _mountPlanList.size(); i++)//i对应_mountPlanList的行，即第i种编队方案
	{
		// 定义一个空的挂载计划
		map<Mount*, int> mountPlan;
		// 定义一个标志位，表示是否存在某个挂载计划
		bool ifExist = true;
		// 遍历挂载计划
		for (int j = 0; j < _mountPlanList[i].size(); j++)//j对应_mountPlanList的列，即第j辆飞机
		{
			// 检查是否有挂点-载荷的装载计划,数量指的是编队内这类挂载的挂点总数
			Mount* mount = _fleet->getMountPlanList()[_mountPlanList[i][j]].first;
			if (mount->getLoadList().size() == 0)
			{
				ifExist = false;
			}
			// 获取挂载数量，将挂载数量累加，计算这个编队的挂载载荷总量
			int num = _fleet->getMountPlanList()[_mountPlanList[i][j]].second;
			if (mountPlan.find(mount) != mountPlan.end())
			{
				mountPlan[mount] = mountPlan[mount] + num;
			}
			// 如果挂载计划中不存在该挂载，将挂载数量设为该挂载的数量
			else
			{
				mountPlan[mount] = num;
			}
		}
		// 如果存在某个挂载计划
		if (ifExist)
		{
			// 将挂载计划添加到挂载合并计划列表中
			_mountMergeClassPlanList.push_back(mountPlan);
		}
	}
	//调整_loadMergePlanClassList大小，使其与_mountMergeClassPlanList大小一致，每个挂点只能挂一个负载，所以两者的数量要相当
	_loadMergePlanClassList.resize(_mountMergeClassPlanList.size());
	// 遍历挂载合并计划列表，即编队的装载计划列表
	for (int i = 0; i < _mountMergeClassPlanList.size(); i++)
	{
		// 获取挂载合并计划列表
		map<Mount*, int> mountPlanList = _mountMergeClassPlanList[i];
		cout << "mountplanlist" << endl;
		map<Mount*, int>::iterator iter = mountPlanList.begin();
		// 遍历挂载合并计划列表
		for (iter; iter != mountPlanList.end(); iter++)
		{
			// 定义一个空的负载列表
			vector<Load*> loadList;
			// 调用findLoad函数，根据mount找到对应的Loadlist负载列表
			//iter是Mount指针与num组成的map，second是计划中可选的数量， (*iter).first->getLoadList().size() - 1是载荷的最大数量
			findLoad(i, (*iter).first, (*iter).second, (*iter).first->getLoadList().size() - 1, loadList);

		}
	}
	// 如果是调试模式
	if (Util::isDebug)
	{
		// 输出挂载合并计划列表的大小
		cout << "generateTotalLoadPlan::findLoad " << _mountMergeClassPlanList.size() << endl;
	}
	// 输出挂载合并计划列表的大小
	cout << "generateTotalLoadPlan::findLoad " << _mountMergeClassPlanList.size() << endl;
	// 遍历挂载合并计划列表
	for (int i = 0; i < _mountMergeClassPlanList.size(); i++)
	{
		// 定义一个空的负载计划
		map<string, int> loadPlan;
		// 调用findLoadPlan函数，查找负载计划
 		findLoadPlan(i, _mountMergeClassPlanList[i].size() - 1, loadPlan);
	}

	// 输出总负载计划的大小
	cout << "generateTotalLoadPlan::findLoadPlan " << _totalLoadPlan.size() << endl;
	if (_totalLoadPlan.size()==0) {
		cout << "No feasible load plan found." << endl;
	}
	// 删除不符合要求的挂载计划
	for (int i = 0; i < _totalLoadPlan.size(); i++)
	{
		map<string, int> loadPlan = _totalLoadPlan[i];
		// 定义一个迭代器
		map<string, int>::iterator iter = loadPlan.begin();
		// 定义一个标志位，表示是否保存
		bool ifSave = true;
		// 遍历总负载计划
		for (iter; iter != loadPlan.end(); iter++)
		{
			// 如果负载数量不是偶数或者目标需求中不存在该负载
			if ((*iter).second % 2 != 0 || _targetDmd.find((*iter).first) == _targetDmd.end())
			{
				// 将标志位设为false
				ifSave = false;
				// 跳出循环
				break;
			}
		}
		// 如果不保存
		if (!ifSave)
		{
			// 从总负载计划中删除该负载计划
			_totalLoadPlan.erase(_totalLoadPlan.begin() + i);
			// 将索引减1
			i--;
		}
	}
	// 输出总负载计划的大小
	cout << "Total load plan: " << _totalLoadPlan.size() << endl;

}

void Scheme::findMount(int num, int numMount, vector<int> mountList)
{
	/*
	 输入：numMount:挂载类型的数量,num要选择的挂载类型的数量上限；mountList：已经选择的挂载类型索引列表
	 输出：隐式修改_mountPlanList，表示挂载组合集合
	 */
	if (num == 1)
	{
		for (int i = 0; i <= numMount; i++)
		{
			vector<int > newMountList = mountList;
			newMountList.push_back(i);
			_mountPlanList.push_back(newMountList);
		}
	}
	else
	{
		for (int i = 0; i <= numMount; i++)
		{
			vector<int > newMountList = mountList;
			newMountList.push_back(i);
			findMount(num - 1, i, newMountList);
		}
	}
}
// findLoad 函数：递归地枚举特定挂载下所有可能的载荷组合
void Scheme::findLoad(int indexI, Mount* mount, int numMount, int numLoad, vector<Load*> loadList)
{
	/*
	输入：indexI装载计划索引； mount挂载指针；numMount需要分配的挂载数量；numLoad可选择的载荷最大索引；loadList载荷列表
	输出：隐式修改loadlist,并添加到_loadMergePlanClassList
	*/
	// 如果还需要分配的挂载数量为1，则直接枚举当前挂载下的所有载荷
	if (numMount == 1)
	{
		for (int i = 0; i <= numLoad; i++)
		{
			// 定义一个新的载荷列表
			vector<Load* > newMountList = loadList;
			// 将当前挂载下的第i个载荷添加到新的载荷列表中
			newMountList.push_back(mount->getLoadList()[i]);
			
			//cout << "loadlist_NUM_SUM：" << a + newMountList.size() << endl;
			// 将新的载荷列表添加到载荷合并计划列表中
			_loadMergePlanClassList[indexI][mount].push_back(newMountList);
		}
	}
	else
	{
		// 如果还需要分配的挂载数量大于1，则递归地枚举当前挂载下的所有载荷
		for (int i = 0; i <= numLoad; i++)
		{
			vector<Load* > newMountList = loadList;//创建新的载荷列表
			newMountList.push_back(mount->getLoadList()[i]);//添加当前载荷
			//cout << newMountList << endl;
			findLoad(indexI, mount, numMount - 1, i, newMountList);//递归调用，继续枚举下一个
		}
	}
}


void Scheme::findLoadPlan(int indexI, int numMount, map<string, int> loadPlan)
{
	//由后向前回溯当numMount为0时，递归结束
	if (numMount == 0)
	{
		map<Mount*, vector<vector<Load* >>>::iterator iter = _loadMergePlanClassList[indexI].begin();
		for (int i = 0; i < numMount; i++)//当numMount为0时，循环不执行
		{
			iter++;
		}
		Mount* mount = (*iter).first;
		vector<vector<Load* >> loadPlanList = (*iter).second;
		for (int i = 0; i < loadPlanList.size(); i++)
		{
			map<string, int> newLoadPlan = loadPlan;
			//遍历一个loadplan,计算mount-Load的总数，保存在_totalLoadPlan中
			for (int j = 0; j < loadPlanList[i].size(); j++)
			{
				Load* load = loadPlanList[i][j];
				string name = to_string(mount->getId()) + ";" + to_string(load->getId());
				if (newLoadPlan.find(name) != newLoadPlan.end())
				{
					newLoadPlan[name] = newLoadPlan[name] + 1;
				}
				else
				{
					newLoadPlan[name] = 1;
				}
			}
			//_totalLoadPlan长度同loadPlanList的长度
			_totalLoadPlan.push_back(newLoadPlan);
		}
	}
	else
	{
		map<Mount*, vector<vector<Load* >>>::iterator iter = _loadMergePlanClassList[indexI].begin();
		//获取最后一个mount的索引，由后向前递归
		for (int i = 0; i < numMount; i++)
		{
			iter++;
		}
		Mount* mount = (*iter).first;
		vector<vector<Load* >> loadPlanList = (*iter).second;
		for (int i = 0; i < loadPlanList.size(); i++)
		{
			map<string, int> newLoadPlan = loadPlan;
			for (int j = 0; j < loadPlanList[i].size(); j++)
			{
				Load* load = loadPlanList[i][j];
				string name = to_string(mount->getId()) + ";" + to_string(load->getId());

				if (newLoadPlan.find(name) != newLoadPlan.end())
				{
					newLoadPlan[name] = newLoadPlan[name] + 1;
				}
				else
				{
					newLoadPlan[name] = 1;
				}
			}
			findLoadPlan(indexI, numMount - 1, newLoadPlan);
		}
	}
}


// --------------------------------------------------------------------------------
// generateTargetTypePlan(int maxTNum)
//           在“最多 maxTNum 种目标类型”的搜索空间里，targetPerGroupNumLimit，每个编队的最大可执行目标数量是输入参数
//           为每一份弹药方案找出“弹药全部用完、且小编队一次装不完”的可行拆分
//---------------------------------------------------------------------------------
void Scheme::generateTargetTypePlan(int maxTNum)
{
	// -------------------------------------------------
	// 1) 生成目标计划
	// -------------------------------------------------
	vector<TargetType*> tpList;                      // 临时容器
	int numTP = _schedule->getTargetTypeList().size(); // 一共有多少种目标类型
	cout << "numTP: " << numTP << endl;

	for (int i = 1; i <= maxTNum; i++)                 // 枚举 1~maxTNum
	{
		tpList.clear();                                // 清空复用
		//“从 0..numTP-1 中选 i 个”可重复的组合，例如：maxTNum=5，numTP=5，则组合数为C51+C62+C73+C84+C95=251
		// 结果直接压入成员变量 _tpComList
		findTP(tpList, i, numTP - 1);
	}

	// 日志：当前已经生成了多少个目标计划
	cout << "Sum TargetTypeListList " << _tpComList.size() << endl;
	cout << "Step0-0 running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

	// -------------------------------------------------
	// 2) 为每一份弹药方案，尝试在所有目标计划里做拆分
	//    并把“弹药恰好用完”的方案压入 _targetTypePlanList
	// -------------------------------------------------
	for (int i = 0; i < _totalLoadPlan.size(); i++)
	{
		map<string, int> loadPlan = _totalLoadPlan[i]; // 编队的指定 <货箱,货物: 数量>装载计划
		for (int j = 0; j < _tpComList.size(); j++)
		{
			vector<TargetType*> subTPList = _tpComList[j]; // 编队的指定<目标1类型，目标2类型。。。>目标计划
			int numSubTP = subTPList.size();                 // 执行目标数量
			vector<pair<TargetType*, map<string, int>>> subTPPlanList;// 拆分后的方案

			// 递归回溯：把 loadPlan装载计划 拆到 subTPPList目标计划中，留下需求满足计划不同与如果相同则更多满足目标需求的需求匹配计划
			//保存在_targetTypePlanList
			findTTPlan(subTPList, loadPlan, subTPPlanList, numSubTP, 0);
		}
	}

	// -------------------------------------------------
	// 4) 遍历所有已生成的目标类型载荷分配方案，执行“小编队一次带走就丢弃”的剪枝
	// -------------------------------------------------
	vector<vector<pair<TargetType*, map<string, int>>>> newTargetTypePlanList;
	for (int i = 0; i < _targetTypePlanList.size(); i++)
	{
		vector<pair<TargetType*, map<string, int>>> targetTypePlan = _targetTypePlanList[i];

		// 4-a) 收集该方案所有“挂载-载荷”数量
		vector<int> mountNumList;
		Mount* mount = NULL;
		for (int j = 0; j < targetTypePlan.size(); j++)
		{
			map<string, int> subLoadPlan = targetTypePlan[j].second;
			map<string, int>::iterator iter = subLoadPlan.begin();
			for (iter; iter != subLoadPlan.end(); iter++)
			{
				vector<string> loadMountStrList;
				string loadMountStr = (*iter).first;
				_split(loadMountStr, loadMountStrList, ";");
				mount = _schedule->getMountList()[atoi(loadMountStrList[0].c_str())];
				mountNumList.push_back((*iter).second);
			}
		}

		// 4-b) 计算“运力一次可装的数量上限”
		int sedMaxMountNum = _fleet->getMountNum(mount);

		// 4-c) 判断该方案能否被“运力一次装走”
		sort(mountNumList.begin(), mountNumList.end());//从小到大排序
		int countMountNum = 0;
		bool isSave = true;
		//for (int j = 0; j < mountNumList.size(); j++)
		//{
		//	countMountNum += mountNumList[j];
		//	if (countMountNum == sedMaxMountNum) // 需要装的和最大可装的数量正好相等 → 可以一次带走
		//		countMountNum = 0;
		//	if (j == mountNumList.size() - 1 && countMountNum <= sedMaxMountNum)
		//		isSave = false; // 最后一段也能被带走 → 丢弃，即飞机编队上会装不满
		//}
		for (int j = 0; j < mountNumList.size(); j++)
		{
			countMountNum += mountNumList[j];
			//if (countMountNum == sedMaxMountNum) // 需要装的和最大可装的数量正好相等 → 可以一次带走
			//	countMountNum = 0;
		}
		if (countMountNum < 0.5*sedMaxMountNum)
			isSave = false; // 最后一段也能被带走 → 丢弃，即飞机编队上会装不满
		
		// 4-d) 保留"刚好能装满"的方案
		if (isSave) {
			newTargetTypePlanList.push_back(targetTypePlan);
		}
	}

	// 用剪枝后的结果覆盖旧结果
	_targetTypePlanList = newTargetTypePlanList;

	// -------------------------------------------------
	// 5) 同步更新“纯目标类型集合”列表，供后续枚举具体目标用
	// -------------------------------------------------
	vector<vector<TargetType*>> targetListList;
	for (int i = 0; i < _targetTypePlanList.size(); i++)
	{
		vector<TargetType*> targetList;
		vector<pair<TargetType*, map<string, int>>> targetTypePlan = _targetTypePlanList[i];
		for (int j = 0; j < targetTypePlan.size(); j++)
		{
			targetList.push_back(targetTypePlan[j].first);
		}
		targetListList.push_back(targetList);
	}
	_tpComList = targetListList;

	// 日志：剪枝后还剩多少方案
	cout << "After Branch TargetTypePlanList " << _targetTypePlanList.size() << endl;
	cout << "Step0-1 running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
	cout << "========================================" << endl;

	// -------------------------------------------------
	// 6) 进入下一步：根据保留下来的“目标类型组合”枚举具体目标
	// -------------------------------------------------
	geneTargetList();
}


void Scheme::findTP(vector<TargetType*> tpList, int num, int numTP)
{
	//输入：tpList,已经选择的目标类型组合列表，
	//num，组合中需要选择的目标类型数量，numTP，可选的目标类型数量，num=2,numTP=5,表示从5个可选的目标类型中任意选择不少于两个目标类型 形成组合
	if (num == 1)
	{
		//如果组合中需要选择的目标类型数量为1，则直接将可选的目标类型加入组合中
		for (int i = 0; i <= numTP; i++)//当i=numTP时，就是回头把索引小于i的TargetType加入到newTPList中
		{
			//cout << "i=" << std::to_string(i) << endl;
			vector<TargetType* > newTPList = tpList;
			newTPList.push_back(_schedule->getTargetTypeList()[i]);
			//cout << "newTPList:" << newTPList << endl;
			_tpComList.push_back(newTPList);
		}
		//cout << "长度为" << std::to_string(_tpComList.size()) << endl;
		//cout << "_tpComList:" << _tpComList << endl;
	}
	else
	{
		for (int i = 0; i <= numTP; i++)
		{
			vector<TargetType*> newTpList = tpList;//先初始化为原有组合列表
			newTpList.push_back(_schedule->getTargetTypeList()[i]);
			findTP(newTpList, num - 1, i);
		}
	}
}

void Scheme::findTTPlan(vector<TargetType*> tpList,
	map<string, int> LoadPlan,
	vector<pair<TargetType*, map<string, int>>> subTPPlanList,
	int numSubTP,
	int tpInd)
{
	// -------------------------------------------------------------------------
	// 递归出口：所有目标都已分配完毕，进入“成品检验”阶段
	// -------------------------------------------------------------------------
	if (numSubTP == 0)
	{
		int totalT = subTPPlanList.size();          // 当前方案里目标的个数

		bool ifExist = false;  // 标记：是否与已有方案重复
		bool otherLimit = false; // 标记：是否出现“劣于旧方案”
		bool ifBetter = false; // 标记：是否“优于旧方案”

		// 遍历 _targetTypePlanList 中所有旧方案，做支配/去重判断
		for (int i = 0; i < _targetTypePlanList.size(); i++)
		{
			// 仅比较“目标个数”相同的方案
			if (_targetTypePlanList[i].size() == totalT)
			{
				vector<pair<TargetType*, map<string, int>>> prePlanList = _targetTypePlanList[i];
				vector<pair<TargetType*, map<string, int>>> curPlanList = subTPPlanList;

				// 把目标类型指针抽出来，准备集合比较
				vector<TargetType*> list1;
				vector<TargetType*> list2;
				for (int k = 0; k < prePlanList.size(); k++)
				{
					list1.push_back(prePlanList[k].first);//原计划的目标类型表
				}
				for (int k = 0; k < curPlanList.size(); k++)
				{
					list2.push_back(curPlanList[k].first);//新计划的目标类型表
				}
				bool sameTT = checkSameSet(list1, list2); // 目标类型集合是否相同

				if (!sameTT) continue; // 集合不同 → 跳过

				ifExist = true; // 至少找到了相同集合

				bool ifTBetter1 = false;
				bool ifTBetter2 = false;

				// 先排序，方便逐项比较，cmp返回true或false，false为0，true为1，sort默认升序，即方案由劣到优
				sort(prePlanList.begin(), prePlanList.end(), cmp);
				sort(curPlanList.begin(), curPlanList.end(), cmp);

				bool ifRep = true; // 假设两个方案完全相同（待证伪）

				// 逐项比较每个目标的弹药分配
				for (int k = 0; k < prePlanList.size(); k++)
				{
					map<string, int> planPerType = curPlanList[k].second;//新方案的目标k的装载计划
					map<string, int> prePlan = prePlanList[k].second;//旧方案目标k的装载计划

					map<string, int>::iterator subIter = prePlan.begin();
					bool ifBetter1 = false;//初始化新分配更好为false
					bool ifBetter2 = false;//初始化旧分配更好为false
					int  count = 0;//记录新方案中与旧方案相同的<挂载-载荷>的数量

					// 遍历旧方案的每一种<挂载-载荷,数量>
					for (subIter; subIter != prePlan.end(); subIter++)
					{
						string planName = (*subIter).first;
						//把旧分配放在新的里头找，找到了
						if (planPerType.find(planName) != planPerType.end())
						{
							count++;
							if ((*subIter).second < planPerType[planName])//如果旧方案的数量小于新方案的数量
							{
								ifBetter1 = true;//则说明新方案优于旧方案
								ifRep = false;//如果旧方案的数量小于新方案的数量，则说明两个方案不相同
							}
							else if ((*subIter).second > planPerType[planName])//如果旧方案的数量大于新方案的数量
							{
								ifBetter2 = true;//则说明新方案劣于旧方案
								ifRep = false;//如果旧方案的数量大于新方案的数量，则说明两个方案不相同
							}
						}
						//旧分配没找到
						else
						{
							ifRep = false;//如果旧方案中存在新方案中没有的装载计划，则说明两个方案不相同
							ifBetter2 = true;//则说明新方案劣于旧方案
						}
					}

					// 如果当前方案多了某种挂载-载荷
					if (count < planPerType.size())
					{
						ifRep = false;//如果新方案中存在旧方案中没有的装载计划，则说明两个方案不相同
						ifBetter1 = true;//则说明新方案优于旧方案
					}

					if (ifBetter1) ifTBetter1 = true;//如果新分配更好，则新方案更好
					if (ifBetter2)
					{
						ifTBetter2 = true;
						if (!ifBetter1) otherLimit = true;//如果旧分配更好，且新分配不好，那么新方案的otherLimit=true
					}
				}

				// 完全相同 → 直接丢弃
				if (ifRep)
				{
					ifBetter = false;
					break;
				}

				// 如果新方案好，旧方案不好
				if (!ifTBetter2 && ifTBetter1)
				{
					_targetTypePlanList.erase(_targetTypePlanList.begin() + i);//删除索引为i的方案，即旧方案
					_remainMountLoadList.erase(_remainMountLoadList.begin() + i);//删除索引为i的剩余负载计划
					i--;
				}

				if (ifTBetter1) ifBetter = true;//如果ifTBetter1为true，则将ifBetter设置为true
			}
		}

		// 如果从未出现过同类方案，直接加入
		if (!ifExist)
		{
			_targetTypePlanList.push_back(subTPPlanList);
			_remainMountLoadList.push_back(LoadPlan);//记录剩余的装载计划
		}

		// 如果优于旧方案且没有其他限制，也加入
		if (ifBetter && !otherLimit)
		{
			_targetTypePlanList.push_back(subTPPlanList);
			_remainMountLoadList.push_back(LoadPlan);
		}
	}
	// -------------------------------------------------------------------------
	// 递归体：还有目标没分完，继续深度优先枚举
	// -------------------------------------------------------------------------
	else
	{
		// 取出当前要处理的目标的目标类型
		TargetType* targetType = tpList[tpInd];

		// 获取该目标类型的所有需求装载计划
		vector<TargetPlan*> targetPlanList = targetType->getTargetPlanList();

		// 拷贝剩余弹药，避免回溯时互相污染
		map<string, int> loadPlan = LoadPlan;

		// 遍历所有需求装载计划尝试分配
		for (int n = 0; n < targetPlanList.size(); n++)
		{
			//按目标类型查找到需求的<挂载-载荷,数量>
			map<MountLoad*, int> loadMountList = targetPlanList[n]->_mountLoadPlan;

			map<MountLoad*, int>::iterator iter = loadMountList.begin();

			// 目标实际能匹配到的<挂载-载荷,数量>
			map<string, int> planPerType;

			// 遍历每一种挂载-载荷需求
			for (iter; iter != loadMountList.end(); iter++)
			{
				// 拼接挂载-载荷名称,用于和loadplan中的挂载-载荷匹配
				string loadName = to_string((*iter).first->_mount->getId()) + ";" +
					to_string((*iter).first->_load->getId());
				//需求的挂载-载荷数量
				int dmdNum = (*iter).second;

				// 如果剩余弹药里有这种挂载-载荷
				if (loadPlan.find(loadName) != loadPlan.end())
				{
					// 足够就全拿
					if (loadPlan[loadName] >= dmdNum)
					{
						//当前需求计划可以匹配到的数量
						planPerType[loadName] = dmdNum;
						// 更新剩余数量
						loadPlan[loadName] -= dmdNum;
					}
					// 不够就尽量拿
					else if (loadPlan[loadName] > 0)
					{

						//匹配数量=剩余数量 
						planPerType[loadName] = loadPlan[loadName];
						//装载计划删除map中的这个组合
						loadPlan.erase(loadPlan.find(loadName));
						continue;
					}
				}
			}

			// 如果这张需求单至少匹配到一种挂载-载荷
			if (planPerType.size() > 0)
			{
				// 把当前目标及其分配结果加入子方案
				vector<pair<TargetType*, map<string, int>>> newSubTPPlanList = subTPPlanList;
				newSubTPPlanList.push_back(make_pair(targetType, planPerType));

				// 递归处理下一个目标，索引加1，tpInd + 1，剩余执行目标数量-1，numSubTP - 1，同时装载计划中剩余数量已更新
				findTTPlan(tpList, loadPlan, newSubTPPlanList, numSubTP - 1, tpInd + 1);
			}
		}
	}
}

//按照目标类型计划生成具体目标的装载计划
void Scheme::generateAftAssignTotalTour()
{
	for (int i = 0; i < _targetListList.size(); i++)
	{
		vector<Target*> targetList = _targetListList[i];//编队执行目标列表
		for (int j = 0; j < _targetTypePlanList.size(); j++)
		{
			vector<pair<TargetType*, map<string, int>>> targetTypePlan = _targetTypePlanList[j];//目标类型计划
			vector<TargetType*> list1;
			vector<TargetType*> list2;

			for (int m = 0; m < targetList.size(); m++)
			{
				list1.push_back(targetList[m]->getType());
			}
			for (int m = 0; m < targetTypePlan.size(); m++)
			{
				list2.push_back(targetTypePlan[m].first);
			}
			//判断生成的目标列表和目标类型计划是否匹配
			bool ifMatch = checkSameSet(list1, list2);

			map<pair<TargetType*, map<string, int>>, int>  mergeTargetTypePlanMap;//合并相同的目标类型计划，记录数量
			vector<pair<pair<TargetType*, map<string, int>>, int>>  mergeTargetTypePlan;//合并后的目标类型计划数量列表
			for (int m = 0; m < targetTypePlan.size(); m++)
			{
				pair<TargetType*, map<string, int>> targetTypeP = targetTypePlan[m];
				if (mergeTargetTypePlanMap.find(targetTypeP) != mergeTargetTypePlanMap.end())
				{
					mergeTargetTypePlanMap[targetTypeP] = mergeTargetTypePlanMap[targetTypeP] + 1;
				}
				else
				{
					mergeTargetTypePlanMap[targetTypeP] = 1;
				}
			}
			map<pair<TargetType*, map<string, int>>, int>::iterator iter = mergeTargetTypePlanMap.begin();
			for (iter; iter != mergeTargetTypePlanMap.end(); iter++)
			{
				mergeTargetTypePlan.push_back(make_pair((*iter).first, (*iter).second));
			}
			if (ifMatch)
			{
				vector<pair<Target*, map<string, int>>> operTargetList;
				set<Target*> tSet;
				//输入最后一个目标类型计划数量列表的索引、数量
				geneOperTargetList(operTargetList, mergeTargetTypePlan, tSet, targetList, mergeTargetTypePlan.size() - 1, mergeTargetTypePlan[mergeTargetTypePlan.size() - 1].second, j, 0);
			}
		}
	}
	//_targetPlanList中存储了运力单次执行任务的具体目标，及目标所需的装载计划
	cout << "TargetPlanList: " << _targetPlanList.size() << endl;
	cout << "Step0-3 running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;

	for (int i = 0; i < _targetPlanList.size(); i++)
	{
		vector<pair<Target*, map<string, int>>> operTaskPlanList = _targetPlanList[i];
		vector<OperTarget*> operTaskList;
		//遍历每个具体目标计划,生成操作目标列表
		for (int j = 0; j < operTaskPlanList.size(); j++)
		{
			vector<pair<MountLoad*, int>> mountLoadPlan;
			vector<pair<Mount*, int>> mountPlanList;
			vector<pair<Load*, int>> loadPlanList;
			map<string, int> loadPlan = operTaskPlanList[j].second;
			map<string, int>::iterator iter = loadPlan.begin();
			for (iter; iter != loadPlan.end(); iter++)
			{
				vector<string> loadMountStrList;
				string loadMountStr = (*iter).first;

				//拆分挂载与载荷
				_split(loadMountStr, loadMountStrList, ";");
				Mount* mount = _schedule->getMountList()[atoi(loadMountStrList[0].c_str())];
				Load* load = _schedule->getLoadList()[atoi(loadMountStrList[1].c_str())];

				//提取数量，形成挂载计划和负荷计划
				mountPlanList.push_back(make_pair(mount, (*iter).second));
				loadPlanList.push_back(make_pair(load, (*iter).second));

				//形成挂载-载荷计划
				MountLoad* mountLoad = _schedule->getMountLoad(mount, load);
				mountLoadPlan.push_back(make_pair(mountLoad, (*iter).second));
			}

			//构造函数，传入目标、装载计划、载荷计划
			OperTarget* operTarget = new OperTarget(operTaskPlanList[j].first, mountPlanList, loadPlanList);
			operTarget->setMountLoadPlan(mountLoadPlan);
			operTaskList.push_back(operTarget);
		}
		//构造函数，传入操作目标列表
		Tour* tour = new Tour(_fleet,operTaskList);
		//添加基地
		tour->setBase(_schedule->getBase());
		//剩余负荷计划，_targetTPIndexList[i]返回的是与目标计划类型一致的目标类型计划的索引，最终返回这个目标类型组合的剩余负载
		map<string, int> remainML = _remainMountLoadList[_targetTPIndexList[i]];
		map<string, int>::iterator remainIter = remainML.begin();
		for (remainIter; remainIter != remainML.end(); remainIter++)
		{
			vector<string> loadMountStrList;
			string loadMountStr = (*remainIter).first;
			//拆分挂载与载荷
			_split(loadMountStr, loadMountStrList, ";");
			//提取数量，形成挂载计划和负荷计划
			Mount* mount = _schedule->getMountList()[atoi(loadMountStrList[0].c_str())];
			Load* load = _schedule->getLoadList()[atoi(loadMountStrList[1].c_str())];
			MountLoad* mountLoad = _schedule->getMountLoad(mount, load);
			//计算剩余总数量
			tour->pushRemainML(mountLoad, (*remainIter).second);
		}
		_tourList.push_back(tour);
	}
}

//void Scheme::generateTimeAssignTotalTour(vector<Slot*> slotList)
//{
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		_tourList[i]->computeMountLoadNum();
//	}
//	vector<Tour* > preTourList = _tourList;
//	vector<Tour* > newTourList;
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		{
//			vector<map<Slot*, int>> timeMapList = generateTimeAssignSchds(_tourList[i], slotList);
//			if (timeMapList.size() == 0)
//			{
//			}
//			else
//			{
//				for (int j = 0; j < timeMapList.size(); j++)
//				{
//					vector<OperTarget*> operTargetList = _tourList[i]->getOperTargetList();
//					vector<OperTarget*> newOperTargetList;
//					for (int q = 0; q < operTargetList.size(); q++)
//					{
//						OperTarget* newOperTarget = new OperTarget(operTargetList[q]->getTarget(), operTargetList[q]->getMountLoadPlan());
//						newOperTargetList.push_back(newOperTarget);
//					}
//					Tour* tour = new Tour(newOperTargetList);
//					//添加基地
//					tour->setBase(_schedule->getBase());
//					map<MountLoad*, int> remainMLList = _tourList[i]->getRemainMLList();
//					tour->setRemainMLList(remainMLList);
//					tour->setTimeMap(timeMapList[j]);
//
//					newTourList.push_back(tour);
//				}
//			}
//		}
//	}
//	_tourList = newTourList;
//	for (int i = 0; i < preTourList.size(); i++)
//	{
//		delete preTourList[i];
//	}
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		_tourList[i]->assignTimeForSubTargets(_fleet, _aftNum);
//	}
//}

//生成目标组合列表
void Scheme::geneTargetList()
{
	//遍历目标组合的列表
	for (int i = 0; i < _tpComList.size(); i++)
	{
		vector<TargetType*> typeList = _tpComList[i];//第i个目标组合
		vector<Target*> tList;
		vector<pair<TargetType*, int>> ttIndMap;//；记录目标类型出现的索引
		set<TargetType* > targetTypeSet;//目标类型记录（不重复）
		for (int j = 0; j < typeList.size(); j++)
		{
			if (targetTypeSet.find(typeList[j]) == targetTypeSet.end())
			{
				//如果 typeList[j] 不在 targetTypeSet 中，就把它插入进去
				targetTypeSet.insert(typeList[j]);
				//记录类型和索引
				ttIndMap.push_back(make_pair(typeList[j], 0));
			}
		}
		//typeList.size() - 1索引由后向前，
		findTargetList(tList, typeList, typeList.size() - 1, Util::targetMaxDistance, ttIndMap);
	}

	cout << "TargetListList " << _targetListList.size() << endl;
	cout << "Step0-2 running time is " << (double)(clock() - Util::startRunningClock) / CLOCKS_PER_SEC << endl;
}

void Scheme::findTargetList(vector<Target*> tList, vector<TargetType*> ttList, int ttInd, double dst, vector<pair<TargetType*, int>> ttIndMap)
{
	//输入：目标列表，目标类型列表，目标类型索引，最大距离，目标类型索引映射
	//递归出口：如果typeList的索引ttInd为-1，则说明已经遍历完所有类型，此时判断是否满足条件
	if (ttInd == -1)
	{
		bool ifFea = true;
		//遍历目标列表，判断是否满足条件
		for (int i = 0; i < tList.size() - 1; i++)
		{
			for (int j = i + 1; j < tList.size(); j++)
			{
				double actDst = getDistance(tList[i]->getLongitude(), tList[i]->getLatitude(), tList[j]->getLongitude(), tList[j]->getLatitude());
				//两个目标之间距离超过最大距离
				if (actDst > dst)
				{
					ifFea = false;
					break;
				}
				//两个目标之间的顺序差超过10
				if (abs(tList[i]->getMainTarget()->getOrder() - tList[j]->getMainTarget()->getOrder()) > 10)
				{
					ifFea = false;
					break;
				}
			}
			if (!ifFea)
			{
				break;
			}
		}
		if (tList.size() > 0)
		{
			for (int i = 0; i < tList.size() - 1; i++)
			{
				//如果相邻的两个目标不属于同一个批次，则不满足条件
				if (tList[i]->getMainTarget()->getBatchId() != tList[i + 1]->getMainTarget()->getBatchId())
				{
					ifFea = false;
					break;
				}
			}
		}

		if (ifFea)
		{
			//如果以上都没有违背，则将编队的一次可执行目标列表加入目标列表列表
			_targetListList.push_back(tList);
		}
	}
	//递归主体：遍历目标类型列表
	else
	{
		TargetType* targetType = ttList[ttInd];//第ttInd个目标类型
		vector<Target*> targetList = targetType->getTargetList();//找到目标类型对应的所有目标
		int stInd = 0;
		for (int i = 0; i < ttIndMap.size(); i++)
		{
			if (ttIndMap[i].first == targetType)
			{
				stInd = i;//目标类型在ttIndMap中的索引
			}
		}
		for (int i = ttIndMap[stInd].second; i < targetList.size(); i++)
		{
			vector<Target*> newTList = tList;
			newTList.push_back(targetList[i]);//将目标加入目标列表
			vector<pair<TargetType*, int>> newTtIndMap = ttIndMap;
			newTtIndMap[stInd].second = i + 1;//下次尝试加入目标的索引
			//如果目标列表不为空，且上一个目标与当前加入的目标不属于同一个批次，则跳过
			//跳过就不会再往newTList中添加目标了，而再次循环时会重置newTList,那么这次加入的目标就也弃用了
			if (tList.size() > 0 && tList[tList.size() - 1]->getMainTarget()->getBatchId() != targetList[i]->getMainTarget()->getBatchId())
			{
				continue;
			}
			if (targetList[i]->getStatus() != NORMAL)
			{//NEW ADD
				continue;
			}
			//更新目标列表，目标类型索引，目标类型索引映射
			findTargetList(newTList, ttList, ttInd - 1, dst, newTtIndMap);
		}
	}
}

bool Scheme::checkSameSet(vector<TargetType*> list1, vector<TargetType*> list2)
{
	bool ifMatch = true;
	if (list1.size() != list2.size())
	{
		ifMatch = false;
	}
	else
	{
		map<TargetType*, int> targetTypeMap;
		for (int m = 0; m < list1.size(); m++)
		{
			TargetType* tp = list1[m];
			if (targetTypeMap.find(tp) != targetTypeMap.end())
			{
				targetTypeMap[tp] = targetTypeMap[tp] + 1;
			}
			else
			{
				targetTypeMap[tp] = 1;
			}
		}
		for (int m = 0; m < list2.size(); m++)
		{
			if (targetTypeMap.find(list1[m]) != targetTypeMap.end())
			{
				targetTypeMap[list1[m]] = targetTypeMap[list1[m]] - 1;//这里把list2改成了list1
			}
			else
			{
				ifMatch = false;
			}
		}
		map<TargetType*, int>::iterator iter = targetTypeMap.begin();
		for (iter; iter != targetTypeMap.end(); iter++)
		{
			if ((*iter).second != 0)
			{
				ifMatch = false;
				break;
			}
		}
	}
	return ifMatch;
}

void Scheme::geneOperTargetList(vector<pair<Target*, map<string, int>>> operTargetList, vector<pair<pair<TargetType*, map<string, int>>, int>> targetTypePlanList, set<Target*> tSet, vector<Target*> targetList, int ttInd, int remainNum, int fixedTtInd, int preTInd)
{
	//递归出口，如果目标类型计划索引为-1，则说明已经遍历完所有目标类型计划，此时将operTargetList加入目标计划列表
	if (ttInd == -1)
	{
		_targetPlanList.push_back(operTargetList);
		_targetTPIndexList.push_back(fixedTtInd);//目标类型计划索引列表，即generateAftAssignTotalTour找到的和targrtlist[i]的目标类型相同的计划索引
	}
	//递归主体,遍历目标列表,按照目标类型计划数量表，设置目标的装载计划
	else
	{
		pair<pair<TargetType*, map<string, int>>, int> targetTypePlan = targetTypePlanList[ttInd];//根据递归的索引ttInd找到目标类型计划
		for (int i = preTInd; i < targetList.size(); i++)
		{
			//如果目标类型计划中的目标类型与目标列表中的目标类型相同，且目标列表中的目标没有在tSet中
			if (targetList[i]->getType() == targetTypePlan.first.first && tSet.find(targetList[i]) == tSet.end())
			{
				vector<pair<Target*, map<string, int>>> newOperTargetList = operTargetList;
				//将目标类型计划中的挂载计划形成组合<具体目标，挂载计划>加入newOperTargetList
				newOperTargetList.push_back(make_pair(targetList[i], targetTypePlan.first.second));
				//加入没有在tSet中的目标
				set<Target*> newTSet = tSet;
				newTSet.insert(targetList[i]);

				int newRemainNum = remainNum;
				newRemainNum--;//目标类型计划的剩余数量-1
				//如果目标类型计划的剩余数量为0，那么从目标列表的起始索引preInd=0开始，重新遍历，寻找与下一个目标类型计划对应的目标类型，更新目标类型计划列表索引为[ttInd - 1]
				if (newRemainNum == 0)
				{
					geneOperTargetList(newOperTargetList, targetTypePlanList, newTSet, targetList, ttInd - 1, targetTypePlanList[ttInd - 1].second, fixedTtInd, 0);
				}
				else
				{
					//还有没有匹配的的目标类型计划数量，则遍历目标的起始索引i+1，
					geneOperTargetList(newOperTargetList, targetTypePlanList, newTSet, targetList, ttInd, newRemainNum, fixedTtInd, i + 1);
				}
			}
		}
	}
}


void Scheme::findExploSeq(int aftNum, vector<int> exploSeq, int preInd)
{
	if (exploSeq.size() == aftNum)
	{
		_exploSeqList.push_back(exploSeq);
	}
	else
	{
		for (int i = preInd; i <= min((int)exploSeq.size(), preInd + 1); i++)
		{
			vector<int> newExploSeq = exploSeq;
			newExploSeq.push_back(i);
			findExploSeq(aftNum, newExploSeq, i);
		}
	}
}

//vector<map<Slot*, int>> Scheme::generateTimeAssignSchdsPreUb(Tour* tour, vector<Slot*> slotList)
//{
//	vector<map<Slot*, int>> slotMapList;
//	int aftNum = _aftNum;
//	int totalWeaponNum = 0;
//	bool ifOccupy = false;
//	Mount* mount = NULL;
//	vector<OperTarget* > operTargetList = tour->getOperTargetList();
//	for (int i = 0; i < operTargetList.size(); i++)
//	{
//		vector<pair<MountLoad*, int>> mountLoadList = operTargetList[i]->getMountLoadPlan();
//		for (int j = 0; j < mountLoadList.size(); j++)
//		{
//			mount = mountLoadList[j].first->_mount;
//			if (mountLoadList[j].first->_mount->isOccupy())
//			{
//				ifOccupy = true;
//			}
//			totalWeaponNum += mountLoadList[j].second;
//		}
//	}
//	if (totalWeaponNum == 0)
//	{
//		return slotMapList;
//	}
//
//	{
//		int weaponUB = _fleet->getMountNum(mount);
//		if (weaponUB != 0 && totalWeaponNum != 0 && ceil((double)totalWeaponNum / (double)weaponUB) < aftNum)
//		{
//			vector<int> grpAftNumList ;
//			grpAftNumList.push_back(1);
//			for (int i = 0; i < grpAftNumList.size(); i++)
//			{
//				if (grpAftNumList[i] == ceil((double)totalWeaponNum / (double)weaponUB))
//				{
//					return slotMapList;
//				}
//			}
//		}
//		aftNum = ceil((double)totalWeaponNum / (double)weaponUB);
//		vector<int> weaponNumList;
//		for (int i = 0; i < aftNum; i++)
//		{
//			weaponNumList.push_back(min(weaponUB, totalWeaponNum - weaponUB * i));
//		}
//		_exploSeqList.clear();
//		vector<int> exploSeq;
//		findExploSeq(aftNum, exploSeq, 0);
//
//		int latestSlotInd = slotList.size() - 1;
//		int earliestSlotInd = 0;
//		int bufferTime = mount->getMountType()->intervalTime / Util::generalSlotLimit.second;
//		for (int i = 0; i < _exploSeqList.size(); i++)
//		{
//			vector<int> exploSeq = _exploSeqList[i];
//			for (int j = earliestSlotInd; j <= latestSlotInd; j++)
//			{
//				bool ifFea = true;
//				map<Slot*, int> slotMap;
//				for (int q = 0; q < exploSeq.size(); q++)
//				{
//					int exploTimes = ceil((double)weaponNumList[q] / 2.0);
//					if (j + (exploTimes - 1) * bufferTime + exploSeq[exploSeq.size() - 1] > slotList.size() - 1)
//					{
//						ifFea = false;
//						break;
//					}
//					else
//					{
//						for (int m = 0; m < exploTimes; m++)
//						{
//							slotMap[slotList[j + exploSeq[q] + m * bufferTime]] = slotMap[slotList[j + exploSeq[q] + m * bufferTime]] + 1 * ifOccupy;
//						}
//					}
//				}
//				if (ifFea)
//				{
//					slotMapList.push_back(slotMap);
//				}
//			}
//		}
//	}
//	return slotMapList;
//}
//
//void Scheme::generateTimeAssignTotalTourPreUb(vector<Slot*> slotList)
//{
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		_tourList[i]->computeMountLoadNum();
//	}
//	vector<Tour* > preTourList = _tourList;
//	vector<Tour* > newTourList;
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		vector<map<Slot*, int>> timeMapList = generateTimeAssignSchdsPreUb(_tourList[i], slotList);
//		if (timeMapList.size() == 0)
//		{
//		}
//		else
//		{
//			for (int j = 0; j < timeMapList.size(); j++)
//			{
//				vector<OperTarget*> operTargetList = _tourList[i]->getOperTargetList();
//				vector<OperTarget*> newOperTargetList;
//				for (int q = 0; q < operTargetList.size(); q++)
//				{
//					OperTarget* newOperTarget = new OperTarget(operTargetList[q]->getTarget(), operTargetList[q]->getMountLoadPlan());
//					newOperTargetList.push_back(newOperTarget);
//				}
//				Tour* tour = new Tour(newOperTargetList);
//				map<MountLoad*, int> remainMLList = _tourList[i]->getRemainMLList();
//				tour->setRemainMLList(remainMLList);
//				tour->setTimeMap(timeMapList[j]);
//				newTourList.push_back(tour);
//			}
//		}
//	}
//	_tourList = newTourList;
//	for (int i = 0; i < _tourList.size(); i++)
//	{
//		_tourList[i]->assignTimeForSubTargets(_fleet, _aftNum);
//	}
//	for (int i = 0; i < preTourList.size(); i++)
//	{
//		delete preTourList[i];
//	}
//}