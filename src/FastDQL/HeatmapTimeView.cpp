#include "FastDQL.h"

namespace FastDQL {

HeatmapTimeView::HeatmapTimeView() {
	graph = NULL;
	mode = -1;
}

void HeatmapTimeView::Paint(Draw& d) {
	Size sz = GetSize();
	
	int layer_count = graph->GetCount();
	int total_output = 0;
	
	for(int i = 0; i < layer_count; i++) {
		RecurrentBase& lb = graph->GetLayer(i);
		Mat& output = graph->GetPool().Get(lb.output);
		total_output += output.GetLength();
	}
	
	ImageBuffer ib(total_output, 1);
	RGBA* it = ib.Begin();
	
	
	for(int i = 0; i < layer_count; i++) {
		
		RecurrentBase& lb = graph->GetLayer(i);
		Mat& output = graph->GetPool().Get(lb.output);
		int output_count = output.GetLength();
		
		double max = 0.0;
		
		tmp.SetCount(output_count);
		
		for(int j = 0; j < output_count; j++) {
			double d = output.Get(j);
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

}
