
#if defined(MEM_POOL_USE_MALLOC)
#include <stdlib.h>
#else
#include <new>
#endif

#include "mempool.h"
#include <iostream>

/* Defines macros to abstract system memory routines */
# ifdef MEM_POOL_USE_MALLOC
#   define MEM_POOL_ALLOCATE(Sz)    malloc(Sz)
#   define MEM_POOL_DEALLOCATE(Ptr) free(Ptr)
# else
#   define MEM_POOL_ALLOCATE(Sz)    ::operator new((Sz), std::nothrow)
#   define MEM_POOL_DEALLOCATE(Ptr) ::operator delete(Ptr)
# endif

int allocatetotalnumcall = 0;
void* MemoryPoolBase::allocateSystemSize(size_t uSize)
{
    void* pvResult = MEM_POOL_ALLOCATE(uSize);
    if (!pvResult)
    {
        MemoryPoolSet::recycleMemoryPools();
        pvResult = MEM_POOL_ALLOCATE(uSize);
        std::cout<< "\n allocating memory size of.. "<< uSize;
    }
    allocatetotalnumcall++;
    std::cout<< "\n memorypool created of, "<< pvResult << ". num of call: " << allocatetotalnumcall;
    return pvResult;
}

void MemoryPoolBase::deallocateSystemSize(void* pvReturn)
{
    MEM_POOL_DEALLOCATE(pvReturn);
}

MemoryPoolSet::MemoryPoolSet()
{
}


MemoryPoolSet::~MemoryPoolSet()
{
    while (!m_oMemoryPoolSet.empty())
    {
        // The destructor of a MemoryPool will remove itself from
        // the MemoryPoolSet.
        delete *m_oMemoryPoolSet.begin();
    }
}

MemoryPoolSet& MemoryPoolSet::instance()
{
    static MemoryPoolSet oInstance;
    return oInstance;
}

void MemoryPoolSet::recycleMemoryPools()
{
    instance().recycle();
}

void MemoryPoolSet::recycle()
{
    std::set<MemoryPoolBase*>::iterator end = m_oMemoryPoolSet.end();
    for (std::set<MemoryPoolBase*>::iterator
            i  = m_oMemoryPoolSet.begin();
            i != end; ++i)
    {
        (*i)->recycle();
    }
}
