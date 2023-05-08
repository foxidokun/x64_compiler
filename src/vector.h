#ifndef X64_TRANSLATOR_VECTOR_H
#define X64_TRANSLATOR_VECTOR_H

//----------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"

//----------------------------------------------------------------------------------------------------------------------
typedef int (*comp_f)(const void*, const void*);

struct sortvec_t {
    void *data;
    size_t elem_size;
    size_t size;
    size_t capacity;
    comp_f comparator;
};

//----------------------------------------------------------------------------------------------------------------------

sortvec_t *sortvec_new(size_t elem_size, comp_f comparator);
void sortvec_ctor(sortvec_t *self, size_t elem_size, comp_f comparator);

void sortvec_insert(sortvec_t *self, void *elem);

void sortvec_dtor(sortvec_t *self);
void sortvec_delete(sortvec_t *self);

#endif //X64_TRANSLATOR_VECTOR_H
