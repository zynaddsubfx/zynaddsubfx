#pragma once
#include <atomic>

struct IntrusiveQueueList
{
    typedef IntrusiveQueueList iql_t;
    IntrusiveQueueList(void)
        :memory(0), next(0), size(0)
    {}

    char                 *memory;
    std::atomic<iql_t*>   next;
    uint32_t              size;
};


static_assert(sizeof(IntrusiveQueueList) <= 3*sizeof(void*),
        "Atomic Types Must Not Add Overhead To Intrusive Queue");

typedef IntrusiveQueueList iql_t;
//Many reader many writer
class LockFreeStack
{
    iql_t head;
    public:
    LockFreeStack(void);
    iql_t *read(void);
    void write(iql_t *Q);
};


/*
 * Many reader Many writer capiable queue
 * - lock free
 * - allocation free (post initialization)
 */
class MultiPseudoStack
{
    LockFreeStack m_free;
    LockFreeStack m_msgs;

    public:
    MultiPseudoStack(void);
    iql_t *alloc(void)   { return m_free.read();   }
    void free(iql_t *q)  {        m_free.write(q); } 
    void write(iql_t *q) {        m_msgs.write(q); }
    iql_t *read(void)    { return m_msgs.read();   }
};
