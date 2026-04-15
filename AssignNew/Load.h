#ifndef LOAD_H
#define LOAD_H

#include "Util.h"

class Load
{//载荷类
private:
	static int _countId;
	int _id;
	string _name;

	double _costUse;			//使用成本
	int _totalNum;				//总数量

public:
	Load(string name, double costUse);//载荷构造函数：输入名称和成本
	~Load();							//载荷解构函数

	//属性的设置和获得函数
	int getId() { return _id; }
	void setId(int id) { _id = id; }

	string getName() { return _name; }
	void setName(string name) { _name = name; }

	double getCostUse() { return _costUse; }
	void setCostUse(double costUse) { _costUse = costUse; }

	int getTotalNum() { return _totalNum; }
	void addLoad(int num) { _totalNum += num; }
	static void initCountId() { _countId = 0; }

	void print();
};

#endif // !LOAD_H
#pragma once
