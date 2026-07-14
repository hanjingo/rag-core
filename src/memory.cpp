#include "memory.h"

void memory_mgr::distribution(std::vector<float> &dst, std::mt19937 &rng)
{
    static std::uniform_real_distribution<float> dist_inst(0.0f, 1.0f);
    for(auto &val : dst)
        val = dist_inst(rng);
}