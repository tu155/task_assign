#include "MainTarget.h"
#include "Target.h"

int MainTarget::_countId = 0;
MainTarget::MainTarget(string name,  int priority, vector<Target*> targets)
	:_name(name),_priority(priority), _targetList(targets)
{
	_latestBgnTime = -1;
	_id = _countId++;
	for (int i = 0; i < _targetList.size(); i++)
	{
		_targetList[i]->setMainTarget(this);
	}
	_solnEarliesBgnTime = 0;
}

MainTarget::~MainTarget()
{
}

bool MainTarget::cmpByOrder(MainTarget* tA, MainTarget* tB)
{
	return tA->getOrder() < tB->getOrder();//order为子目标的顺序，返回两个子目标的顺序大小比较。如果前者顺序更靠前，那么返回值为true
}
bool MainTarget::cmpByMTOrder(Target* t1, Target* t2)
{
	if (t1->getMainTarget() == t2->getMainTarget())
	{
		//如果子目标所属maintarget相同，则比较当前目标顺序大小
		return t1->getOrder() < t2->getOrder();
	}
	else
	{
		//如果maintarget不同，则比较主目标的顺序大小
		return t1->getMainTarget()->getOrder() < t2->getMainTarget()->getOrder();
	}
}
void MainTarget::print()
{
	cout << "MainTarget: id: " << _id << " name: " << _name << " subTargetNum: " << _targetList.size() << endl;
	for (int i = 0; i < _targetList.size(); i++) {
		Target* target = _targetList[i];
		target->print();
	}
	
}
void MainTarget::clearSoln()
{
	_latestBgnTime = -1;
	_solnEarliesBgnTime = 0;
}
