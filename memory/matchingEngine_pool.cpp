#include <iostream>
#include "mempool.h"
#include "../profile.h"
#include "../matchingEngine/matchingEngine.h"


using matchPool = MemoryPool<sizeof(OrdertoMatch), OrdertoMatch>;
std::atomic<size_t> matchpool_blocks_in_use;


OrdertoMatch *allocateOrder(const OrdertoMatch &order)
{
    void *mpool = matchPool::instance().allocate();
    if (!mpool)
        return nullptr;
    ++matchpool_blocks_in_use;
    return new (mpool) OrdertoMatch(order);
}

void deallocateOrder(OrdertoMatch *order)
{
    order->~OrdertoMatch();
    matchPool::instance().deallocate(order);
    --matchpool_blocks_in_use;
}