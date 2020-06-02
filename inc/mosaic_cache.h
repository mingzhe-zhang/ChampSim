// ====================================
// ======== MOSAIC CACHE USAGE ========
// ====================================
// STEP 1: instantiate Mosaic_Cache with Mosaic_Cache();
// STEP 2: initialize the Mosaic_Cache
// 		2-1 set_work_mode()
// 		2-2 set_writeback_mode()
// 		2-3 set_delta()
// 		2-4 set_check_period()
// 		2-5 set_mosaic_cache_info() (you can only reset adaptive info with set_adaptive())
// 		2-6 init_mosaic_cache()
// STEP 3: get information for host simulator initilization with get_max_way_num()
// STEP 4: register cache accesses during the runtime, using access_reg()
// STEP 5: dynamic way information for each cache access by using get_current_way_num(), 
// 		   get_current_way_start_pos() or get_current_way_end_pos().
// STEP 6: Periodically check the mode of mosaic cache
// 	 	6-1 For motivation mode, periodically output the lpmr by using get_lpmr(); 
// 	 	6-2 For other modes, check if the mosaic cache needs reconfig by using need_check(), if the 
// 	 		return is true, use reconfig(). As the return of reconfig() is true, issue writebacks 
// 	 		according to the return of get_writeback_mode(), and then register the writebacks with
// 	 		add_writeback().
// STEP 7: periodically check if the mosaic cache requires to forward the stat window with need_forward(),
// 		   if the return is true, call forward_window()
// STEP 8: output the statistics with print_statistics()
// ====================================
// ======== USAGE END, ENJOY! =========
// ====================================
#pragma once		 
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
	int latency;
};


class Mosaic_Cache
{
public:
	Mosaic_Cache(int new_core_num, int cache_level_count);
	Mosaic_Cache(){};
	void Mosaic_Cache_Global_Init(int new_core_num, int cache_level_count);
	~Mosaic_Cache();

	bool set_work_mode(int new_mode);
	bool set_writeback_mode(int new_mode);
	void set_delta(float new_delta);
	void set_check_period(uint64_t new_check_period);

	bool set_mosaic_cache_info(int cache_level, int way_num, int adaptive_way_num, int ratio, int reconfig_threshold, int latency);
	bool set_adaptive(int cache_level, int new_adaptive_way_num, int reconfig_threshold);
	bool init_mosaic_cache();

	void forward_window(uint64_t current_cycle);

	int get_last_operation(){return last_operation;};
	
	bool RollBack();

	int get_current_way_num(int cache_level);
	int get_current_way_start_pos(int cache_level);
	int get_current_way_end_pos(int cache_level);

	int get_max_way_num(int cache_level);
	int get_writeback_mode(){return writeback_mode;};
	int get_work_mode(){cout<<"addr work_mode"<<&work_mode<<endl;return work_mode;};

	bool need_check(uint64_t current_cycle);
	bool need_forward(uint64_t current_cycle);

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
	// for rollback
	struct mosaic_cache_info_t *cache_info_snapshot;

	// mosaic cache configuration
	
	static float target_delta; 	// delta: the ratio of memory access time over compute time, 
							// the smaller delta indicates better performance
	
	static uint64_t check_period; 	// mosaic cache periodically checks whether the cache hierarchy 
						   	// requires reconfiguration, the 'check period' indicates the 
						   	// cycle number between two checkings.
						 
	static int work_mode; 	// the working mode of mosaic cache
					// 0: off
					// 1: motivation mode
					// 2: only for l1-l2
					// 3: only for l2-l3
					// 4: for l1-l2-l3

	static int writeback_mode;	// 0: directly writeback, 1: non-writeback

	bool _reconfig_l1_to_l2();
	bool _reconfig_l2_to_l1();
	bool _reconfig_l2_to_l3();
	bool _reconfig_l3_to_l2(); 

	// for rollback
	void _snapshot();
	int last_operation;	// the last operation type
						// 0: none
						// 1: l1 to l2
						// 2: l2 to l1
						// 3: l2 to l3
						// 4: l3 to l2

	// for statistics
	int** _writeback_counter;
	int _total_writeback_counter;
	int _l1_to_l2_counter;
	int _l2_to_l3_counter;
	int _l2_to_l1_counter;
	int _l3_to_l2_counter;
	int _total_reconfig_counter;
};

extern Mosaic_Cache Mosaic_Cache_Monitor; // = new Mosaic_Cache(NUM_CPUS, 3);

#endif