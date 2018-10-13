#ifndef _FastDQL_Agent_h_
#define _FastDQL_Agent_h_

#include "Recurrent.h"

namespace FastDQL {

enum {ACT_LEFT, ACT_UP, ACT_RIGHT, ACT_DOWN, ACT_IDLE};






struct DQNet {
	MatId W1;
	MatId b1;
	MatId W2;
	MatId b2;
	
	void Load(const ValueMap& map);
	void Store(ValueMap& map);
	void Serialize(Stream& s) {s % W1 % b1 % W2 % b2;}
};



struct DQExperience : Moveable<DQExperience> {
	Mat state0, state1;
	int action0, action1;
	double reward0;
	
	void Set(Mat& state0, int action0, double reward0, Mat& state1, int action1) {
		this->state0 = state0;
		this->action0 = action0;
		this->reward0 = reward0;
		this->state1 = state1;
		this->action1 = action1;
	}
	void Serialize(Stream& s) {s % state0 % state1 % action0 % action1 % reward0;}
};

class DQNAgent : public MatPool {
	
	DQNet net;
	Graph G;
	Vector<DQExperience> exp; // experience
	double gamma, epsilon, alpha, tderror_clamp;
	double tderror;
	int experience_add_every, experience_size;
	int learning_steps_per_iteration;
	int num_hidden_units;
	int expi;
	int nh;
	int ns;
	int na;
	int t;
	bool has_reward;
	
	MatId state;
	Mat state0, state1;
	int action0, action1;
	double reward0;
	
	int width, height, length, action_count;
	
public:

	DQNAgent();
	
	void Init(int width, int height, int action_count=0);
	bool LoadInitJSON(const String& json);
	int GetNumStates() {return length;}
	int GetMaxNumActions() {return action_count;}
	
	void LoadInit(const ValueMap& map);
	void Reset();
	
	int GetExperienceWritePointer() const {return expi;}
	double GetTDError() const {return tderror;}
	double GetEpsilon() const {return epsilon;}
	Graph& GetGraph() {return G;}
	int GetExperienceCount() const {return exp.GetCount();}
	void ClearExperience() {exp.Clear();}
	
	void SetEpsilon(double e) {epsilon = e;}
	void SetGamma(double g) {gamma = g;}
	
	int Act(const Vector<double>& slist);
	void Evaluate(const Vector<double>& in, Vector<double>& out);
	void Learn(double reward1, bool force_experience=false);
	void Learn(const Vector<double>& in, const Vector<double>& out);
	double LearnFromTuple(Mat& s0, int a0, double reward0, Mat& s1, int a1);
	
	void Serialize(Stream& s) {
		s % exp;
		SerializeWithoutExperience(s);
	}
	
	void SerializeWithoutExperience(Stream& s) {
		MatPool::Serialize(s);
		s % net
		  % G
		  % exp
		  % gamma % epsilon % alpha % tderror_clamp
		  % tderror
		  % experience_add_every % experience_size
		  % learning_steps_per_iteration
		  % num_hidden_units
		  % expi
		  % nh
		  % ns
		  % na
		  % t
		  % has_reward
		  % state
		  % state0 % state1
		  % action0 % action1
		  % reward0
		  
		  % width % height % length % action_count;
		if (s.IsLoading())
			FixPool();
	}
	
	
	void FixPool() {G.SetPool(*this); G.FixPool();}
	
};

















void	RandMat(int n, int d, double mu, double std, Mat& out);
int		SampleWeighted(Vector<double>& p);
void	UpdateMat(Mat& m, double alpha);

}

#endif
