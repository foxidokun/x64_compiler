#include <assert.h>
#include "../common.h"
#include "../log.h"
#include "address_translator.h"

//----------------------------------------------------------------------------------------------------------------------

const int START_CAPACITY = 16;

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

static result_t addr_transl_resize(addr_transl_t *self);

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------

addr_transl_t *addr_transl_new() {
    addr_transl_t *self = (addr_transl_t *) calloc(1, sizeof (addr_transl_t));
    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl_delete(addr_transl_t *self) {
    free(self->mappings);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

result_t addr_transl_insert(addr_transl_t* self, uint64_t old_addr, uint64_t new_addr) {
    if (self->size == self->capacity) {
        UNWRAP_ERROR(addr_transl_resize(self));
    }

    self->mappings[self->size].old_addr = old_addr;
    self->mappings[self->size].new_addr = new_addr;

    self->size++;

    return result_t::OK;
}

//----------------------------------------------------------------------------------------------------------------------

uint64_t addr_transl_translate(addr_transl_t* self, uint64_t old_addr) {
    for (size_t i = 0; i < self->size; ++i) {
        if (self->mappings[i].old_addr == old_addr) {
            return self->mappings[i].new_addr;
        }
    }

    log(ERROR, "Failed to translate addr %d", old_addr);
    return ERROR;
}

//----------------------------------------------------------------------------------------------------------------------

result_t addr_transl_remember_old_addr(addr_transl_t* self, uint64_t old_addr) {
    if (self->old_addr_is_stored) {
        log(ERROR, "Trying to remember old addr, but struct is already holding one...");
        return result_t::ERROR;
    }

    self->old_addr_is_stored = true;
    self->stored_old_addr = old_addr;
    return result_t::OK;
}

//----------------------------------------------------------------------------------------------------------------------

result_t addr_transl_insert_with_remembered_addr(addr_transl_t* self, uint64_t new_addr) {
    if (!self->old_addr_is_stored) {
        log(ERROR, "Trying to insert with remembered addr, but no addr is remembered :(");
        return result_t::ERROR;
    }

    UNWRAP_ERROR (addr_transl_insert(self, self->stored_old_addr, new_addr) )

    self->old_addr_is_stored = false;
    return result_t::OK;
}

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static result_t addr_transl_resize(addr_transl_t *self) {
    size_t new_capacity = (self->capacity) ? 2*self->capacity : START_CAPACITY;
    mapping_t *tmp_buf = (mapping_t *) realloc(self->mappings, self->capacity * sizeof (mapping_t));

    if (tmp_buf) {
        self->capacity = new_capacity;
        self->mappings = tmp_buf;
        return result_t::OK;
    } else {
        log(ERROR, "Failed to resize array from %d to %d elements", self->capacity, new_capacity);
        return result_t::ERROR;
    }
}