#ifndef MOSAIC_CACHE_H
#define MOSAIC_CACHE_H

#include "lpm.h"
#include <iostream>

struct mosaic_cache_info_t
{
	bool need_set;
	bool need_init;
	int origin_way_num;
	int current_way_start_pos;
	int current_way_end_pos;
	int max_way_num;
	int adaptive_way_num;
	int ratio_of_lower_level;
	int reconfig_threshold;
};


class Mosaic_Cache
{
public:
	Mosaic_Cache(int new_core_num, int cache_level_count, float new_delta, uint64_t new_check_period);
	~Mosaic_Cache();

	bool set_work_mode(int new_mode);
	bool set_writeback_mode(int new_mode);

	bool set_mosaic_cache_info(int cache_level, int way_num, int adaptive_way_num, int ratio, int reconfig_threshold);
	bool set_adaptive_way_num(int cache_level, int new_adaptive_way_num);
	bool init_mosaic_cache();

	int get_current_way_num(int cache_level);
	int get_current_way_start_pos(int cache_level);
	int get_current_way_end_pos(int cache_level);

	int get_max_way_num(int cache_level);
	int get_writeback_mode(){return writeback_mode;};

	bool need_check(uint64_t current_cycle);

	bool reconfig(uint64_t current_cycle); 

	bool access_reg(int core_id, int cache_level, uint64_t start_cycle, uint64_t cycle_count, bool type);
	
	float get_lpmr(int core_id, int cache_level);

	// for statistics
	void add_writeback(int core_id, int cache_level, int writeback_count);
	void print_statistics();

private:
	LPM* lpm_monitor;

	// system configuration
	int core_num;

	// cache configuration
	int cache_level_count;

	uint64_t last_check_cycle;

	struct mosaic_cache_info_t *mosaic_cache_info;

	// mosaic cache configuration
	
	float target_delta; 	// delta: the ratio of memory access time over compute time, 
							// the smaller delta indicates better performance
	
	uint64_t check_period; 	// mosaic cache periodically checks whether the cache hierarchy 
						   	// requires reconfiguration, the 'check period' indicates the 
						   	// cycle number between two checkings.
						 
	int work_mode; 	// the working mode of mosaic cache
					// 0: off
					// 1: motivation mode
					// 2: only for l1-l2
					// 3: only for l2-l3
					// 4: for l1-l2-l3

	int writeback_mode;	// 0: directly writeback, 1: non-writeback

	bool _reconfig_l1_to_l2();
	bool _reconfig_l2_to_l1();
	bool _reconfig_l2_to_l3();
	bool _reconfig_l3_to_l2();

	// for statistics
	int** _writeback_counter;
	int _total_writeback_counter;
	int _l1_to_l2_counter;
	int _l2_to_l3_counter;
	int _l2_to_l1_counter;
	int _l3_to_l2_counter;
	int _total_reconfig_counter;
};

#endif