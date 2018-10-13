#ifndef _FastDQLCtrl_TrainingGraph_h_
#define _FastDQLCtrl_TrainingGraph_h_

#include <FastDQL/FastDQL.h>
#include <CtrlLib/CtrlLib.h>

using Upp::max;
#include <PlotCtrl/PlotCtrl.h>

namespace FastDQL {
using namespace Upp;
using namespace FastDQL;

class TrainingGraph : public ParentCtrl {
	
protected:
	friend class LayerView;
	
	PlotCtrl plotter;
	int average_size;
	int last_steps;
	int interval;
	int limit;
	int mode;
	
	enum {MODE_LOSS, MODE_REWARD};
	
public:
	typedef TrainingGraph CLASSNAME;
	TrainingGraph();
	
	void SetModeLoss() {mode = MODE_LOSS; plotter.data[0].SetTitle("Loss");}
	void SetModeReward() {mode = MODE_REWARD; plotter.data[0].SetTitle("Reward");}
	void SetAverage(int size) {average_size = size;}
	void SetInterval(int period) {interval = period;}
	void SetLimit(int size) {limit = size;}
	
	void RefreshData();
	void PostAddValue(double value) {PostCallback(THISBACK1(AddValue, value));}
	void AddValue(double value);
	void Clear();
};

}

#endif
