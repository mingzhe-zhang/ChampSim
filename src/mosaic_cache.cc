#include "mosaic_cache.h"

Mosaic_Cache::Mosaic_Cache(int new_core_num, int new_cache_level_count, int l1_way, int l2_way, 
				 int l3_way, int new_l1_l2_ratio, int new_l2_l3_ratio,
				 float new_delta, uint64_t new_check_period)
{
	// init system and cache configuration
	core_num = new_core_num;
	cache_level_count = new_cache_level_count;

	origin_l1_way_num = l1_way;
	origin_l2_way_num = l2_way;
	origin_l3_way_num = l3_way;
	l1_l2_ratio = new_l1_l2_ratio;
	l2_l3_ratio = new_l2_l3_ratio;

	// init mosaic_cache configuration
	target_delta = new_delta;
	check_period = new_check_period;
	set_work_mode(0);
	set_writeback_mode(0);

	// init mosaic_cache information
	current_l1_way_num = origin_l1_way_num;
	current_l2_way_num = origin_l2_way_num;
	current_l3_way_num = origin_l3_way_num;

	current_l1_way_end_pos = origin_l1_way_num;
	current_l2_way_start_pos = 0;
	current_l2_way_end_pos = origin_l2_way_num;
	current_l3_way_end_pos = origin_l3_way_num;

	last_check_cycle = 0;

	// init cache info
	mosaic_cache_info = new struct mosaic_cache_info_t[cache_level_count];

	// init lpm_monitor
	lpm_monitor = new LPM[core_num];
	for(int idx = 0; idx < core_num; idx++)
	{
		lpm_monitor[idx].init_LPM(cache_level_count, target_delta, check_period);
	}
}

Mosaic_Cache::~Mosaic_Cache()
{
	delete[] lpm_monitor;
	delete[] mosaic_cache_info;
}

bool Mosaic_Cache::set_work_mode(int new_mode)
{
	if(new_mode < 0 || new_mode > 4)
		return false;
	work_mode = new_mode;
	return true;
}

bool Mosaic_Cache::set_writeback_mode(int new_mode)
{
	if(new_mode < 0 || new_mode > 1)
		return false;
	writeback_mode = new_mode;
	return true;
}

bool Mosaic_Cache::set_adaptive_way_num(int cache_level, int new_adaptive_way_num)
{
	if(cache_level == LPM_L2)
	{
		if(new_adaptive_way_num > origin_l2_way_num)
		{
			return false;
		}
		else
		{
			adaptive_way_l2 = new_adaptive_way_num;
			return true;
		}
	}

	if(cache_level == LPM_L3)
	{
		if(new_adaptive_way_num > origin_l3_way_num)
		{
			return false;
		}
		else
		{
			adaptive_way_l3 = new_adaptive_way_num;
			return true;
		}
	}

	return false;
}

int Mosaic_Cache::get_current_way_num(int cache_level)
{
	int ret_val;
	switch(cache_level)
	{
		case LPM_L1:
		{
			ret_val = current_l1_way_num;
			break;
		}
		case LPM_L2:
		{
			ret_val = current_l2_way_num;
			break;
		}
		case LPM_L3:
		{
			ret_val = current_l3_way_num;
			break;
		}
		default:
		{
			ret_val = -1;
			break;
		}
	}
	return ret_val;
}

int Mosaic_Cache::get_current_way_start_pos(int cache_level)
{
	int ret_val;
	switch(cache_level)
	{
		case LPM_L1:
		{
			ret_val = 0;
			break;
		}
		case LPM_L2:
		{
			ret_val = current_l2_way_start_pos;
			break;
		}
		case LPM_L3:
		{
			ret_val = 0;
			break;
		}
		default:
		{
			ret_val = -1;
			break;
		}
	}
	return ret_val;
}

int Mosaic_Cache::get_current_way_end_pos(int cache_level)
{
	int ret_val;
	switch(cache_level)
	{
		case LPM_L1:
		{
			ret_val = current_l1_way_end_pos;
			break;
		}
		case LPM_L2:
		{
			ret_val = current_l2_way_end_pos;
			break;
		}
		case LPM_L3:
		{
			ret_val = current_l3_way_end_pos;
			break;
		}
		default:
		{
			ret_val = -1;
			break;
		}
	}
	return ret_val;
}

int Mosaic_Cache::get_max_way_num(int cache_level)
{
	int ret_val = -1;
	switch(cache_level)
	{
		case LPM_L1:
		{
			ret_val = origin_l1_way_num + adaptive_way_l2 * l1_l2_ratio / 2;
			break;
		}
		case LPM_L2:
		{
			ret_val = origin_l2_way_num + adaptive_way_l3 * l2_l3_ratio / core_num;
			break;
		}
		case LPM_L3:
		{
			ret_val = origin_l3_way_num + adaptive_way_l2 * core_num / l2_l3_ratio;
			break;
		}
		default: break;
	}
	return ret_val;
}

bool Mosaic_Cache::need_check(uint64_t current_cycle)
{
	if(current_cycle < (last_check_cycle + check_period))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Mosaic_Cache::reconfig(uint64_t current_cycle)
{
	if(work_mode == 0 || work_mode == 1 || current_cycle < last_check_cycle)
		return false;
	if(work_mode == 2) // only for L1-L2
	{
		// L2->L1?
		int vote = 0;
		for(int core_idx = 0; core_idx < core_num; core_idx++)
		{
			if(lpm_monitor[core_idx].check_perf_match(LPM_L1))
			{
				vote++;
			}
		}
		if(vote >= (int)(core_num/2))
	}
	else if(work_mode == 3) // only for L2-L3
	{

	}
	else if(work_mode == 4) // L1-L2-L3
	{

	}

	return false;
}

bool Mosaic_Cache::access_reg(int core_id, int cache_level, uint64_t start_cycle, uint64_t cycle_count, bool type)
{
	if(core_id < 0 || core_id >= core_num
		|| cache_level < LPM_L1 || cache_level > LPM_L3
		|| type < LPM_HIT_ACCESS || type > LPM_MISS_ACCESS
		|| start_cycle < last_check_cycle)
	{
		return false;
	}

	if(start_cycle - last_check_cycle + cycle_count >= check_period)
	{
		cycle_count = check_period - (start_cycle - last_check_cycle);
	}

	lpm_monitor[core_id].access_reg(cache_level, start_cycle, cycle_count, type);
	return true;
}
	
float Mosaic_Cache::get_lpmr(int core_id, int cache_level)
{
	if(core_id < 0 || core_id >= core_num || cache_level < LPM_L1 || cache_level > LPM_L3)
	{
		return 0; // fault
	}

	return lpm_monitor[core_id].get_lpmr(cache_level);
}