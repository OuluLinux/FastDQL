#include "WaterWorld.h"


WaterWorldAgent::WaterWorldAgent() {
	
	// positional information
	p.x = 300;
	p.y = 300;
	v.x = 0;
	v.y = 0;
	op = p;
	
	actions.Add(ACT_LEFT);
	actions.Add(ACT_RIGHT);
	actions.Add(ACT_UP);
	actions.Add(ACT_DOWN);
	
	// properties
	rad = 10;
	for (int k = 0; k < EYES_COUNT; k++) {
		eyes.Add().Init(k*0.21);
	}
	
	train_items.Reserve(max_items);
}

void WaterWorldAgent::Forward() {
	// in forward pass the agent simply behaves in the environment
	// create input to brain
	LoadState(input_array);
	dqn_action = dqn_trainer.Act(input_array);
    action = actions[dqn_action];
}

void WaterWorldAgent::LoadState(WaterWorldTrainer::MatType& mat) {
	ASSERT(mat.GetLength() == EYES_COUNT * 5 + 2);
	int ne = EYES_COUNT * 5;
	for (int i = 0; i < EYES_COUNT; i++) {
		Eye& e = eyes[i];
		mat.Set(i*5, 1.0);
		mat.Set(i*5+1, 1.0);
		mat.Set(i*5+2, 1.0);
		mat.Set(i*5+3, e.vx); // velocity information of the sensed target
		mat.Set(i*5+4, e.vy);
		if(e.sensed_type != -1) {
			// sensed_type is 0 for wall, 1 for food and 2 for poison.
			// lets do a 1-of-k encoding into the input array
			mat.Set(i*5 + e.sensed_type, e.sensed_proximity/e.max_range); // normalize to [0,1]
		}
	}
	
	// proprioception and orientation
	mat.Set(ne+0, v.x);
	mat.Set(ne+1, v.y);
}

void WaterWorldAgent::Backward() {
	reward = digestion_signal;
	
	// pass to brain for learning
	if (do_training) {
		
		if (iter % add_item_step == 0) {
			// Get item reference
			if (item_cursor < max_items)
				train_items.Add();
			WaterWorldTrainer::DQItem& item = train_items[item_cursor++];
			if (item_cursor >= max_items) item_cursor = 0;
			
			// Load current state
			item.before_state = input_array;
			item.before_action = dqn_action;
			item.after_reward = reward;
			LoadState(item.after_state);
			
			// Learn current action
			dqn_trainer.Learn(item);
			
			// Learn also few old actions
			for(int i = 0; i < 5; i++) {
				dqn_trainer.Learn(train_items[Random(train_items.GetCount())]);
			}
		}
		
		smooth_reward += reward;
		
		if (iter % 1000 == 0) {
			while (smooth_reward_history.GetCount() >= max_reward_history_size) {
				smooth_reward_history.Remove(0);
			}
			smooth_reward_history.Add(smooth_reward);
			
			world->reward.SetLimit(max_reward_history_size);
			world->reward.AddValue(smooth_reward);
			
			iter = 0;
		}
		iter++;
	}
}

void WaterWorldAgent::Reset() {
	dqn_trainer.Reset();
	
	rad = 10;
	action = 0;
	
	smooth_reward = 0.0;
	reward = 0;
}

