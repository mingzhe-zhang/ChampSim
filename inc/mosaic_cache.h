#ifndef MOSAIC_CACHE_H
#define MOSAIC_CACHE_H

#include "lpm.h"

struct mosaic_cache_info_t
{
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
	Mosaic_Cache(int new_core_num, int new_cache_level_count, int l1_way, int l2_way, 
				 int l3_way, int new_l1_l2_ratio, int new_l2_l3_ratio,
				 float new_delta, uint64_t new_check_period);
	~Mosaic_Cache();

	bool set_work_mode(int new_mode);
	bool set_writeback_mode(int new_mode);

	bool set_adaptive_way_num(int cache_level, int new_adaptive_way_num);

	int get_current_way_num(int cache_level);
	int get_current_way_start_pos(int cache_level);
	int get_current_way_end_pos(int cache_level);

	int get_max_way_num(int cache_level);
	int get_writeback_mode(){return writeback_mode;};

	bool need_check(uint64_t current_cycle);

	bool reconfig(uint64_t current_cycle); 

	bool access_reg(int core_id, int cache_level, uint64_t start_cycle, uint64_t cycle_count, bool type);
	
	float get_lpmr(int core_id, int cache_level);


private:
	LPM* lpm_monitor;

	// system configuration
	int core_num;

	// cache configuration
	int cache_level_count;
	// int origin_l1_way_num;
	// int origin_l2_way_num;
	// int origin_l3_way_num;
	// int l1_l2_ratio;
	// int l2_l3_ratio;

	// // mosaic cache information
	// int current_l1_way_num;
	// int current_l2_way_num;
	// int current_l3_way_num;

	// int current_l1_way_end_pos;
	// int current_l2_way_start_pos;
	// int current_l2_way_end_pos;
	// int current_l3_way_end_pos;

	// int adaptive_way_l2;
	// int adaptive_way_l3;

	uint64_t last_check_cycle;

	struct mosaic_cache_info_t *mosaic_cache_info;

	// int reconfig_threshold_l1;
	// int reconfig_threshold_l2;
	// int reconfig_threshold_l3;

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
};

#endif