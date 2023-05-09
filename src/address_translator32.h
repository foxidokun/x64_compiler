#ifndef X64_TRANSLATOR_ADDRESS_TRANSLATOR32_H
#define X64_TRANSLATOR_ADDRESS_TRANSLATOR32_H

//----------------------------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include "vector.h"

//----------------------------------------------------------------------------------------------------------------------

enum class mapping_t32ype_t {
    KNOWN,
    UNRESOLVED
};

struct mapping_t32 {
    mapping_t32ype_t type;

    uint32_t old_addr;
    union {
        uint32_t new_addr;
        sortvec_t new_addr_vec;
    };
};

struct addr_transl32_t {
    sortvec_t mappings;
};

//----------------------------------------------------------------------------------------------------------------------

addr_transl32_t *addr_transl32_new();
void addr_transl_delete(addr_transl32_t *self);

void addr_transl_insert(addr_transl32_t* self, uint32_t old_addr, uint32_t new_addr);
void addr_transl32_translate(addr_transl32_t* self, uint32_t old_addr, uint32_t *new_addr_ptr);

#endif //X64_TRANSLATOR_ADDRESS_TRANSLATOR_H
