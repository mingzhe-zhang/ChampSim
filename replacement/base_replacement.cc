#include "cache.h"

uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // baseline LRU replacement policy for other caches 
    return lru_victim(cpu, instr_id, set, current_set, ip, full_addr, type); 
}

void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    if (type == WRITEBACK) {
        if (hit) // wrietback hit does not update LRU state
            return;
    }

    return lru_update(set, way);
}

uint32_t CACHE::lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // zmz modify (STEP 5)
    int current_way_start_pos=0, current_way_end_pos=NUM_WAY;
    if(Mosaic_Cache_Monitor.get_work_mode() != 0)
    {
        switch(cache_type)
        {
            case IS_L1D:
                current_way_start_pos = Mosaic_Cache_Monitor.get_current_way_start_pos(LPM_L1);
                current_way_end_pos = Mosaic_Cache_Monitor.get_current_way_end_pos(LPM_L1);
                break;
            case IS_L2C:
                current_way_start_pos = Mosaic_Cache_Monitor.get_current_way_start_pos(LPM_L2);
                current_way_end_pos = Mosaic_Cache_Monitor.get_current_way_end_pos(LPM_L2);
                break;
            case IS_LLC:
                current_way_start_pos = Mosaic_Cache_Monitor.get_current_way_start_pos(LPM_L3);
                current_way_end_pos = Mosaic_Cache_Monitor.get_current_way_end_pos(LPM_L3);
                break;
            default:
                current_way_start_pos = 0;
                current_way_end_pos = NUM_WAY;
                break;
        }
    }

    //uint32_t way = 0;
    uint32_t way = current_way_start_pos;

    // fill invalid line first
    // for (way=0; way<NUM_WAY; way++) 
    for (way=current_way_start_pos; way<current_way_end_pos; way++)
    {
        if (block[set][way].valid == false) 
        {

            DP ( if (warmup_complete[cpu]) {
            cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " invalid set: " << set << " way: " << way;
            cout << hex << " address: " << (full_addr>>LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
            cout << dec << " lru: " << block[set][way].lru << endl; });

            break;
        }
    }

    // LRU victim
    //if (way == NUM_WAY)
    if (way == current_way_end_pos)
    {
        //for (way=0; way<NUM_WAY; way++)
        for (way=current_way_start_pos; way<current_way_end_pos; way++)
        {
            if (block[set][way].lru == NUM_WAY-1) 
            {

                DP ( if (warmup_complete[cpu]) {
                cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " replace set: " << set << " way: " << way;
                cout << hex << " address: " << (full_addr>>LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
                cout << dec << " lru: " << block[set][way].lru << endl; });

                break;
            }
        }
    }

    //if (way == NUM_WAY)
    if (way == current_way_end_pos)
    {
        cerr << "[" << NAME << "] " << __func__ << " no victim! set: " << set << endl;
        assert(0);
    }

    return way;
}

void CACHE::lru_update(uint32_t set, uint32_t way)
{
    // zmz modify (STEP 5)
    int current_way_start_pos=0, current_way_end_pos=NUM_WAY;
    if(Mosaic_Cache_Monitor.get_work_mode() != 0)
    {
        switch(cache_type)
        {
            case IS_L1D:
                current_way_start_pos = Mosaic_Cache_Monitor.get_current_way_start_pos(LPM_L1);
                current_way_end_pos = Mosaic_Cache_Monitor.get_current_way_end_pos(LPM_L1);
                break;
            case IS_L2C:
                current_way_start_pos = Mosaic_Cache_Monitor.get_current_way_start_pos(LPM_L2);
                current_way_end_pos = Mosaic_Cache_Monitor.get_current_way_end_pos(LPM_L2);
                break;
            case IS_LLC:
                current_way_start_pos = Mosaic_Cache_Monitor.get_current_way_start_pos(LPM_L3);
                current_way_end_pos = Mosaic_Cache_Monitor.get_current_way_end_pos(LPM_L3);
                break;
            default:
                current_way_start_pos = 0;
                current_way_end_pos = NUM_WAY;
                break;
        }
    }

    // zmz modify
    // update lru replacement state
    //for (uint32_t i=0; i<NUM_WAY; i++) 
    for (uint32_t i=current_way_start_pos; i<current_way_end_pos; i++)
    {
        if (block[set][i].lru < block[set][way].lru) 
        {
            block[set][i].lru++;
        }
    }
    block[set][way].lru = 0; // promote to the MRU position
}

void CACHE::replacement_final_stats()
{

}

#ifdef NO_CRC2_COMPILE
void InitReplacementState()
{
    
}

uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    return 0;
}

void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    
}

void PrintStats_Heartbeat()
{
    
}

void PrintStats()
{

}
#endif
