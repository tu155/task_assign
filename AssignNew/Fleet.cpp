#include "Fleet.h"

int Fleet::_countId = 0;

Fleet::Fleet(string name, double range, double optSpeed, pair<double, double> speedRange, double oilPerMin, time_t prepareTime, double costUse, double costSpeed)
	:_name(name), _range(range), _optSpeed(optSpeed), _speedRange(speedRange), _oilPerMin(oilPerMin), _prepareTime(prepareTime), _costUse(costUse), _costSpeed(costSpeed)
{
	_id = _countId++;
	_type = NULL;
}

Fleet::Fleet(string name, double costUse, double optSpeed, double oilPerMin)
	: _name(name), _costUse(costUse), _optSpeed(optSpeed), _oilPerMin(oilPerMin)
{
	_id = _countId++;
	_type = NULL;
}

Fleet::~Fleet()
{
	// 在析构函数中安全地处理_type
	if (_type != nullptr) {
		// 必要的清理操作
	}
	// 确保不访问nullptr
}
//获取挂载数量
int Fleet::getMountNum(Mount* mount)
{
	for (int i = 0; i < _mountPlanList.size(); i++)
	{
		if (_mountPlanList[i].first == mount)
		{
			return _mountPlanList[i].second;
		}
	}
	return 0;
}


void Fleet::print()
{
	cout << "Fleet: id: " << _id << " name: " << _name <<" optSpeed: " << _optSpeed
		<< " speedLB: " << _speedRange.first << " speedUB: " << _speedRange.second << " fuel: " << _fuel << " prepareTime: " << _prepareTime << endl;
	for (int i = 0; i < _mountPlanList.size(); i++) {
		cout << "Mount: " << _mountPlanList[i].first->getName() << " num: " << _mountPlanList[i].second << endl;
	}
}