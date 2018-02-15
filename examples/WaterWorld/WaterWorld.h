#ifndef _WaterWorld_WaterWorld_h
#define _WaterWorld_WaterWorld_h

#include <CtrlLib/CtrlLib.h>
#include <FastDQL/FastDQL.h>
#include <Docking/Docking.h>
using namespace Upp;
using namespace FastDQL;

#define IMAGECLASS WaterWorldImg
#define IMAGEFILE <WaterWorld/WaterWorld.iml>
#include <Draw/iml_header.h>

class WaterWorld;


const int AGENT_COUNT = 5;
enum {ACT_LEFT, ACT_RIGHT, ACT_UP, ACT_DOWN, ACT_COUNT};
const int EYES_COUNT = 30;
const int INPUT_SIZE = EYES_COUNT * 5 + 2;

typedef DQNTrainer<ACT_COUNT, INPUT_SIZE> WaterWorldTrainer;


// Wall is made up of two points
struct Wall : Moveable<Wall> {
	Wall() {}
	Wall(const Point& p1, const Point& p2) : p1(p1), p2(p2) {}
	Pointf p1, p2;
};


struct InterceptResult {
	Pointf up;
	double ua, ub;
	int type;
	bool is_intercepting;
	int vx, vy;
	
	InterceptResult() {Reset();}
	InterceptResult(bool is_in) {Reset(); is_intercepting = is_in;}
	InterceptResult(const InterceptResult& ir) {*this = ir;}
	void Reset() {up.x = 0; up.y = 0; ua = 0; ub = 0; vx = 0; vy = 0; is_intercepting = false;}
	operator bool() const {return is_intercepting;}
	InterceptResult& operator=(const InterceptResult& ir) {
		up = ir.up;
		ua = ir.ua;
		ub = ir.ub;
		vx = ir.vx;
		vy = ir.vy;
		type = ir.type;
		is_intercepting = ir.is_intercepting;
		return *this;
	}
};

void AddBox(Vector<Wall>& lst, int x, int y, int w, int h);
inline double RandomRange(double min, double max) {return min + Randomf() * (max - min);}
inline int RandomRangeInt(int min, int max) {return min + Random(max - min);}
InterceptResult IsLineIntersect(Pointf l1a, Pointf l1b, Pointf l2a, Pointf l2b);
InterceptResult IsLinePointIntersect(Pointf l1a, Pointf l1b, Pointf p, int rad);
template <class T> inline void Scale(T& p, double s) { p.x *= s; p.y *= s; }
template <class T> inline void Normalize(T& p) {double d = Length(p); Scale(p, 1.0 / d);}
template <class T> inline T Rotate(const T& p, double angle) {
	return T( // CLOCKWISE
		+p.x * cos(angle) + p.y * sin(angle),
		-p.x * sin(angle) + p.y * cos(angle));
}

// item is circle thing on the floor that agent can interact with (see or eat, etc)
struct Item : Moveable<Item> {
	void Init(int x, int y, int type) {
		p.x = x;
		p.y = y;
		v.x = Randomf() * 5 - 2.5;
		v.y = Randomf() * 5 - 2.5;
		this->type = type;
		rad = 10; // default radius
		age = 0;
		cleanup_ = false;
	}
	Pointf p, v;
	int type, rad, age;
	bool cleanup_;
};



// Eye sensor has a maximum range and senses walls
struct Eye : Moveable<Eye> {
	double angle; // angle relative to agent its on
	double max_range;
	double sensed_proximity; // what the eye is seeing. will be set in world.tick()
	double vx, vy;
	int sensed_type; // what does the eye see?
	
	void Init(double angle) {
		this->angle = angle;
		max_range = 120;
		sensed_proximity = 120;
		sensed_type = -1;
		vx = 0;
		vy = 0;
	}
};


class WaterWorldAgent {
	
protected:
	friend class WaterWorldCtrl;
	
public:
	WaterWorldAgent();
	void Forward();
	void Backward();
	void Reset();
	void LoadState(WaterWorldTrainer::MatType& mat);
	int GetExperienceWritePointer() {return item_cursor;}
	
	WaterWorld* world = NULL;
	
	Vector<WaterWorldTrainer::DQItem> train_items;
	WaterWorldTrainer dqn_trainer;
	Vector<Eye> eyes;
	Vector<double> smooth_reward_history;
	Vector<int> actions;
	Vector<Pointf> tail;
	Pointf p;		// positional information
	Pointf v;		// velocity
	Pointf op;		// old position
	double smooth_reward = 0.0;
	double reward = 0.0;
	double rad = 0.0;
	double digestion_signal = 0.0l;
	int add_item_step = 5;
	int max_items = 10000;
	int max_reward_history_size = 1000;
	int iter = 0;
	int action = -1;
	int max_tail = 100;
	int item_cursor = 0;
	bool do_training = true;
	
};




struct World : public Ctrl {
	Array<WaterWorldAgent> agents;
	Vector<Item> items;
	Vector<Wall> walls;
	int W, H;
	int clock;
	
	World();
	InterceptResult StuffCollide(Pointf p1, Pointf p2, bool check_walls, bool check_items);
	void Tick();
	virtual void Paint(Draw& d);
	
};




class WaterWorld : public DockWindow {
	
protected:
	friend class WaterWorldAgent;
	
	HeatmapTimeView<WaterWorldTrainer> network_view;
	TrainingGraph<WaterWorldTrainer> reward;
	World world;
	Label lbl_eps;
	SliderCtrl eps;
	Splitter btnsplit;
	Button goveryfast, gofast, gonorm, goslow, reset;
	ParentCtrl statusctrl;
	Label status;
	Button load_pretrained, load_btn, save_btn, reload_btn;
	ParentCtrl agent_ctrl;
	String t;
	int simspeed = 2;
	bool running = false, stopped = true;
	
	SpinLock ticking_lock;
	
public:
	typedef WaterWorld CLASSNAME;
	WaterWorld();
	~WaterWorld();
	
	virtual void DockInit();
	
	void Reset();
	void Load();
	void Save();
	void Start();
	void Reload();
	void Refresher();
	void RefreshEpsilon();
	void LoadPretrained();
	void RefreshStatus();
	void PostRefreshStatus() {PostCallback(THISBACK(RefreshStatus));}
	void Tick();
	void Ticking();
	void SetSpeed(int i);
	
};

#endif
