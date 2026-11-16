#include <iostream>
#include "../profile.h"
#include "matchingEngine_pool.h"


using matchPool = MemoryPool<sizeof(OrdertoMatch), OrdertoMatch>;
std::atomic<size_t> matchpool_blocks_in_use;

OrdertoMatch *allocateOrder(const OrdertoMatch &order)
{
    //ScopedTimer t(HFTProfiler::MATCHPOOL_ALLOC);
    void *mpool = matchPool::instance().allocate();
    
    if (!mpool)
        return nullptr;
    HFTProfiler::instance().recordPoolState(
            ++matchpool_blocks_in_use,
            2000000, HFTProfiler::MATCHPOOL_ALLOC);
    return new (mpool) OrdertoMatch(order);
}

void deallocateOrder(OrdertoMatch *order)
{
    //ScopedTimer t(HFTProfiler::MATCHPOOL_DEALLOC);
    order->~OrdertoMatch();
    matchPool::instance().deallocate(order);
    --matchpool_blocks_in_use;
}