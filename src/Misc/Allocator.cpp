#include <cstddef>
#include <cstdlib>
#include <utility>
#include <cstdio>
#include "../../tlsf/tlsf.h"
#include "Allocator.h"

Allocator::Allocator(void)
{
    void *buf = malloc(1024*1024);
    impl = tlsf_create_with_pool(buf, 1024*1024);
}

void *Allocator::alloc_mem(size_t mem_size)
{
    void *mem = tlsf_malloc(impl, mem_size);
    //fprintf(stderr, "Allocating memory (%p)\n", mem);
    return mem;
    //return malloc(mem_size);
}
void Allocator::dealloc_mem(void *memory)
{
    //fprintf(stderr, "Freeing memory (%p)\n", memory);
    tlsf_free(impl, memory);
    //free(memory);
}
