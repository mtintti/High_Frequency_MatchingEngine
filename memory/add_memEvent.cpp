#include <iostream>
#include "mempool.h"
#include "../profile.h"
#include "../orderbook/orderbook.h"
#include "add_memEvent.h"

using depthPool = MemoryPool<sizeof(DepthUpdate)>;
using BuffersPool = MemoryPool<50>;
std::atomic<size_t> pool_blocks_in_use;

void memoryEvent(int64_t price5, int64_t amount5, bool bidExist, int64_t firstID, int64_t lastID)
{
    int totalcount = 0;

    void *mpool;
    {
        ScopedTimer t(HFTProfiler::POOL_ALLOC);
        mpool = depthPool::instance().allocate();
        HFTProfiler::instance().recordPoolState(
            ++pool_blocks_in_use,
            50, HFTProfiler::POOL_ALLOC);
    }
    DepthUpdate *dUp = new (mpool) DepthUpdate{};
    if (bidExist)
    {
        dUp->price = price5;
        dUp->isitBid = true;
    }
    else
    {
        dUp->price = price5;
        dUp->isitBid = false;
    }
    dUp->quantity = amount5;
    dUp->U_first = firstID;
    dUp->id_sequence = lastID;
    totalcount++;

    {
        ScopedTimer t(HFTProfiler::ORDERBOOK_UPDATE);
        
        if(obook.pausing_dUp != true){
            obook.updateDepthBased(dUp);
        }
        
    }


    {
        ScopedTimer t(HFTProfiler::POOL_DEALLOC);
        dUp->~DepthUpdate();
        depthPool::instance().deallocate(mpool);
        --pool_blocks_in_use;
    }
    if (totalcount > 700)
    {
        /*obook.asksSize();
        obook.bidsSize();
        obook.printTop(5);
        std::cout << "\n sync (1) or not (0)? " << obook.isin_Sync;
        std::cout << "\n websock priceOf: " << priceOf << " qAm "<< quantityAmount;
        std::cout << "\n after conversion ";
        std::cout << price5 << " " << amount5;*/
        totalcount = 0;
    }
}