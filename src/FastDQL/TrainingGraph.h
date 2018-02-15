#ifndef _FastDQL_TrainingGraph_h_
#define _FastDQL_TrainingGraph_h_

#include <FastDQL/FastDQL.h>
#include <CtrlLib/CtrlLib.h>

using Upp::max;
#include <PlotCtrl/PlotCtrl.h> // include bazaar directory from U++ repo if missing

namespace FastDQL {
using namespace Upp;
using namespace FastDQL;

template <class T>
class TrainingGraph : public ParentCtrl {
	
protected:
	friend class LayerView;
	
	T* ses = NULL;
	PlotCtrl plotter;
	int average_size;
	int last_steps;
	int interval;
	int limit;
	int mode;
	
	enum {MODE_LOSS, MODE_REWARD};
	
public:
	typedef TrainingGraph CLASSNAME;
	
	void SetModeLoss() {mode = MODE_LOSS; plotter.data[0].SetTitle("Loss");}
	void SetModeReward() {mode = MODE_REWARD; plotter.data[0].SetTitle("Reward");}
	void SetAverage(int size) {average_size = size;}
	void SetInterval(int period) {interval = period;}
	void SetLimit(int size) {limit = size;}
	
	TrainingGraph() {
		ses = NULL;
		mode = MODE_REWARD;
		
		plotter.SetMode(PLOT_AA).SetLimits(-5,5,-5,5);
		plotter.data.Add();
		plotter.data.Add();
		plotter.data[0].SetTitle("Reward").SetThickness(1.5).SetColor(Red());
		plotter.data[1].SetDash("1.5").SetTitle("Average").SetThickness(1.0).SetColor(Blue());
		
		Add(plotter.SizePos());
		
		average_size = 20;
		interval = 200;
		last_steps = 0;
		limit = 0;
	}
	
	void SetSession(T& ses) {
		this->ses = &ses;
		ses.WhenStepInterval << THISBACK(StepInterval);
	}
	
	void RefreshData() {
		plotter.Sync();
		plotter.Refresh();
	}
	
	void StepInterval(int num_steps) {
		// log progress to graph, (full loss)
		if (num_steps >= last_steps + interval) {
			AddValue();
			last_steps = num_steps;
		}
	}
	
	void Clear() {
		plotter.data[0].Clear();
		plotter.data[1].Clear();
		last_steps = 0;
	}
	
	void AddValue() {
		if (!ses) return;
		
		double av;
		
		if (mode == MODE_REWARD) {
			av = ses->GetRewardAverage();
		}
		else if (mode == MODE_LOSS) {
			double xa = ses->GetLossAverage();
			double xw = ses->GetL2DecayLossAverage();
			if (xa < 0 || xw < 0) return;
			av = xa + xw;
		}
		
		AddValue(av);
	}
	
	void AddValue(double value) {
		int c = plotter.data[0].GetCount();
		int id = c;
		if (id) id = plotter.data[0][id-1].x+1;
		
		plotter.data[0].AddXY(id, value);
		int count = id + 1;
		if (count < 2) return;
		
		int pos = c-1;
		double sum = 0;
		int av_count = 0;
		for(int i = 0; i < average_size && pos >= 0; i++) {
			sum += plotter.data[0][pos].y;
			av_count++;
			pos--;
		}
		double avav = sum / av_count;
		plotter.data[1].AddXY(id, avav);
		
		if (limit > 0) {
			while (plotter.data[0].GetCount() > limit) {
				plotter.data[0].Remove(0);
				plotter.data[1].Remove(0);
			}
		}
		
		double min = +DBL_MAX;
		double max = -DBL_MAX;
		c = plotter.data[0].GetCount();
		for(int i = 0; i < c; i++) {
			double d = plotter.data[0][i].y;
			if (d > max) max = d;
			if (d < min) min = d;
		}
		double diff = max - min;
		if (diff <= 0) return;
		double center = min + diff / 2;
		plotter.SetLimits(plotter.data[0][0].x, id, min, max);
		plotter.SetModify();
		
		PostCallback(THISBACK(RefreshData));
	}
	
};

}

#endif
