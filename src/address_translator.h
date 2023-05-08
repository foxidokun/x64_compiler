#ifndef X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
#define X64_TRANSLATOR_ADDRESS_TRANSLATOR_H

//----------------------------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include "vector.h"

//----------------------------------------------------------------------------------------------------------------------

enum class mapping_type_t {
    KNOWN,
    UNRESOLVED
};

struct mapping_t {
    mapping_type_t type;

    uint64_t old_addr;
    union {
        uint64_t new_addr;
        sortvec_t new_addr_vec;
    };
};

struct addr_transl_t {
    sortvec_t mappings;
};

//----------------------------------------------------------------------------------------------------------------------

addr_transl_t *addr_transl_new();
void addr_transl_delete(addr_transl_t *self);

void addr_transl_insert(addr_transl_t* self, uint64_t old_addr, uint64_t new_addr);
void addr_transl_translate(addr_transl_t* self, uint64_t old_addr, uint64_t *new_addr_ptr);

#endif //X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
