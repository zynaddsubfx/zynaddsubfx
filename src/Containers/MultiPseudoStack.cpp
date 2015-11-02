#include "MultiPseudoStack.h"
#include <cassert>
    
LockFreeStack::LockFreeStack(void)
{
    head.next = &head;
}
//Aquire node R    := head
//Update      head := next(R)
iql_t *LockFreeStack::read(void) {
retry:
    iql_t *R = head.next.load();
    if(R == &head)
        return 0;
    iql_t *N = R->next.load();

    //Stack H-R-N-...-H
    //when head.next is still the old next 'r'
    //set  head.next to r->next
    if(!head.next.compare_exchange_strong(R, N))
        goto retry;

    return R;
}

//Insert Node Q
void LockFreeStack::write(iql_t *Q) {
    if(!Q)
        return;
retry:
    iql_t *old_next = head.next.load();

    Q->next = old_next;
    if(!head.next.compare_exchange_strong(old_next, Q))
        goto retry;
}

MultiPseudoStack::MultiPseudoStack(void)
{
    //32 instances of 2kBi memory chunks
    for(int i=0; i<32; ++i) {
        iql_t *ptr  = new iql_t;
        ptr->size   = 2048;
        ptr->memory = new char[2048];
        free(ptr);
    }
}
