#include <iostream>
#include "mempool.h"
#include "../profile.h"
#include "../matchingEngine/matchingEngine.h"


using matchPool = MemoryPool<sizeof(OrdertoMatch), OrdertoMatch>;
std::atomic<size_t> matchpool_blocks_in_use;

template <size_t Sz, typename PoolName_toUse>
OrdertoMatch *allocateOrder(const OrdertoMatch &order)
{
    ScopedTimer t(HFTProfiler::MATCHPOOL_ALLOC);
    void *mpool = matchPool::instance().allocate();
    
    if (!mpool)
        return nullptr;
    HFTProfiler::instance().recordPoolState(
            ++matchpool_blocks_in_use,
            50, HFTProfiler::MATCHPOOL_ALLOC);
    return new (mpool) OrdertoMatch(order);
}

void deallocateOrder(OrdertoMatch *order)
{
    ScopedTimer t(HFTProfiler::MATCHPOOL_DEALLOC);
    order->~OrdertoMatch();
    matchPool::instance().deallocate(order);
    --matchpool_blocks_in_use;
}