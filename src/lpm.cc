#include "lpm.h"
LPM::LPM()
{
	// WARNING: THIS FUNCTION REQUIRES init_LPM().
}

LPM::LPM(int total_cache_level, float delta, uint64_t new_window_width)
{
	init_LPM(total_cache_level, delta, new_window_width);
}

LPM::~LPM()
{
	_destroy_lpm();
}

void LPM::init_LPM(int total_cache_level, float delta, uint64_t new_window_width)
{
	cache_level_count = total_cache_level;
	ratio_memory_compute = delta;
	window_width = new_window_width;
	last_inst_num = 0;

	pure_miss_cycle = new int[cache_level_count];
	mix_cycle = new int[cache_level_count];
	pure_hit_cycle = new int[cache_level_count];
	active_cycle = new int[cache_level_count];
	ratio_miss_cycle_active_cycle = new float[cache_level_count];
	ratio_pure_miss_cycle_all_miss_cycle = new float[cache_level_count];

	lpmr = new float[cache_level_count];

	need_update = new bool[cache_level_count];

	cycle_stat = new struct cycle_stat_t*[cache_level_count];

	access_count = new int[cache_level_count];

	for (int i = 0; i < cache_level_count; i++)
	{
		cycle_stat[i] = new struct cycle_stat_t[window_width];
		access_count[i] = 0;
	}

	reset(0);
}

void LPM::reset(uint64_t new_start_cycle)
{
	inst_count = 0;
	//cycle_count = 0;
	f_mem = 0;
	//access_count = 0;

	window_start_cycle = new_start_cycle;

	for(int idx=0; idx<cache_level_count; idx++) 
	{
		pure_miss_cycle[idx] = 0;
		mix_cycle[idx] = 0;
		pure_hit_cycle[idx] = 0;
		active_cycle[idx] = 0;
		ratio_miss_cycle_active_cycle[idx] = 0;
		ratio_pure_miss_cycle_all_miss_cycle[idx] = 0;
		lpmr[idx] = 0;
		need_update[idx] = false;
		access_count[idx] = 0;

		for (int cyc_idx = 0; cyc_idx < window_width; cyc_idx++)
		{
			cycle_stat[idx][cyc_idx].hit_count = 0;
			cycle_stat[idx][cyc_idx].miss_count = 0;
		}
	}
}

bool LPM::update_lpmr(int cache_level, int inst_num, uint64_t current_cycle)
{
	if(!(cache_level < cache_level_count))
	{
		return false;
	}

	// update m,x,w,h
	for(int cache_level_idx = 0; cache_level_idx < cache_level_count; ++cache_level_idx)
	{
		mix_cycle[cache_level_idx] = 0;
		pure_miss_cycle[cache_level_idx] = 0;
		pure_hit_cycle[cache_level_idx] = 0;
		active_cycle[cache_level_idx] = 0;

		for(int cyc_idx= 0; cyc_idx < (current_cycle - window_start_cycle); cyc_idx++)
		{
			if(cycle_stat[cache_level_idx][cyc_idx].miss_count > 0) // pure miss cycle or mix cycle
			{
				if(cycle_stat[cache_level_idx][cyc_idx].hit_count > 0) // mix cycle
				{
					mix_cycle[cache_level_idx]++;
				}
				else //pure miss cycle
				{
					pure_miss_cycle[cache_level_idx]++;
				}
			}
			else // pure hit cycle or memory non-active cycle
			{
				if(cycle_stat[cache_level_idx][cyc_idx].hit_count > 0) // pure hit cycle
				{
					pure_hit_cycle[cache_level_idx]++;
				}
			}
		}
		active_cycle[cache_level_idx] = pure_miss_cycle[cache_level_idx] + mix_cycle[cache_level_idx] 
			+ pure_hit_cycle[cache_level_idx];
		ratio_miss_cycle_active_cycle[cache_level_idx]
			= (float)(pure_miss_cycle[cache_level_idx] + mix_cycle[cache_level_idx])
			/ ((float)active_cycle[cache_level_idx]);
		ratio_pure_miss_cycle_all_miss_cycle[cache_level_idx]
			= ((float)pure_miss_cycle[cache_level])
			/ (float)(pure_miss_cycle[cache_level] + mix_cycle[cache_level]);
	}
	

	inst_count = inst_num - last_inst_num;

	f_mem = ((float)access_count[0]) / ((float)inst_count);
	

	// float multiplex_ratio_miss_cycle_active_cycle = 1;
	// if(cache_level > 0)
	// {
	// 	for(int idx = 0; idx < cache_level; idx++)
	// 	{
	// 		multiplex_ratio_miss_cycle_active_cycle *= ratio_miss_cycle_active_cycle[idx];
	// 	}
	// }
	float multiplex_ratio_miss_cycle_active_cycle = 0;
	if(cache_level == 0)
	{
		multiplex_ratio_miss_cycle_active_cycle = 1;
	}
	else if(cache_level == 1)
	{
		multiplex_ratio_miss_cycle_active_cycle = ratio_miss_cycle_active_cycle[0];
	}
	else if(cache_level == 2)
	{
		multiplex_ratio_miss_cycle_active_cycle = ratio_miss_cycle_active_cycle[0] * ratio_miss_cycle_active_cycle[1];
	}


	// for test
	cout<<endl<<"window width: "<<window_width<<endl;
	cout<<endl<<"m[0]="<<pure_miss_cycle[0]<<", x[0]="<<mix_cycle[0]<<", h[0]="<<pure_hit_cycle[0]<<", w[0]="<<active_cycle[0]<<", div="<<ratio_miss_cycle_active_cycle[0]<<endl;
	cout<<"m[1]="<<pure_miss_cycle[1]<<", x[1]="<<mix_cycle[1]<<", h[1]="<<pure_hit_cycle[1]<<", w[1]="<<active_cycle[1]<<", div="<<ratio_miss_cycle_active_cycle[1]<<endl;
	cout<<"m[2]="<<pure_miss_cycle[2]<<", x[2]="<<mix_cycle[2]<<", h[2]="<<pure_hit_cycle[2]<<", w[2]="<<active_cycle[2]<<", div="<<ratio_miss_cycle_active_cycle[2]<<endl;
	cout << "inst_count="<<inst_count<<", cycle_count="<<cycle_count<<", f_mem="<<f_mem<<", access_count="<<access_count[cache_level]<<", active_cycle="<<active_cycle[cache_level]<<", multiplex_ratio_miss_cycle_active_cycle="<<multiplex_ratio_miss_cycle_active_cycle<<endl;

	lpmr[cache_level] = ((float)inst_count) / ((float)cycle_count) 
			* f_mem *  ((float)active_cycle[0]) / ((float)access_count[0])
			* multiplex_ratio_miss_cycle_active_cycle;

	need_update[cache_level] = false;
	return true;
}

bool LPM::check_perf_match(int cache_level)
{
	float multiplex_ratio_miss_cycle_active_cycle = 1;
	if(cache_level > 0)
	{
		for(int idx = 0; idx < cache_level; idx++)
		{
			multiplex_ratio_miss_cycle_active_cycle *= ratio_miss_cycle_active_cycle[idx];
		}
	}
	float threshold = ratio_memory_compute 
		/ (ratio_miss_cycle_active_cycle[0] * ratio_pure_miss_cycle_all_miss_cycle[0]) 
		* multiplex_ratio_miss_cycle_active_cycle;
	if (lpmr[cache_level] > threshold)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool LPM::access_reg(int cache_level, uint64_t event_cycle, uint64_t hit_latency, int type)
{

	if(!(cache_level < cache_level_count))
	{
		cout<<endl<<"access_reg fail: cache_level >= cache_level_count"<<endl;
		return false;
	}

	if(event_cycle < window_start_cycle)
	{
		cout<<endl<<"access_reg fail: start_cycle < window_start_cycle"<<endl;
		return false;
	}

	if(type != LPM_ACCESS_START && type != LPM_ACCESS_END)
	{
		cout<<endl<<"access_reg fail: unkonwn type"<<endl;
		return false;
	}

	if (type == LPM_ACCESS_END_EXTEND)
	{
		for(uint64_t cycle_idx=0; cycle_idx < event_cycle - window_start_cycle; cycle_idx++)
		{
			cycle_stat[cache_level][cycle_idx].miss_count++;
		}
		access_count[cache_level]++;
		need_update[cache_level] = true;
		return true;
	}
	
	uint64_t start_cycle_idx = event_cycle - window_start_cycle;
	//uint64_t end_cycle_idx = ((start_cycle_idx + cycle_count) < window_width) ? (start_cycle_idx+cycle_count) : window_width;

	for(uint64_t cycle_idx = start_cycle_idx; cycle_idx < window_width; ++cycle_idx)
	{
		if(type == LPM_ACCESS_START)
		{
			if(cycle_idx - start_cycle_idx < hit_latency)
			{
				cycle_stat[cache_level][cycle_idx].hit_count++;
				access_count[cache_level]++;
			}
			else
			{
				cycle_stat[cache_level][cycle_idx].miss_count++;
			}
		}
		else // LPM_ACCESS_END
		{
			cycle_stat[cache_level][cycle_idx].miss_count--;
		}
	}
	// for test
	//cout<<"lpm need update"<<endl;
	need_update[cache_level] = true;
	return true;
}

float LPM::get_lpmr(int cache_level, int inst_num, uint64_t current_cycle)
{
	if(!(cache_level < cache_level_count))
	{
		// for test
		//cout<<"LPM::get_lpmr fault"<<endl;
		return 0; // fault
	}
	if(need_update[cache_level]==true)
	{
		// for test
		//cout << "LPM::get_lpmr update"<<endl;
		update_lpmr(cache_level, inst_num, current_cycle);
	}
	return lpmr[cache_level];
}

void LPM::set_window_width(uint64_t new_window_width)
{
	window_width = new_window_width;

	for (int i = 0; i < cache_level_count; i++)
	{
		delete [] cycle_stat[i];
		cycle_stat[i] = new struct cycle_stat_t[window_width];
		for (int j = 0; j < window_width; j++)
		{
			cycle_stat[i][j].hit_count = 0;
			cycle_stat[i][j].miss_count = 0;
		}
	}
}

void LPM::_destroy_lpm()
{
	delete []pure_miss_cycle;
	delete []mix_cycle;
	delete []pure_hit_cycle;
	delete []active_cycle;
	delete []ratio_miss_cycle_active_cycle;
	delete []ratio_pure_miss_cycle_all_miss_cycle;
	delete []lpmr;
	delete []need_update;
	for(int i = 0; i < cache_level_count; i++)
	{
		delete []cycle_stat[i];
	}
	delete []cycle_stat;
}