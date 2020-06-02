#include "lpm.h"
LPM::LPM()
{
	// WARNING: THIS FUNCTION REQUIRES init_LPM().
}

LPM::LPM(int total_cache_level, float delta, uint64_t new_window_width)
{
	cache_level_count = total_cache_level;
	ratio_memory_compute = delta;
	window_width = new_window_width;

	pure_miss_cycle = new int[cache_level_count];
	mix_cycle = new int[cache_level_count];
	pure_hit_cycle = new int[cache_level_count];
	active_cycle = new int[cache_level_count];
	ratio_miss_cycle_active_cycle = new float[cache_level_count];
	ratio_pure_miss_cycle_all_miss_cycle = new float[cache_level_count];

	lpmr = new float[cache_level_count];

	need_update = new bool[cache_level_count];

	cycle_stat = new struct cycle_stat_t*[cache_level_count];

	for (int i = 0; i < cache_level_count; i++)
	{
		cycle_stat[i] = new struct cycle_stat_t[window_width];
	}

	reset(0);
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

	pure_miss_cycle = new int[cache_level_count];
	mix_cycle = new int[cache_level_count];
	pure_hit_cycle = new int[cache_level_count];
	active_cycle = new int[cache_level_count];
	ratio_miss_cycle_active_cycle = new float[cache_level_count];
	ratio_pure_miss_cycle_all_miss_cycle = new float[cache_level_count];

	lpmr = new float[cache_level_count];

	need_update = new bool[cache_level_count];

	cycle_stat = new struct cycle_stat_t*[cache_level_count];

	for (int i = 0; i < cache_level_count; i++)
	{
		cycle_stat[i] = new struct cycle_stat_t[window_width];
	}

	reset(0);
}

void LPM::reset(uint64_t new_start_cycle)
{
	inst_count = 0;
	cycle_count = 0;
	f_mem = 0;
	access_count = 0;

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

		for (int cyc_idx = 0; cyc_idx < window_width; cyc_idx++)
		{
			cycle_stat[idx][cyc_idx].hit_count = 0;
			cycle_stat[idx][cyc_idx].miss_count = 0;
		}
	}
}

bool LPM::update_lpmr(int cache_level)
{
	if(!(cache_level < cache_level_count))
	{
		return false;
	}

	// update m,x,w,h
	for(int cyc_idx= 0; cyc_idx < window_width; cyc_idx++)
	{
		if(cycle_stat[cache_level][cyc_idx].miss_count > 0) // pure miss cycle or mix cycle
		{
			if(cycle_stat[cache_level][cyc_idx].hit_count > 0) // mix cycle
			{
				mix_cycle[cache_level]++;
			}
			else //pure miss cycle
			{
				pure_miss_cycle[cache_level]++;
			}
		}
		else // pure hit cycle or memory non-active cycle
		{
			if(cycle_stat[cache_level][cyc_idx].hit_count > 0) // pure hit cycle
			{
				pure_hit_cycle[cache_level]++;
			}
		}
	}
	active_cycle[cache_level] = pure_miss_cycle[cache_level] + mix_cycle[cache_level] 
		+ pure_hit_cycle[cache_level];

	f_mem = ((float)access_count) / ((float)cycle_count);
	ratio_miss_cycle_active_cycle[cache_level]
		= (float)(pure_miss_cycle[cache_level] + mix_cycle[cache_level])
		/ ((float)active_cycle[cache_level]);
	ratio_pure_miss_cycle_all_miss_cycle[cache_level]
		= ((float)pure_miss_cycle[cache_level])
		/ (float)(pure_miss_cycle[cache_level] + mix_cycle[cache_level]);

	float multiplex_ratio_miss_cycle_active_cycle = 1;
	if(cache_level > 0)
	{
		for(int idx = 0; idx < cache_level; idx++)
		{
			multiplex_ratio_miss_cycle_active_cycle *= ratio_miss_cycle_active_cycle[idx];
		}
	}
	// for test
	cout << "inst_count="<<inst_count<<", cycle_count="<<cycle_count<<", f_mem="<<f_mem<<", access_count="<<access_count<<", active_cycle="<<active_cycle<<", multiplex_ratio_miss_cycle_active_cycle="<<multiplex_ratio_miss_cycle_active_cycle<<endl;
	
	lpmr[cache_level] = ((float)inst_count) / ((float)cycle_count) 
			* f_mem * ((float)access_count) / ((float)active_cycle[0]) 
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

bool LPM::access_reg(int cache_level, uint64_t start_cycle, uint64_t cycle_count, bool type)
{
	if(!(cache_level < cache_level_count))
	{
		return false;
	}

	if(start_cycle < window_start_cycle)
	{
		return false;
	}

	if(type != LPM_HIT_ACCESS && type != LPM_MISS_ACCESS)
	{
		return false;
	}

	uint64_t start_cycle_idx = start_cycle - window_start_cycle;
	uint64_t end_cycle_idx = ((start_cycle_idx + cycle_count) < window_width) ? cycle_count : window_width;

	for(uint64_t cycle_idx = start_cycle_idx; cycle_idx < end_cycle_idx; ++cycle_idx)
	{
		if(type == LPM_HIT_ACCESS)
		{
			cycle_stat[cache_level][cycle_idx].hit_count++;
		}
		else
		{
			cycle_stat[cache_level][cycle_idx].miss_count++;
		}
	}
	// for test
	cout<<"lpm need update"<<endl;
	need_update[cache_level] = true;
	return true;
}

float LPM::get_lpmr(int cache_level)
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
		cout << "LPM::get_lpmr update"<<endl;
		update_lpmr(cache_level);
	}
	return lpmr[cache_level];
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