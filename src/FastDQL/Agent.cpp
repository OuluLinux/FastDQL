#include "FastDQL.h"
#include <random>


namespace FastDQL {

#define STOREVAR(json, field) map.GetAdd(#json) = this->field;
#define STOREVAR_(field) map.GetAdd(#field) = this->field;
#define LOADVAR(field, json) this->field = map.GetValue(map.Find(#json));
#define LOADVAR_(field) this->field = map.GetValue(map.Find(#field));
#define LOADVARDEF(field, json, def) {Value tmp = map.GetValue(map.Find(#json)); if (tmp.IsNull()) this->field = def; else this->field = tmp;}
#define LOADVARDEF_(field, def) {Value tmp = map.GetValue(map.Find(#field)); if (tmp.IsNull()) this->field = def; else this->field = tmp;}
#define LOADVARDEFTEMP(field, json, def) {Value tmp = map.GetValue(map.Find(#json)); if (tmp.IsNull()) field = def; else field = tmp;}

// return Mat but filled with random numbers from gaussian
void RandMat(int n, int d, double mu, double std, Mat& m) {
	m.Init(d, n, 0);
	
	std::default_random_engine generator;
	std::normal_distribution<double> distribution(mu, std);
	generator.seed(Random(INT_MAX));
	
	for (int i = 0; i < m.GetLength(); i++)
		m.Set(i, distribution(generator));
}

int SampleWeighted(Vector<double>& p) {
	ASSERT(!p.IsEmpty());
	double r = Randomf();
	double c = 0.0;
	for (int i = 0; i < p.GetCount(); i++) {
		c += p[i];
		if (c >= r)
			return i;
	}
	Panic("Invalid input vector");
	return 0;
}

void UpdateMat(Mat& m, double alpha) {
	// updates in place
	for (int i = 0; i < m.GetLength(); i++) {
		double d = m.GetGradient(i);
		if (d != 0.0) {
			m.Add(i, -alpha * d);
			m.SetGradient(i, 0);
		}
	}
}




















DQNAgent::DQNAgent() {
	G.SetPool(*this);
	
	gamma = 0.75; // future reward discount factor
	epsilon = 0.1; // for epsilon-greedy policy
	alpha = 0.01; // value function learning rate
	
	experience_add_every = 25; // number of time steps before we add another experience to replay memory
	experience_size = 5000; // size of experience replay
	learning_steps_per_iteration = 10;
	tderror_clamp = 1.0;
	
	num_hidden_units = 100;
	
	expi = 0;
	t = 0;
	reward0 = 0;
	action0 = 0;
	action1 = 0;
	has_reward = false;
	tderror = 0;
	
}

void DQNAgent::Init(int width, int height, int action_count) {
	ASSERT(width > 0 && height > 0);
	
	if (action_count <= 0) {
		action_count =
			(width  > 1 ? 2 : 1) *
			(height > 1 ? 2 : 1);
	}
	
	// hardcoding one gridworld for now
	this->width		= width;
	this->height	= height;
	length = height * width; // number of states
	this->action_count = action_count;
	
	Reset();
}

void DQNAgent::Reset() {
	ClearPool();
	
	nh = num_hidden_units; // number of hidden units
	ns = GetNumStates();
	na = GetMaxNumActions();
	
	RandMat(nh, ns, 0, 0.01, net.W1);
	InitMat(net.b1, 1, nh, 0);
	//net.b1 = RandMat(nh, 1, 0, 0.01);
	RandMat(na, nh, 0, 0.01, net.W2);
	InitMat(net.b2, 1, na, 0);
	//net.b2 = RandMat(na, 1, 0, 0.01);
	InitMat(state);
	
	// Original:
	//		var a1mat = G.Add(G.Mul(net.W1, s), net.b1);
	//		var h1mat = G.Tanh(a1mat);
	//		var a2mat = G.Add(G.Mul(net.W2, h1mat), net.b2);
	G.Clear();
	G.Mul(net.W1);
	G.Add(net.b1);
	G.Tanh();
	G.Mul(net.W2);
	G.Add(net.b2);
	
	expi = 0; // where to insert
	
	t = 0;
	
	reward0 = 0;
	action0 = 0;
	action1 = 0;
	has_reward = false;
	
	tderror = 0; // for visualization only...
}

bool DQNAgent::LoadInitJSON(const String& json) {
	
	Value js = ParseJSON(json);
	if (js.IsNull()) {
		LOG("JSON parse failed");
		return false;
	}
	
	ValueMap map = js;
	LoadInit(map);
	return true;
}

void DQNAgent::LoadInit(const ValueMap& map) {
	LOADVARDEF(gamma, gamma, 0.75); // future reward discount factor
	LOADVARDEF(epsilon, epsilon, 0.1); // for epsilon-greedy policy
	LOADVARDEF(alpha, alpha, 0.01); // value function learning rate
	LOADVARDEF(experience_add_every, experience_add_every, 25); // number of time steps before we add another experience to replay memory
	LOADVARDEF(experience_size, experience_size, 5000); // size of experience replay
	LOADVARDEF(learning_steps_per_iteration, learning_steps_per_iteration, 10);
	LOADVARDEF(tderror_clamp, tderror_clamp, 1.0);
	LOADVARDEF(num_hidden_units, num_hidden_units, 100);
}

int DQNAgent::Act(const Vector<double>& slist) {
	
	// convert to a Mat column vector
	Mat& state_mat = Get(state);
	state_mat.Init(width, height, slist);
	
	// epsilon greedy policy
	int action;
	if (Randomf() < epsilon) {
		action = Random(na);
	} else {
		// greedy wrt Q function
		//Mat& amat = ForwardQ(net, state);
		MatId a = G.Forward(state);
		Mat& amat = Get(a);
		action = amat.GetMaxColumn(); // returns index of argmax action
	}
	
	// shift state memory
	state0 = state1;
	action0 = action1;
	state1 = state_mat;
	action1 = action;
	
	return action;
}

void DQNAgent::Learn(double reward1, bool force_experience) {
	
	// perform an update on Q function
	if (has_reward && alpha > 0 && state0.GetLength() > 0) {
		
		// learn from this tuple to get a sense of how "surprising" it is to the agent
		tderror = LearnFromTuple(state0, action0, reward0, state1, action1); // a measure of surprise
		
		// decide if we should keep this experience in the replay
		if (t % experience_add_every == 0 || force_experience) {
			if (exp.GetCount() == expi)
				exp.Add();
			ASSERT(state1.GetLength() > 0);
			exp[expi].Set(state0, action0, reward0, state1, action1);
			expi += 1;
			if (expi >= experience_size) { expi = 0; } // roll over when we run out
		}
		t += 1;
		
		if (!exp.IsEmpty()) {
			// sample some additional experience from replay memory and learn from it
			for (int k = 0; k < learning_steps_per_iteration; k++) {
				int ri = Random(exp.GetCount()); // TODO: priority sweeps?
				DQExperience& e = exp[ri];
				LearnFromTuple(e.state0, e.action0, e.reward0, e.state1, e.action1);
			}
		}
	}
	reward0 = reward1; // store for next update
	has_reward = true;
}

double DQNAgent::LearnFromTuple(Mat& s0, int a0, double reward0, Mat& s1, int a1) {
	ASSERT(s0.GetLength() > 0);
	ASSERT(s1.GetLength() > 0);
	// want: Q(s,a) = r + gamma * max_a' Q(s',a')
	
	MatId s0_id = AddTempMat(s0);
	MatId s1_id = AddTempMat(s1);
	
	// compute the target Q value
	MatId t = G.Forward(s1_id);
	Mat& tmat = Get(t);
	double qmax = reward0 + gamma * tmat.Get(tmat.GetMaxColumn());
	
	// now predict
	MatId pred = G.Forward(s0_id);
	Mat& pred_mat = Get(pred);
	
	double tderror = pred_mat.Get(a0) - qmax;
	double clamp = tderror_clamp;
	if (fabs(tderror) > clamp) {  // huber loss to robustify
		if (tderror > clamp)
			tderror = +clamp;
		else
			tderror = -clamp;
	}
	pred_mat.SetGradient(a0, tderror);
	G.Backward(); // compute gradients on net params
	
	// update net
	UpdateMat(Get(net.W1), alpha);
	UpdateMat(Get(net.b1), alpha);
	UpdateMat(Get(net.W2), alpha);
	UpdateMat(Get(net.b2), alpha);
	
	ClearTempMat();
	
	return tderror;
}

void DQNAgent::Evaluate(const Vector<double>& in, Vector<double>& out) {
	
	// convert to a Mat column vector
	Mat& state_mat = Get(state);
	state_mat.Init(width, height, in);
	
	// greedy wrt Q function
	//Mat& amat = ForwardQ(net, state);
	MatId a = G.Forward(state);
	Mat& mat = Get(a);
	
	out.SetCount(mat.GetLength());
	for(int i = 0; i < mat.GetLength(); i++)
		out[i] = mat.Get(i);
	
}

void DQNAgent::Learn(const Vector<double>& in, const Vector<double>& out) {
	// convert to a Mat column vector
	Mat& state_mat = Get(state);
	state_mat.Init(width, height, in);
	
	// now predict
	MatId pred = G.Forward(state);
	Mat& pred_mat = Get(pred);
	
	for(int i = 0; i < out.GetCount(); i++) {
		double tderror = pred_mat.Get(i) - out[i];
		double clamp = tderror_clamp;
		if (fabs(tderror) > clamp) {  // huber loss to robustify
			if (tderror > clamp)
				tderror = +clamp;
			else
				tderror = -clamp;
		}
		pred_mat.SetGradient(i, tderror);
	}
	G.Backward(); // compute gradients on net params
	
	// update net
	UpdateMat(Get(net.W1), alpha);
	UpdateMat(Get(net.b1), alpha);
	UpdateMat(Get(net.W2), alpha);
	UpdateMat(Get(net.b2), alpha);
	
	ClearTempMat();
	
}


}
