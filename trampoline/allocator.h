//
// Created by roman on 07.07.17.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H


class allocator {
    static void *head;
    static void *page_alloc();
public:
    static void *alloc();
    static void free(void *);
};


#endif //ALLOCATOR_H
