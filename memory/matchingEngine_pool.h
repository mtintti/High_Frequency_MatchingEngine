#pragma once
#include <atomic>
#include <new>
#include "../matchingEngine/matchingEngine.h"
#include "mempool.h"

using matchPool = MemoryPool<sizeof(OrdertoMatch), OrdertoMatch>;
extern std::atomic<size_t> matchpool_blocks_in_use;

OrdertoMatch *allocateOrder(const OrdertoMatch &order);
void deallocateOrder(OrdertoMatch *order);