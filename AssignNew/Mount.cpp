#include "Mount.h"

int Mount::_countId = 0;

Mount::Mount(string name, double costUse)
	:_name(name), _costUse(costUse)
{
	_id = _countId++;
	_totalNum = 0;
	_isOccupy = false;
}

Mount::Mount(string name, time_t reqTime, double costUse, bool isOccupy, double range,int stock_num)
	:_name(name), _reqTime(reqTime), _costUse(costUse),  _isOccupy(isOccupy), _range(range),_stock_num(stock_num)
{
	_id = _countId++;
	_totalNum = 0;
}

Mount::~Mount()
{
}

double Mount::getLoadCost(Load* load)
{
	if (_loadCostList.find(load) != _loadCostList.end())
	{
		return _loadCostList[load];
	}
	return 999.0;
}

void Mount::setLoadCost(Load* load, double cost)
{
	_loadCostList[load] = cost;
}

void Mount::print()
{
	cout << "Mount: id: " << _id << " name: " << _name << " costUse: " << _costUse << endl;
}