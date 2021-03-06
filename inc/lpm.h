#ifndef LPM_H
#define LPM_H

#include "champsim.h"

#define LPM_L1 0
#define LPM_L2 1
#define LPM_L3 2
// Add new macro as LPM_Ln if there are more cache hierarchies

#define LPM_ACCESS_START 0
#define LPM_ACCESS_END 1
#define LPM_ACCESS_END_EXTEND 2

struct cycle_stat_t 
{
	int hit_count;
	int miss_count;
};

class LPM
{
public:
	LPM();
	LPM(int total_cache_level, float delta, uint64_t new_window_width);
	~LPM();

	void init_LPM(int total_cache_level, float delta, uint64_t new_window_width);
	void reset(uint64_t new_start_cycle);
	bool update_lpmr(int cache_level, int inst_num, uint64_t current_cycle);
	bool check_perf_match(int cache_level);
	bool access_reg(int cache_level, uint64_t event_cycle, uint64_t hit_latency, int type);
	float get_lpmr(int cache_level, int inst_num, uint64_t current_cycle);

	void set_delta(float delta){ratio_memory_compute = delta;};
	void set_cycle_count(int new_cycle_count){cout<<"cycle count="<<new_cycle_count; cycle_count = new_cycle_count;};
	void set_last_inst_num(int inst_num){last_inst_num = inst_num;};
	void set_window_width(uint64_t new_window_width);

private:
	int cache_level_count; 
	float ratio_memory_compute; // delta: the ratio of memory access time over compute time
	
	int* pure_miss_cycle; // m: pure miss cycle number
	int* mix_cycle; // x: number of mixed hit/miss cycles
	int* pure_hit_cycle; // h: number of pure hit cycles
	int* active_cycle; // w: number of memory active cycles, w=h+m+x
	float* ratio_miss_cycle_active_cycle; // miu: ratio of miss cycles over memory active cycles, miu=(m+x)/w
	float* ratio_pure_miss_cycle_all_miss_cycle; // k: ratio of pure miss cycles over miss cycles
	float* lpmr; // layer performance matching ratio of level i
	bool* need_update; // indicate whether the LPMR of the cache level requires update

	int last_inst_num; // instruction count when last access_reg() is accessed
	int inst_count; // IC: instruction count
	int cycle_count; // n: total number of CPU cycles
	float f_mem; // f_mem: average number of memory accesses per instrction
	int* access_count; // total number of memory access

	uint64_t window_start_cycle;
	uint64_t window_width;

	struct cycle_stat_t** cycle_stat; 

	void _destroy_lpm(); // WARNING: THIS FUNCTION WILL DELETE ALL DATA IN LPM.
};

#endif