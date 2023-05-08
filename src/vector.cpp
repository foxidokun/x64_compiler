#include <string.h>
#include "vector.h"

//----------------------------------------------------------------------------------------------------------------------

const int START_CAPACITY = 1;

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

static void sortvect_resize(sortvec_t *self);

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

sortvec_t *sortvec_new(size_t elem_size, comp_f comparator) {
    sortvec_t *self  = (sortvec_t *) calloc(1, sizeof (sortvec_t));
    sortvec_ctor(self, elem_size, comparator);
    return self;
}

void sortvec_ctor(sortvec_t *self, size_t elem_size, comp_f comparator) {
    self->data       = calloc(START_CAPACITY, elem_size);
    self->capacity   = START_CAPACITY;
    self->elem_size  = elem_size;
    self->comparator = comparator;
}

//----------------------------------------------------------------------------------------------------------------------

void sortvec_dtor(sortvec_t *self) {
    free(self->data);
}

void sortvec_delete(sortvec_t *self) {
    sortvec_dtor(self);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

void sortvec_insert(sortvec_t *self, void *elem) {
    if (self->size == self->capacity) {
        sortvect_resize(self);
    }

    memcpy((char *)self->data + self->elem_size * self->size, elem, self->elem_size);
    self->size++;

    if (self->size >= 2) {
        void *prev_elem = (char *)self->data + self->elem_size * (self->size - 2);
        void *cur_elem  = (char *)prev_elem + self->elem_size;

        if (self->comparator(prev_elem, cur_elem) < 0) {
            qsort(self->data, self->size, self->elem_size, self->comparator);
        }
    }

}

//----------------------------------------------------------------------------------------------------------------------
// Private
//----------------------------------------------------------------------------------------------------------------------

static void sortvect_resize(sortvec_t *self) {
    self->capacity *= 2;
    self->data = realloc(self->data, self->capacity * self->elem_size);
}