#include "WaterWorld.h"
#include "pretrained.brc"
#include <plugin/bz2/bz2.h>

WaterWorld::WaterWorld() {
	Title("WaterWorld: Deep Q Learning");
	Icon(WaterWorldImg::icon());
	Sizeable().MaximizeBox().MinimizeBox();
	
	for(int i = 0; i < world.agents.GetCount(); i++)
		world.agents[i].world = this;
	
	agent_ctrl.Add(load_btn.HSizePos().BottomPos(60,30));
	agent_ctrl.Add(save_btn.HSizePos().BottomPos(30,30));
	agent_ctrl.Add(reload_btn.HSizePos().BottomPos(0,30));
	load_btn.SetLabel("Load Agent");
	save_btn.SetLabel("Save Agent");
	reload_btn.SetLabel("Reload Agent");
	reload_btn <<= THISBACK(Reload);
	save_btn <<= THISBACK(Save);
	load_btn <<= THISBACK(Load);
	
	statusctrl.Add(status.HSizePos().VSizePos(0,30));
	statusctrl.Add(load_pretrained.HSizePos().BottomPos(0,30));
	load_pretrained.SetLabel("Load a Pretrained Agent");
	load_pretrained <<= THISBACK(LoadPretrained);
	
	Add(btnsplit.HSizePos().TopPos(0,30));
	Add(world.HSizePos().VSizePos(30,30));
	Add(lbl_eps.LeftPos(4,200-4).BottomPos(0,30));
	Add(eps.HSizePos(200,0).BottomPos(0,30));
	eps <<= THISBACK(RefreshEpsilon);
	eps.MinMax(0, +100);
	eps.SetData(20);
	lbl_eps.SetLabel("Exploration epsilon: ");
	btnsplit.Horz();
	btnsplit << reset << goveryfast << gofast << gonorm << goslow;
	goveryfast.SetLabel("Unlimited speed");
	gofast.SetLabel("Go Fast");
	gonorm.SetLabel("Go Normal");
	goslow.SetLabel("Go Slow");
	reset.SetLabel("Reset");
	goveryfast	<<= THISBACK1(SetSpeed, 3);
	gofast	<<= THISBACK1(SetSpeed, 2);
	gonorm	<<= THISBACK1(SetSpeed, 1);
	goslow	<<= THISBACK1(SetSpeed, 0);
	reset	<<= THISBACK(Reset);
	
	network_view.SetGraph(world.agents[0].dqn_trainer);
	
	SetSpeed(1);
	
	PostCallback(THISBACK(Reset));
	PostCallback(THISBACK(Reload));
	PostCallback(THISBACK(Start));
	RefreshEpsilon();
	
	PostCallback(THISBACK(Refresher));
}

WaterWorld::~WaterWorld() {
	running = false;
	while (!stopped) Sleep(100);
}

void WaterWorld::Tick() {
	if      (simspeed == 2) Sleep(1);
	if      (simspeed == 1) Sleep(10);
	else if (simspeed == 0) Sleep(100);
	world.Tick();
}

void WaterWorld::Ticking() {
	while (running) {
		ticking_lock.Enter();
		Tick();
		ticking_lock.Leave();
	}
	stopped = true;
}

void WaterWorld::DockInit() {
	DockLeft(Dockable(agent_ctrl, "Edit Agent").SizeHint(Size(320, 240)));
	DockLeft(Dockable(statusctrl, "Status").SizeHint(Size(320, 240)));
	DockLeft(Dockable(reward, "Reward graph").SizeHint(Size(320, 240)));
	DockRight(Dockable(network_view, "Network View").SizeHint(Size(320, 480)));
}

void WaterWorld::Refresher() {
	world.Refresh();
	RefreshStatus();
	network_view.Refresh();
	
	PostCallback(THISBACK(Refresher));
}

void WaterWorld::Start() {
	ASSERT(!running);
	running = true;
	stopped = false;
	Thread::Start(THISBACK(Ticking));
}

void WaterWorld::Reset() {
	for(int i = 0; i < world.agents.GetCount(); i++) {
		WaterWorldAgent& agent = world.agents[i];
		agent.do_training = true;
		agent.Reset();
	}
}

void WaterWorld::Reload() {
	ticking_lock.Enter();
	for(int i = 0; i < world.agents.GetCount(); i++) {
		WaterWorldAgent& agent = world.agents[i];
		
		agent.dqn_trainer.SetDefaultSettings();
		agent.dqn_trainer.Reset();
	}
	ticking_lock.Leave();
}

void WaterWorld::RefreshEpsilon() {
	double d = (double)eps.GetData() / 100.0;
	for(int i = 0; i < world.agents.GetCount(); i++) {
		WaterWorldAgent& agent = world.agents[i];
		agent.dqn_trainer.SetEpsilon(d);
	}
	lbl_eps.SetLabel("Exploration epsilon: " + FormatDoubleFix(d, 2));
}

void WaterWorld::SetSpeed(int i) {
	simspeed = i;
}

void WaterWorld::Load() {
	FileOut fin(ConfigFile("agent.bin"));
	ticking_lock.Enter();
	for(int i = 0; i < world.agents.GetCount(); i++) {
		WaterWorldAgent& agent = world.agents[i];
		fin % agent.dqn_trainer;
	}
	ticking_lock.Leave();
}

void WaterWorld::Save() {
	FileOut fout(ConfigFile("agent.bin"));
	ticking_lock.Enter();
	for(int i = 0; i < world.agents.GetCount(); i++) {
		WaterWorldAgent& agent = world.agents[i];
		fout % agent.dqn_trainer;
	}
	ticking_lock.Leave();
}

void WaterWorld::LoadPretrained() {
	// This is the pre-trained network from original ConvNetJS
	MemReadStream pretrained_mem(pretrained, pretrained_length);
	BZ2DecompressStream decomp(pretrained_mem);
	decomp.SetLoading();
	ticking_lock.Enter();
	for(int i = 0; i < world.agents.GetCount(); i++) {
		WaterWorldAgent& agent = world.agents[i];
		agent.do_training = false;
		decomp % agent.dqn_trainer;
	}
	ticking_lock.Leave();
}

void WaterWorld::RefreshStatus() {
	WaterWorldAgent& agent = world.agents[0];
	
	String s;
	s << "Experience write pointer: " << agent.GetExperienceWritePointer() << "\n";
	s << "Latest TD error: " << FormatDoubleFix(agent.dqn_trainer.GetTDError(), 3);
	status.SetLabel(s);
}

