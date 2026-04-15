#include "Greedy.h"

Greedy::Greedy(Schedule* schedule): _schedule(schedule) {
	greedyRun();
}

void Greedy::greedyRun() {
    vector<string>flagList = { "timeShortest","costLowest","minDifference" };
    //vector<string>flagList = { "timeShortest","costLowest"};
    for (int i = 0; i < flagList.size(); i++) {
        Individual ind(flagList[i], _schedule);
        ind.encode();
        ind.decode();
        check(ind);
        map<string,float>singleStrategy;
        ind.cocuSumCostTimeTotal();
        cout << "-----------"<<flagList[i] << "策略下的结果：--------------------" << endl;
        cout << "SumCost: " << ind._SumCost << endl;
        cout << "SumTime: " << ind._SumTime << endl;
        singleStrategy["SumCost"] = ind._SumCost;
        singleStrategy["SumTime"] = ind._SumTime;
        
        ind.cocuSumDifferenceTotal();
        cout << "minDifference: " << ind._SumDifference << endl;
        ind._objectives.push_back(ind._SumTime);
        ind._objectives.push_back(ind._SumCost);
        if (ind._objCount == 3) {
            ind._objectives.push_back(ind._SumDifference);
            singleStrategy["minDifference"] = ind._SumDifference;
        }
        //存储结果
        vector<Tour* >soluTourList = ind.getsolnTourList();

        _result[flagList[i]] = soluTourList;
        _objectives[flagList[i]] = singleStrategy;
    }
    
}

void Greedy::check(Individual ind) {
    ind.getAllocation();
    //清空覆盖目标集合和未覆盖目标集合
    _schedule->clearCoverT();
    _schedule->clearUncoverT();
    //检查目标是否覆盖，如果覆盖，计算总需求量是否超出可供量，并记录
    for (Target* target : _schedule->getTargetList()) {
        Mount* mount = ind._targetMount[target];
        ind._mountNeedTotalNum[mount] += ind._targetAllocation[make_pair(target, mount)];
        if (ind._targetNeedNum[target] <= ind._targetAllocation[make_pair(target, mount)]) {
            target->setLpVal(1.0);
            _schedule->pushCoverT(target);//记录覆盖的目标
        }
        else {
            target->setLpVal(0.0);
            _schedule->pushUncoverT(target);//记录未覆盖的目标
            ind._targetUncoverNum[target] = ind._targetNeedNum[target] - ind._targetAllocation[make_pair(target, mount)];
        }
    };
    bool needContinue = false;
    if (ind._targetUncoverNum.size() > 0) {
        needContinue = true;
        cout << "error : 有目标未完全覆盖" << endl;
    }
    bool error = false;
    //如果目标未覆盖，检查剩余资源能否满足
    for (auto it = ind._targetUncoverNum.begin(); it != ind._targetUncoverNum.end(); it++) {
        if (it->second > 0) {
            Mount* mount = ind._targetMount[it->first];
            if (_schedule->getBase()->getMountList()[mount] - ind._mountNeedTotalNum[mount] > it->second) {
                error = true;
                cout << "error : 有充足的剩余资源，但是未分配给目标" << endl;
                break;
            }
        }
    }
    //检查已经安排的飞机的装载量是否超出最大载重
    bool overweight = false;
    for (Fleet* fleet : _schedule->getFleetList()) {
        Mount* mount = ind._fleetMount[fleet];
        int mountAssignment = ind._fleetAllocation[make_pair(fleet, mount)];
        int mountNum = fleet->getMountNum(mount);
        if (mountAssignment > mountNum) {
            overweight = true;
            cout << "error : 飞机的装载量超出最大载重" << endl;
            break;
        }
    }
}
