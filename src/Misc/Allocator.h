#include <cstdlib>

class Allocator
{
    public:
        Allocator(void);
        virtual void *alloc_mem(size_t mem_size);
        virtual void dealloc_mem(void *memory);

        template <typename T, typename... Ts>
        T *alloc(Ts... ts)
        {
            void *data = alloc_mem(sizeof(T));
            if(!data)
                return nullptr;
            return new (data) T(ts...);
        }

        template <typename T, typename... Ts>
        T *valloc(size_t len, Ts... ts)
        {
            T *data = (T*)alloc_mem(len*sizeof(T));
            if(!data)
                return nullptr;
            for(unsigned i=0; i<len; ++i)
                new ((void*)&data[i]) T(ts...);

            return data;
        }

        template <typename T>
        void dealloc(T*&t)
        {
            if(t) {
                t->~T();
                dealloc_mem((void*)t);
                t = nullptr;
            }
        }

        //Destructor Free Version
        template <typename T>
        void devalloc(T*&t)
        {
            if(t) {
                dealloc_mem(t);
                t = nullptr;
            }
        }

        template <typename T>
        void devalloc(size_t elms, T*&t)
        {
            if(t) {
                for(size_t i=0; i<elms; ++i)
                    (t+i)->~T();

                dealloc_mem(t);
                t = nullptr;
            }
        }

    void addMemory(void *, size_t mem_size);//{(void)mem_size;};

    //Return true if the current pool cannot allocate n chunks of chunk_size
    bool lowMemory(unsigned n, size_t chunk_size);//{(void)n;(void)chunk_size; return false;};

    void *impl;
};

//Memory that could either be from the heap or from the realtime allocator
//This should be avoided when possible, but it's not clear if it can be avoided
//in all cases
template<class T>
class HeapRtMem
{
};


//A helper class used to perform a series of allocations speculatively to verify
//that there is enough memory for a set transation to occur.
//Stuff will get weird if this ends up failing, but it will be able to at least
//detect where there is an issue
class StaticAllocFreeVerifyier
{
    void *scratch_buf[4096];
    unsigned alloc_count;
};


/**
 * General notes on Memory Allocation Within ZynAddSubFX
 * -----------------------------------------------------
 *
 *  - Parameter Objects Are never allocated within the realtime thread
 *  - Effects, notes and note subcomponents must be allocated with an allocator
 *  - 5M Chunks are used to give the allocator the memory it wants
 *  - If there are 3 chunks that are unused then 1 will be deallocated
 *  - The system will request more allocated space if 5x 1MB chunks cannot be
 *    allocated at any given time (this is likely huge overkill, but if this is
 *    satisfied, then a lot of note spamming would be needed to run out of
 *    space)
 *
 *   - Things will get a bit weird around the effects due to how pointer swaps
 *     occur
 *     * When a new Part instance is provided it may or may not come with some
 *       instrument effects
 *     * Merging blocks is an option, but one that is not going to likely be
 *       implmented too soon, thus all effects need to be reallocated when the
 *       pointer swap occurs
 *     * The old effect is extracted from the manager
 *     * A new one is constructed with a deep copy
 *     * The old one is returned to middleware for deallocation
 */
