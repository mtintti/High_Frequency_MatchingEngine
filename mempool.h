
#include <set>
#include <stdexcept>
#include <string>
#include <stddef.h>
#include <assert.h>

class MemoryPoolBase
{
public:
    virtual ~MemoryPoolBase() {}; // had this, -> {};
    virtual void recycle() = 0;
    static void *allocateSystemSize(size_t uSize);
    static void deallocateSystemSize(void *pvReturn);
    struct BlockList
    {
        BlockList *m_pNext;
    };
};

struct DepthUpdate
{
    int64_t price; //was long double and then int64_t
    int64_t quantity; //was long double
    bool isitBid;
    uint64_t id_sequence;
};

class MemoryPoolSet
{
public:
    static MemoryPoolSet &instance();
    static void recycleMemoryPools();
    void add(MemoryPoolBase *pMemoryPool)
    {
        m_oMemoryPoolSet.insert(pMemoryPool);
    };
    void remove(MemoryPoolBase *pMemoryPool)
    {
        m_oMemoryPoolSet.erase(pMemoryPool);
    };
    //__PRIVATE:
    //    ~MemoryPoolSet();
private:
    MemoryPoolSet();
    ~MemoryPoolSet();
    void recycle();
    std::set<MemoryPoolBase *> m_oMemoryPoolSet;
    MemoryPoolSet(const MemoryPoolSet &);
    const MemoryPoolSet &operator=(const MemoryPoolSet &);
};

template <size_t Sz>
class MemoryPool : public MemoryPoolBase
{
public:
    static MemoryPool &instance()
    {
        if (!s_pInstance)
        {
            createInstance();
        }
        return *s_pInstance;
    }
    static MemoryPool &instanceKnown()
    {
        assert(s_pInstance != NULL);
        return *s_pInstance;
    }
    void *allocate()
    {
        void *pvResult;
        if (s_pMemoryPool)
        {
            pvResult = s_pMemoryPool;
            s_pMemoryPool = s_pMemoryPool->m_pNext;
        }
        else
        {
            pvResult = allocateSystemSize(align(Sz));
        }
        return pvResult;
    }
    void deallocate(void *pvReturn)
    {
        assert(pvReturn != NULL);
        BlockList *pBlockList = reinterpret_cast<BlockList *>(pvReturn);
        pBlockList->m_pNext = s_pMemoryPool;
        s_pMemoryPool = pBlockList;
    }
    virtual void recycle();

    static void prewarmAtStart(size_t count)
    {
        for (size_t i = 0; i < count; i++)
        {
            void *mem = allocateSystemSize(align(Sz));
            if (!mem)
                break;
            BlockList *block = reinterpret_cast<BlockList *>(mem);
            block->m_pNext = s_pMemoryPool;
            s_pMemoryPool = block;
        }
    }

private:
    MemoryPool()
    {
        MemoryPoolSet::instance().add(this);
    }
    ~MemoryPool()
    {
#ifdef _DEBUG
        // Empty the pool to avoid false memory leakage alarms.  This is
        // generally not necessary for release binaries.
        BlockList *pBlockList = s_pMemoryPool;
        while (pBlockList)
        {
            BlockList *pBlockNext = pBlockList->m_pNext;
            deallocateSystemSize(pBlockList);
            pBlockList = pBlockNext;
        }
        s_pMemoryPool = NULL;
#endif
        MemoryPoolSet::instance().remove(this);
        s_pInstance = NULL;
        s_fDestroyed = true;
    }
    static void onDeadReference()
    {
        throw std::runtime_error("dead reference detected");
    }
    static size_t align(size_t uSize)
    {
        return uSize >= sizeof(BlockList) ? uSize : sizeof(BlockList);
    }
    static void createInstance();

    static bool s_fDestroyed;
    static MemoryPool *s_pInstance;
    static BlockList *s_pMemoryPool;

    /* Forbid their use */
    MemoryPool(const MemoryPool &);
    const MemoryPool &operator=(const MemoryPool &);
};

template <size_t Sz>
bool
    MemoryPool<Sz>::s_fDestroyed = false;
template <size_t Sz>
MemoryPool<Sz> *
    MemoryPool<Sz>::s_pInstance = NULL;
template <size_t Sz>
MemoryPoolBase::BlockList *
    MemoryPool<Sz>::s_pMemoryPool = NULL;

template <size_t Sz>
void MemoryPool<Sz>::recycle()
{
    BlockList *pBlockList = s_pMemoryPool;
    while (pBlockList)
    {
        if (BlockList *pBlockTemp = pBlockList->m_pNext)
        {
            BlockList *pBlockNext = pBlockTemp->m_pNext;
            pBlockList->m_pNext = pBlockNext;
            // deallocSys(pBlockTemp);
            deallocateSystemSize(pBlockTemp);
            pBlockList = pBlockNext;
        }
        else
        {
            break;
        }
    }
}

template <size_t Sz>
void MemoryPool<Sz>::createInstance()
{
    if (s_fDestroyed)
    {
        onDeadReference();
    }
    else
    {
        if (!s_pInstance)
        {
            MemoryPoolSet::instance(); // Force its creation
            s_pInstance = new MemoryPool();
        }
    }
}

#undef __PRIVATE

/*
template<size_t Sz>
class MemoryPool {
    public:
    static void* allocaate();
    static void deallocate(void*);
};*/
