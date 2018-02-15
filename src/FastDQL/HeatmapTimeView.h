#ifndef _FastDQLCtrl_HeatmapTimeView_h_
#define _FastDQLCtrl_HeatmapTimeView_h_

#include <FastDQL/FastDQL.h>
#include <CtrlLib/CtrlLib.h>

namespace FastDQL {
using namespace Upp;
using namespace FastDQL;


template <class T>
class HeatmapTimeView : public Ctrl {
	T* graph = NULL;
	Array<Image> lines;
	Vector<double> tmp;
	int mode = -1;
	
public:
	typedef HeatmapTimeView CLASSNAME;
	
	HeatmapTimeView() {}
	
	void SetGraph(T& t) {graph = &t;}
	
	template <class T> void PaintLayer(T& mat, RGBA*& it)  {
		int output_count = mat.GetLength();
		
		double max = 0.0;
		
		tmp.SetCount(output_count);
		
		for(int j = 0; j < output_count; j++) {
			double d = mat.Get(j);
			tmp[j] = d;
			double fd = fabs(d);
			if (fd > max) max = fd;
		}
		
		for(int j = 0; j < output_count; j++) {
			double d = tmp[j];
			byte b = fabs(d) / max * 255;
			if (d >= 0)	{
				it->r = 0;
				it->g = 0;
				it->b = b;
				it->a = 255;
			}
			else {
				it->r = b;
				it->g = 0;
				it->b = 0;
				it->a = 255;
			}
			it++;
		}
	}
	
	virtual void Paint(Draw& d) {
		Size sz = GetSize();
		
		const int layer_count = 5;
		int total_output = 0;
		
		for(int i = 0; i < layer_count; i++) {
			switch (i) {
				case 0: total_output += graph->data.mul1.output.GetLength(); break;
				case 1: total_output += graph->data.add1.output.GetLength(); break;
				case 2: total_output += graph->data.tanh.output.GetLength(); break;
				case 3: total_output += graph->data.mul2.output.GetLength(); break;
				case 4: total_output += graph->data.mul2.output.GetLength(); break;
			}
		}
		
		ImageBuffer ib(total_output, 1);
		RGBA* it = ib.Begin();
		
		
		for(int i = 0; i < layer_count; i++) {
			switch (i) {
				case 0: PaintLayer(graph->data.mul1.output, it); break;
				case 1: PaintLayer(graph->data.add1.output, it); break;
				case 2: PaintLayer(graph->data.tanh.output, it); break;
				case 3: PaintLayer(graph->data.mul2.output, it); break;
				case 4: PaintLayer(graph->data.mul2.output, it); break;
			}
		}
		
		lines.Add(ib);
		while (lines.GetCount() > sz.cy) lines.Remove(0);
		
		
		ImageDraw id(sz);
		id.DrawRect(sz, Black());
		for(int i = 0; i < lines.GetCount(); i++) {
			Image& ib = lines[i];
			id.DrawImage(0, i, sz.cx, 1, ib);
		}
		
		
		d.DrawImage(0, 0, id);
	}
};

}

#endif
