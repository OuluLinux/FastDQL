#ifndef _FastDQLCtrl_HeatmapTimeView_h_
#define _FastDQLCtrl_HeatmapTimeView_h_

#include <FastDQL/FastDQL.h>
#include <CtrlLib/CtrlLib.h>

namespace FastDQL {
using namespace Upp;
using namespace FastDQL;

class HeatmapTimeView : public Ctrl {
	Graph* graph;
	Array<Image> lines;
	Vector<double> tmp;
	int mode;
	
	
public:
	typedef HeatmapTimeView CLASSNAME;
	HeatmapTimeView();
	
	virtual void Paint(Draw& d);
	
	void SetGraph(Graph& g) {this->graph = &g;}
	
};

}

#endif
