#ifndef X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
#define X64_TRANSLATOR_ADDRESS_TRANSLATOR_H

//----------------------------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include "../common.h"

//----------------------------------------------------------------------------------------------------------------------

struct mapping_t {
    uint64_t old_addr;
    uint64_t new_addr;
};

struct addr_transl_t {
    mapping_t *mappings;
    size_t size;
    size_t capacity;

    bool old_addr_is_stored;
    uint64_t stored_old_addr;
};

//----------------------------------------------------------------------------------------------------------------------

addr_transl_t *addr_transl_new();
void addr_transl_delete(addr_transl_t *self);

result_t addr_transl_insert(addr_transl_t* self, uint64_t old_addr, uint64_t new_addr);

result_t addr_transl_remember_old_addr(addr_transl_t* self, uint64_t old_addr);
result_t addr_transl_insert_if_remembered_addr(addr_transl_t* self, uint64_t new_addr);

uint64_t addr_transl_translate(addr_transl_t* self, uint64_t old_addr);

#endif //X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
