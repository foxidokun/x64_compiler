#ifndef X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
#define X64_TRANSLATOR_ADDRESS_TRANSLATOR_H

//----------------------------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "vector.h"

//----------------------------------------------------------------------------------------------------------------------

enum class mapping_type_t {
    BASIC,
    CALL
};

struct mapping_t {
    mapping_type_t type;

    uint64_t old_addr;
    uint64_t new_addr;
};

struct addr_transl_t {
    mapping_t *mappings;
    size_t size;
    size_t capacity;

    bool old_addr_registered;
    uint64_t old_addr;
};

//----------------------------------------------------------------------------------------------------------------------

addr_transl_t *addr_transl_new();
void addr_transl_delete(addr_transl_t *self);

void addr_transl_insert(addr_transl_t* self, uint64_t old_addr, uint64_t new_addr);

result_t addr_transl_remember_old_addr(addr_transl_t* self, uint64_t old_addr);
result_t addr_transl_insert_with_new_addr(addr_transl_t* self, uint64_t new_addr);

uint64_t addr_transl_translate(addr_transl_t* self, uint64_t old_addr);

#endif //X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
