#pragma once//包含守卫，防止重复包含
#include "Util.h"
#include "Target.h"
#include "Mount.h"
#include "Load.h"
#include "Fleet.h"
#include "TargetType.h"
#include "MainTarget.h"
#include "Slot.h"
//#include "Tour.h"
#include "Scheme.h"
#include "Base.h"
#include "MountType.h"

class Schedule {
private:
    Base* _base;
    vector<TargetType* > _targetTypeList;
    vector<Target* > _targetList;
    vector<FleetType* > _fleetTypeList;
    vector<Fleet* > _fleetList;
    vector<Mount* > _mountList;
    vector<Load* > _loadList;
    vector<MountLoad* > _mountLoadList;
    vector<Scheme*> _schemeList;
    vector<Tour*> _tourList;
    vector<Tour*> _backUpTourList;
    map<int, Tour*> _tourMap;
    vector<TargetPlan*> _targetPlanList;
    //vector<Scenario* > _scenList;
	vector<MainTarget* > _mainTargetList;
    vector<MountType*> _mountTypeList;

	map<string, vector<MainTarget*>> _batchMap;				//?
	map<string, int> _tnpgUbMap;				//运力可访问目标数量上限
	vector<Slot* > _slotList;							//时间槽
	map<int, vector<int>> _occupiedSlotMap;
	map<int, map<MountType*, int>> _occupiedRegionGrpMap;
	map<Slot*, map<MountType*, int>> _regionGrpCapMap;
    vector<Tour*> _givenTourList;
    vector<pair<string, vector<Target*>>> _batchTargetMap;

    vector<Target* > _uncoverTList;
    vector<Target* > _coverTList;

	map<FleetType*,int> _fleetTypeTourNumMap;
public:
	Schedule();
	~Schedule();
	void resetStaticCounters() {
		Base::initCountId();
		Mount::initCountId();
		Load::initCountId();      // 如果Load也有类似问题
		OperTarget::initCountId();    // 如果OperTarget也有类似问题
		Slot::initCountId();    // 如果Slot也有类似问题
		Target::initCountId();    // 如果Target也有类似问题
		TargetType::initCountId();    // 如果TargetType也有类似问题
		Tour::initCountId();    // 如果Tour也有类似问题
	}

	Base* getBase() {return _base; };
	void setBase(Base* base) { _base = base; };

	vector<Load* > getLoadList() { return _loadList; }
	void pushLoad(Load* load) { _loadList.push_back(load); }
	void setLoadList(vector<Load*> loadList) { _loadList = loadList; }

	vector<Mount* > getMountList() { return _mountList; }
	void pushMount(Mount* mount) { _mountList.push_back(mount); }
	void setMountList(vector<Mount* > mountList) { _mountList = mountList; }

	vector<FleetType* > getFleetTypeList() { return _fleetTypeList; }
	void setFleetTypeList(vector<FleetType*> fleetTypeList) { _fleetTypeList = fleetTypeList; }
	void pushFleetType(FleetType* fleetType) { _fleetTypeList.push_back(fleetType); }

	vector<TargetType* > getTargetTypeList() { return _targetTypeList; }
	void pushTargetType(TargetType* targetType) { _targetTypeList.push_back(targetType); }
	void setTargetTypeList(vector<TargetType* > targetTypeList) { _targetTypeList = targetTypeList; }

	vector<MountLoad* > getMountLoadList() { return _mountLoadList; }
	void setMountLoadList(vector<MountLoad* >  mountLoadList) { _mountLoadList = mountLoadList; }
	void pushMountLoad(MountLoad* mountLoad) { _mountLoadList.push_back(mountLoad); }

	vector<TargetPlan*> getTargetPlanList() { return _targetPlanList; }
	void setTargetPlanList(vector<TargetPlan*> plnList) { _targetPlanList = plnList; }
	void pushTargetPlanList(TargetPlan* pln) { _targetPlanList.push_back(pln); }

	//vector<Scenario* > getScenList() { return _scenList; }
	//void pushScen(Scenario* scen) { _scenList.push_back(scen); }


	vector<Tour*> getTourList() { return _tourList; }
	void pushTour(Tour* tour) { _tourList.push_back(tour); }

	vector<Slot* > getSlotList() { return _slotList; }

	vector<Tour*> getGivenTourList() { return _givenTourList; }
	void pushGivenTour(Tour* tour) { _givenTourList.push_back(tour); }

	vector<Tour*> getBackUpTourList() { return _backUpTourList; }

	vector<Scheme*> getSchemeList() { return _schemeList; }
	void pushScheme(Scheme* scheme) { _schemeList.push_back(scheme); }

	Tour* getTour(int tourId);
	void pushTourMap(Tour* tour, int tourId) { _tourMap[tourId] = tour; }
	map<int, Tour*> getTourMap() { return _tourMap; }

	void pushBatchTargets(string batch, vector<Target*> targets) { _batchTargetMap.push_back(make_pair(batch, targets)); }
	vector<pair<string, vector<Target*>>> getBatchTargetMap() { return _batchTargetMap; }

	vector<Target* > getUncoverTList() { return _uncoverTList; }
	void pushUncoverT(Target* target) { _uncoverTList.push_back(target); }
	void setUncoverTList(vector<Target*> tList) { _uncoverTList = tList; }
    void clearUncoverT() { _uncoverTList.clear(); }

	vector<Target* > getCoverTList() { return _coverTList; }
	void pushCoverT(Target* target) { _coverTList.push_back(target); }
	void setCoverTList(vector<Target*> targetList) { _coverTList = targetList; }
	void clearCoverT() { _coverTList.clear(); };

	vector<MainTarget* > getMainTargetList() { return _mainTargetList; }
	void setMainTargetList(vector<MainTarget*>mainTargets) { _mainTargetList = mainTargets; }
	void pushMainTarget(MainTarget* mainTarget) { _mainTargetList.push_back(mainTarget); }

	vector<MainTarget*> getMaintTargetListByBatId(string batId);
	void pushBatchMainTarget(string batId, MainTarget* mainTarget);
	map<string, vector<MainTarget*>> getBatchMap() { return _batchMap; }

	vector<MountType*>& getMountTypeList() { return _mountTypeList; }
	//const vector<MountType*>& getMountTypeList() { return _mountTypeList; }

	void pushMountType(MountType* mountType) { _mountTypeList.push_back(mountType); }



    //目标类型、目标类型列表的查询、新增、重置
    TargetType* getTargetType(string targetTypeName);
    
    //目标、目标列表的查询、新增、重置
    Target* getTarget(string targetName);
	Target* getTargetById(int id);
    vector<Target* > getTargetList() { return _targetList; }
    void pushTarget(Target* target) { _targetList.push_back(target); }
    void setTargetList(vector<Target* > targetList) { _targetList = targetList; }
    
    
    //运力类型、运力类型列表的查询、新增、重置
    FleetType* getFleetType(string fleetTypeName);
    

    //运力、运力列表的查询、新增、重置
    Fleet* getFleet(string fleetName);
    vector<Fleet* > getFleetList() { return _fleetList; };
    void pushFleet(Fleet* fleet) { _fleetList.push_back(fleet); }
    void setFleetList(vector<Fleet* > fleetList) { _fleetList = fleetList; }
    

    //挂载、挂载列表的查询、新增、重置
    Mount* getMount(string mountName);

    //负载、负载列表的查询、新增、重置
    Load* getLoad(string loadName);

	MountType* getMountType(string name);
	MountLoad* getMountLoad(Mount* mount, Load* load);
	MountLoad* getMountLoadStr(string mountLoad);
	Scheme* getScheme(Fleet* fleet, int aftNum);
	TargetPlan* getTargetPlanByMountLoad(vector<pair<MountLoad*, int>>mountLoadList);
	TargetPlan* getTargetPlanByName(string name);
	
	void init();
	void generateSlotList();
	void computeSlotUB();
	void computeSlotLB();
	void initResAssign();

	void generateSchemes();
	int computeSlotNum();
	void fsbCheck(vector<Tour*> tourList);
	void reduceTours();
	bool checkSlotMapFeasible(Slot* slot, int aftNum, Mount* mount);

	void clearSolnBtwRun();
	void clearSoln();

	void setFleetTypeTourNumMap(FleetType*fleetType,int tourNum) {_fleetTypeTourNumMap[fleetType] = tourNum;};
	int getFleetTypeTourNum(FleetType*fleetType) { return _fleetTypeTourNumMap[fleetType]; };
	//void setPara(Scenario* scen);

	//generate given tours
	//vector<Tour*> geneGivenTourList();
};
