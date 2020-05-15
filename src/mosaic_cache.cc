#include "mosaic_cache.h"

Mosaic_Cache::Mosaic_Cache(int new_core_num, int new_cache_level_count)
{
	// init system and cache configuration
	core_num = new_core_num;
	cache_level_count = new_cache_level_count;
	mosaic_cache_info = new struct mosaic_cache_info_t[cache_level_count];
	for(int idx = 0; idx < cache_level_count; idx++)
	{
		mosaic_cache_info[idx].need_set = true;
		mosaic_cache_info[idx].need_init = true;
	}

	// for rollback
	cache_info_snapshot = NULL;
	last_operation = 0;

	// init mosaic_cache configuration
	target_delta = 1;
	check_period = 500000;
	set_work_mode(0);
	set_writeback_mode(0);

	// init mosaic_cache information
	last_check_cycle = 0;

	// init lpm_monitor
	lpm_monitor = new LPM[core_num];
	for(int idx = 0; idx < core_num; idx++)
	{
		lpm_monitor[idx].init_LPM(cache_level_count, target_delta, check_period);
	}

	// init statistics
	_writeback_counter = new int*[core_num];
	for(int core_idx = 0; core_idx < core_num; ++core_idx)
	{
		_writeback_counter[core_idx] = new int[3];
		for(int i = 0; i < 3; ++i)
		{
			_writeback_counter[core_idx][i] = 0;
		}
	}
	_total_writeback_counter = 0;
	_l1_to_l2_counter = 0;
	_l2_to_l1_counter = 0;
	_l2_to_l3_counter = 0;
	_l3_to_l2_counter = 0;
}

Mosaic_Cache::~Mosaic_Cache()
{
	// output statistics
	print_statistics();

	delete[] lpm_monitor;
	delete[] mosaic_cache_info;
	delete[] cache_info_snapshot;
	for(int core_idx = 0; core_idx < core_num; core_idx++)
	{
		delete[] _writeback_counter[core_idx];
	}
	delete[] _writeback_counter;
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

void Mosaic_Cache::set_delta(float new_delta)
{
	target_delta = new_delta;
}

void Mosaic_Cache::set_check_period(uint64_t new_check_period)
{
	check_period = new_check_period;
}

bool Mosaic_Cache::set_mosaic_cache_info(int cache_level, int way_num, int adaptive_way_num, 
	int ratio, int reconfig_threshold, int latency)
{
	if(cache_level < LPM_L1 || cache_level > LPM_L3
		|| way_num < 0 || adaptive_way_num < 0 || adaptive_way_num > way_num
		|| reconfig_threshold <0 || reconfig_threshold >core_num || latency < 0)
	{
		return false;
	}

	mosaic_cache_info[cache_level].origin_way_num = way_num;
	mosaic_cache_info[cache_level].current_way_start_pos = 0;
	mosaic_cache_info[cache_level].current_way_end_pos = way_num;
	mosaic_cache_info[cache_level].adaptive_way_num = adaptive_way_num;
	mosaic_cache_info[cache_level].ratio_of_lower_level = ratio;
	mosaic_cache_info[cache_level].reconfig_threshold = reconfig_threshold;
	mosaic_cache_info[cache_level].latency = latency;
	mosaic_cache_info[cache_level].need_set = false;

	return true;
}

bool Mosaic_Cache::init_mosaic_cache()
{
	for(int cache_level_idx = 0; cache_level_idx < cache_level_count; cache_level_idx++)
	{
		if(mosaic_cache_info[cache_level_idx].need_set == true)
			return false;
	}

	for(int cache_level_idx = 0; cache_level_idx < LPM_L3; cache_level_idx++)
	{
		if(cache_level_idx == LPM_L1)
		{
			mosaic_cache_info[cache_level_idx].max_way_num =
				mosaic_cache_info[cache_level_idx].origin_way_num
				+ mosaic_cache_info[cache_level_idx+1].adaptive_way_num
				* mosaic_cache_info[cache_level_idx].ratio_of_lower_level / 2;
		}
		else if(cache_level_idx == LPM_L2)
		{
			mosaic_cache_info[cache_level_idx].max_way_num =
				mosaic_cache_info[cache_level_idx].origin_way_num
				+ mosaic_cache_info[cache_level_idx+1].adaptive_way_num
				* mosaic_cache_info[cache_level_idx].ratio_of_lower_level / core_num;
		}
		else if(cache_level_idx == LPM_L3)
		{
			mosaic_cache_info[cache_level_idx].max_way_num = 
				mosaic_cache_info[cache_level_idx].origin_way_num 
				+ mosaic_cache_info[cache_level_idx-1].adaptive_way_num * core_num 
				/ mosaic_cache_info[cache_level_idx-1].ratio_of_lower_level; 
		}
		mosaic_cache_info[cache_level_idx].need_init = false;
	}
	return true;
}

bool Mosaic_Cache::set_adaptive(int cache_level, int new_adaptive_way_num, int reconfig_threshold)
{
	if(cache_level<LPM_L1 || cache_level > LPM_L3)
		return false;
	if(mosaic_cache_info[cache_level].need_init == true)
		return false;
	if(mosaic_cache_info[cache_level].origin_way_num < new_adaptive_way_num)
		return false;
	if(reconfig_threshold > core_num)
		return false;
	mosaic_cache_info[cache_level].adaptive_way_num = new_adaptive_way_num;
	mosaic_cache_info[cache_level].reconfig_threshold = reconfig_threshold;
	if(init_mosaic_cache())
	{
		return true;
	}
	else
		return false;
}

int Mosaic_Cache::get_current_way_num(int cache_level)
{
	if(cache_level < LPM_L1 || cache_level > LPM_L3)
		return -1;
	if(mosaic_cache_info[cache_level].need_init == true)
		return -1;
	int ret_val = mosaic_cache_info[cache_level].current_way_end_pos 
		- mosaic_cache_info[cache_level].current_way_start_pos + 1;
	return ret_val;
}

int Mosaic_Cache::get_current_way_start_pos(int cache_level)
{
	if(cache_level < LPM_L1 || cache_level > LPM_L3)
		return -1;
	if(mosaic_cache_info[cache_level].need_init == true)
		return -1;
	return mosaic_cache_info[cache_level].current_way_start_pos;
}

int Mosaic_Cache::get_current_way_end_pos(int cache_level)
{
	if(cache_level < LPM_L1 || cache_level > LPM_L3)
		return -1;
	if(mosaic_cache_info[cache_level].need_init == true)
		return -1;
	return mosaic_cache_info[cache_level].current_way_end_pos;
}

int Mosaic_Cache::get_max_way_num(int cache_level)
{
	if(cache_level < LPM_L1 || cache_level > LPM_L3)
		return -1;
	if(mosaic_cache_info[cache_level].need_init == true)
		return -1;
	return mosaic_cache_info[cache_level].max_way_num;
}

bool Mosaic_Cache::need_check(uint64_t current_cycle)
{

	if(mosaic_cache_info[LPM_L1].need_init == true
		|| mosaic_cache_info[LPM_L2].need_init == true
		|| mosaic_cache_info[LPM_L3].need_init == true)
		return false;

	if(current_cycle < (last_check_cycle + check_period))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Mosaic_Cache::need_forward(uint64_t current_cycle)
{
	if(last_check_cycle + check_period < current_cycle)
		return true;
	else
		return false;
}

bool Mosaic_Cache::reconfig(uint64_t current_cycle)
{
	if(mosaic_cache_info[LPM_L1].need_init == true
		|| mosaic_cache_info[LPM_L2].need_init == true
		|| mosaic_cache_info[LPM_L3].need_init == true)
		return false;

	if(work_mode == 0 || work_mode == 1 || current_cycle < last_check_cycle)
		return false;

	if(work_mode == 2) // only for L1-L2
	{
		int vote=0;
		// L2->L1?
		for(int core_idx = 0; core_idx < core_num; core_idx++)
		{
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L1))
			{
				vote++;
			}
		}
		if(vote >= mosaic_cache_info[LPM_L1].reconfig_threshold)
		{
			// L2->L1
			if(_reconfig_l2_to_l1())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		
		// L1->L2?
		vote = 0;
		for(int core_idx = 0; core_idx < core_num; core_idx++)
		{
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L2)
				&& lpm_monitor[core_idx].check_perf_match(LPM_L1))
			{
				vote++;
			}
		}
		if(vote >= mosaic_cache_info[LPM_L2].reconfig_threshold)
		{
			// L1->L2
			if(_reconfig_l1_to_l2())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
	else if(work_mode == 3) // only for L2-L3
	{
		// L3->L2?
		int vote = 0;
		for(int core_idx = 0; core_idx < core_num; core_idx++)
		{
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L2))
			{
				vote++;
			}
		}
		if(vote >= mosaic_cache_info[LPM_L2].reconfig_threshold)
		{
			if(_reconfig_l3_to_l2())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		// L2->L3?
		vote = 0;
		for(int core_idx = 0; core_idx < core_num; core_idx++)
		{
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L3)
				&& lpm_monitor[core_idx].check_perf_match(LPM_L2))
			{
				vote++;
			}
		}
		if(vote >= mosaic_cache_info[LPM_L3].reconfig_threshold)
		{
			if(_reconfig_l2_to_l3())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
	else if(work_mode == 4) // L1-L2-L3
	{
		int vote_l1 = 0;
		int vote_l2 = 0;
		int vote_l3 = 0;
		// L2 first
		for(int core_idx = 0; core_idx < core_num; core_idx++)
		{
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L1))
			{
				vote_l1++;
			}
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L2))
			{
				vote_l2++;
			}
			if(!lpm_monitor[core_idx].check_perf_match(LPM_L3))
			{
				vote_l3++;
			}
		}

		if(vote_l2 >= mosaic_cache_info[LPM_L2].reconfig_threshold)
		{
			if(_reconfig_l3_to_l2())
			{
				return true;
			}
			else if(vote_l1 < mosaic_cache_info[LPM_L1].reconfig_threshold)
			{
				if(_reconfig_l1_to_l2())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else if(vote_l1 >= mosaic_cache_info[LPM_L1].reconfig_threshold)
		{
			if(_reconfig_l2_to_l1())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if(vote_l3 >= mosaic_cache_info[LPM_L3].reconfig_threshold)
		{
			if(_reconfig_l2_to_l3())
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		
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

	if(type == LPM_HIT_ACCESS)
	{
		cycle_count = mosaic_cache_info[cache_level].latency;
	}
	else if(start_cycle - last_check_cycle + cycle_count >= check_period)
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

void Mosaic_Cache::add_writeback(int core_id, int cache_level, int writeback_count)
{
	_writeback_counter[core_id][cache_level] += writeback_count;
	_total_writeback_counter += writeback_count;
}

void Mosaic_Cache::print_statistics()
{
	cout<<"====MOSAIC_CACHE_STAT_BEGIN===="<<endl;
	cout<<"TOTAL WRITEBACK: "<<_total_writeback_counter<<endl;
	for(int core_idx = 0; core_idx < core_num; core_idx++)
	{
		cout<<"-- Core ["<<core_idx<<"]"<<endl;
		for(int i = 0; i < 3; i++)
		{
			cout<<"---- Cache L"<<(i+1)<<": "<<_writeback_counter[core_idx][i]<<endl;
		}
	}
	cout<<"TOTAL CACHE RECONFIG: "<<_total_reconfig_counter<<endl;
	cout<<"-- L1 to L2: "<<_l1_to_l2_counter<<endl;
	cout<<"-- L2 to L1: "<<_l2_to_l1_counter<<endl;
	cout<<"-- L2 to L3: "<<_l2_to_l3_counter<<endl;
	cout<<"-- L3 to L2: "<<_l3_to_l2_counter<<endl;
	cout<<"====MOSAIC_CACHE_STAT_END===="<<endl;
}

bool Mosaic_Cache::_reconfig_l1_to_l2()
{
	if(mosaic_cache_info[LPM_L1].current_way_end_pos > mosaic_cache_info[LPM_L1].origin_way_num
		&& mosaic_cache_info[LPM_L2].current_way_start_pos > 0)
	{
		// L2 already sent adaptive block to L1, now return to L2
		int new_pos = mosaic_cache_info[LPM_L1].current_way_end_pos
			- mosaic_cache_info[LPM_L1].ratio_of_lower_level / 2;
		if(new_pos > mosaic_cache_info[LPM_L1].origin_way_num)
		{
			// create rollback information
			_snapshot();
			last_operation =1;

			mosaic_cache_info[LPM_L2].current_way_start_pos--;
			mosaic_cache_info[LPM_L1].current_way_end_pos = new_pos;
			return true;
		}
		else
		{
			return false;
		}
	}	
	else
	{
		return false;
	}
}

bool Mosaic_Cache::_reconfig_l2_to_l1()
{
	if(mosaic_cache_info[LPM_L1].current_way_end_pos < mosaic_cache_info[LPM_L1].max_way_num
		&& mosaic_cache_info[LPM_L2].current_way_start_pos < mosaic_cache_info[LPM_L2].adaptive_way_num-1)
	{
		int new_pos = mosaic_cache_info[LPM_L1].current_way_end_pos 
			+ mosaic_cache_info[LPM_L1].ratio_of_lower_level / 2;
		if(new_pos < mosaic_cache_info[LPM_L1].max_way_num)
		{
			// create rollback information
			_snapshot();
			last_operation = 2;

			mosaic_cache_info[LPM_L1].current_way_end_pos = new_pos;
			mosaic_cache_info[LPM_L2].current_way_start_pos++;
			return true;
		} 
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool Mosaic_Cache::_reconfig_l2_to_l3()
{
	if(mosaic_cache_info[LPM_L2].current_way_end_pos > mosaic_cache_info[LPM_L2].origin_way_num) 
	{
		// L3 already sent adaptive way to L2, now return to L3
		int new_pos = mosaic_cache_info[LPM_L2].current_way_end_pos 
			- mosaic_cache_info[LPM_L2].ratio_of_lower_level * 1 / core_num;
		if(new_pos >= mosaic_cache_info[LPM_L2].origin_way_num
			&& mosaic_cache_info[LPM_L3].current_way_end_pos < mosaic_cache_info[LPM_L3].origin_way_num)
		{
			// create rollback information
			_snapshot();
			last_operation = 3;

			mosaic_cache_info[LPM_L2].current_way_end_pos = new_pos;
			mosaic_cache_info[LPM_L3].current_way_end_pos++;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if(mosaic_cache_info[LPM_L2].current_way_start_pos 
		< mosaic_cache_info[LPM_L2].adaptive_way_num-1)
	{
		// L2's adaptive ways are available, now send to L3
		int new_pos = mosaic_cache_info[LPM_L2].current_way_start_pos
			+ mosaic_cache_info[LPM_L2].ratio_of_lower_level / core_num;
		if(new_pos < mosaic_cache_info[LPM_L2].adaptive_way_num
			&& mosaic_cache_info[LPM_L3].current_way_end_pos < mosaic_cache_info[LPM_L3].max_way_num)
		{
			// create rollback information
			_snapshot();
			last_operation = 3;

			mosaic_cache_info[LPM_L2].current_way_start_pos = new_pos;
			mosaic_cache_info[LPM_L3].current_way_end_pos++;
			return true;
		}
		else
		{
			return false;
		}
	}
	else // L2 has no block to send to L3
	{
		return false;
	}
}

bool Mosaic_Cache::_reconfig_l3_to_l2()
{
	if(mosaic_cache_info[LPM_L3].current_way_end_pos > mosaic_cache_info[LPM_L3].origin_way_num)
	{
		// L2 already sent its block to L3, now return to L2
		int new_pos = mosaic_cache_info[LPM_L2].current_way_start_pos 
			- mosaic_cache_info[LPM_L2].ratio_of_lower_level / core_num;
		if(new_pos >=0)
		{
			// create rollback information
			_snapshot();
			last_operation = 4;

			mosaic_cache_info[LPM_L2].current_way_start_pos = new_pos;
			mosaic_cache_info[LPM_L3].current_way_end_pos--;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if(mosaic_cache_info[LPM_L3].current_way_end_pos <= mosaic_cache_info[LPM_L3].origin_way_num
		&& mosaic_cache_info[LPM_L3].current_way_end_pos >= (mosaic_cache_info[LPM_L3].origin_way_num 
			- mosaic_cache_info[LPM_L3].adaptive_way_num))
	{
		// L3 has adaptive block for sending to L2
		int new_pos = mosaic_cache_info[LPM_L2].current_way_end_pos
			+ mosaic_cache_info[LPM_L2].ratio_of_lower_level / core_num;
		if(new_pos < mosaic_cache_info[LPM_L2].max_way_num)
		{
			// create rollback information
			_snapshot();
			last_operation = 4;

			mosaic_cache_info[LPM_L2].current_way_end_pos = new_pos;
			mosaic_cache_info[LPM_L3].current_way_end_pos--;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void Mosaic_Cache::_snapshot()
{
	if(cache_info_snapshot != NULL)
		delete[] cache_info_snapshot;

	cache_info_snapshot = new struct mosaic_cache_info_t[cache_level_count];

	for(int cache_idx = 0; cache_idx < cache_level_count; ++cache_idx)
	{
		cache_info_snapshot[cache_idx].need_set = mosaic_cache_info[cache_idx].need_set;
		cache_info_snapshot[cache_idx].need_init = mosaic_cache_info[cache_idx].need_init;
		cache_info_snapshot[cache_idx].origin_way_num = mosaic_cache_info[cache_idx].origin_way_num;
		cache_info_snapshot[cache_idx].current_way_start_pos = mosaic_cache_info[cache_idx].current_way_start_pos;
		cache_info_snapshot[cache_idx].current_way_end_pos = mosaic_cache_info[cache_idx].current_way_end_pos;
		cache_info_snapshot[cache_idx].max_way_num = mosaic_cache_info[cache_idx].max_way_num;
		cache_info_snapshot[cache_idx].adaptive_way_num = mosaic_cache_info[cache_idx].adaptive_way_num;
		cache_info_snapshot[cache_idx].ratio_of_lower_level = mosaic_cache_info[cache_idx].ratio_of_lower_level;
		cache_info_snapshot[cache_idx].reconfig_threshold = mosaic_cache_info[cache_idx].reconfig_threshold;
		cache_info_snapshot[cache_idx].latency = mosaic_cache_info[cache_idx].latency;
	}
}

void Mosaic_Cache::forward_window(uint64_t current_cycle)
{
	last_check_cycle = current_cycle;

	for(int core_idx = 0; core_idx < core_num; core_idx++)
	{
		lpm_monitor[core_idx].reset(current_cycle);
	}
}

bool Mosaic_Cache::RollBack()
{
	if(cache_info_snapshot == NULL || last_operation < 1 || last_operation > 4)
		return false;

	for(int cache_idx = 0; cache_idx < cache_level_count; cache_idx++)
	{
		mosaic_cache_info[cache_idx].need_set = cache_info_snapshot[cache_idx].need_set;
		mosaic_cache_info[cache_idx].need_init =cache_info_snapshot[cache_idx].need_init;
		mosaic_cache_info[cache_idx].origin_way_num = cache_info_snapshot[cache_idx].origin_way_num;
		mosaic_cache_info[cache_idx].current_way_start_pos = cache_info_snapshot[cache_idx].current_way_start_pos;
		mosaic_cache_info[cache_idx].current_way_end_pos = cache_info_snapshot[cache_idx].current_way_end_pos;
		mosaic_cache_info[cache_idx].max_way_num = cache_info_snapshot[cache_idx].max_way_num;
		mosaic_cache_info[cache_idx].adaptive_way_num = cache_info_snapshot[cache_idx].adaptive_way_num;
		mosaic_cache_info[cache_idx].ratio_of_lower_level = cache_info_snapshot[cache_idx].ratio_of_lower_level;
		mosaic_cache_info[cache_idx].reconfig_threshold = cache_info_snapshot[cache_idx].reconfig_threshold;
		mosaic_cache_info[cache_idx].latency = cache_info_snapshot[cache_idx].latency;
	}

	_total_reconfig_counter--;
	switch (last_operation)
	{
		case 1:
			_l1_to_l2_counter--;
			break;
		case 2:
			_l2_to_l1_counter--;
			break;
		case 3:
			_l2_to_l3_counter--;
			break;
		case 4:
			_l3_to_l2_counter--;
			break;
		default:
			return false;
	}

	delete[] cache_info_snapshot;
	cache_info_snapshot = NULL;
	return true;
}