//
// Created by roman on 07.07.17.
//

#include <cstdio>
#include <sys/mman.h>
#include "allocator.h"

const size_t PAGE_SZ = 1 << 12;
const size_t TRAMPOLINE_SZ = 1 << 8;
void *allocator::head = nullptr;

void *allocator::alloc() {
    if (!allocator::head) {
        allocator::head = page_alloc();
        if (!allocator::head)
            return nullptr;
    }
    void *ret = allocator::head;
    allocator::head = *reinterpret_cast<void **>(allocator::head);
    return ret;
}

void allocator::free(void *ptr) {
    *reinterpret_cast<void **>(ptr) = allocator::head;
    allocator::head = ptr;
}

void *allocator::page_alloc() {
    void *page = mmap(nullptr, PAGE_SZ, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char *char_page = reinterpret_cast<char *>(page);
    for (size_t i = 0; i < PAGE_SZ; i += TRAMPOLINE_SZ) {
        if (i + TRAMPOLINE_SZ < PAGE_SZ)
            *reinterpret_cast<void **>(char_page + i) = char_page + i + TRAMPOLINE_SZ;
        else
            *reinterpret_cast<void **>(char_page + i) = nullptr;
    }
    return page;
}
