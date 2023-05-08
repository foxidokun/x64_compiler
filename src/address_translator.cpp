#include <assert.h>
#include "log.h"
#include "address_translator.h"

const uint64_t DEBUG_UNRESOLVED_VALUE = (uint64_t) -1;

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

static int  mapping_comp(const void *lhs_void, const void *rhs_void);
static int  addr_comp   (const void *lhs_void, const void *rhs_void);

static void update_addrs(mapping_t *mapping, uint64_t new_addr);


//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------

addr_transl_t *addr_transl_new() {
    addr_transl_t *self = (addr_transl_t *) calloc(1, sizeof(addr_transl_t));
    sortvec_ctor(&self->mappings, sizeof(mapping_t), mapping_comp);
    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl_delete(addr_transl_t *self) {
    mapping_t *mappings = (mapping_t *) self->mappings.data;

    for (size_t i = 0; i < self->mappings.size; ++i) {
        if (mappings[i].type == mapping_type_t::UNRESOLVED) {
            assert(0 && "Unresolved mapping in the end");
        }
    }

    sortvec_dtor(&self->mappings);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl_insert(addr_transl_t* self, uint64_t old_addr, uint64_t new_addr) {
    log(INFO, "Saving %lx -> %lx", old_addr, new_addr);

    mapping_t map = {
            .type = mapping_type_t::KNOWN,
            .old_addr = old_addr,
            .new_addr = new_addr
    };

    mapping_t *existing_map = (mapping_t *) bsearch(&map, self->mappings.data, self->mappings.size, sizeof(mapping_t), mapping_comp);

    if (existing_map) {
        assert(existing_map->old_addr == old_addr);

        update_addrs(existing_map, new_addr);
        sortvec_dtor(&existing_map->new_addr_vec);

        existing_map->type = mapping_type_t::KNOWN,
        existing_map->new_addr = new_addr;
    } else {
        sortvec_insert(&self->mappings, &map);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl_translate(addr_transl_t* self, uint64_t old_addr, uint64_t *new_addr_ptr) {
    mapping_t map = {
            .type = mapping_type_t::UNRESOLVED,
            .old_addr = old_addr,
            .new_addr = 0
    };

    log(INFO, "Translating old addr = %d", old_addr);

    mapping_t *existing_map = (mapping_t *) bsearch(&map, self->mappings.data,  self->mappings.size, sizeof(mapping_t), mapping_comp);

    if (existing_map) {
        if (existing_map->type == mapping_type_t::KNOWN) {
            log(INFO, "\tSuccessfully resolved to %d", existing_map->new_addr);
            *new_addr_ptr = existing_map->new_addr;
        } else {
            log(INFO, "\tAdding to existed queue of unresolved");
            sortvec_insert(&existing_map->new_addr_vec, &new_addr_ptr);
        }
    } else {
        log(INFO, "\tCreated new queue of unresolved");
        sortvec_ctor(&map.new_addr_vec, sizeof(uint64_t *), addr_comp);
        sortvec_insert(&map.new_addr_vec, &new_addr_ptr);

        sortvec_insert(&self->mappings, &map);
        *new_addr_ptr = (uint64_t) DEBUG_UNRESOLVED_VALUE;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static int mapping_comp(const void *lhs_void, const void *rhs_void) {
    const mapping_t *lhs = (const mapping_t *) lhs_void;
    const mapping_t *rhs = (const mapping_t *) rhs_void;

    if (lhs->old_addr < rhs->old_addr) {
        return -1;
    } else if (lhs->old_addr == rhs->old_addr) {
        return 0;
    } else {
        return 1;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static int  addr_comp(const void *lhs_void, const void *rhs_void) {
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void update_addrs(mapping_t *mapping, uint64_t new_addr) {
    uint64_t **addresses = (uint64_t **) mapping->new_addr_vec.data;

    log (INFO, "Updating existing addresses...")

    for (size_t i = 0; i < mapping->new_addr_vec.size; ++i) {
        log(INFO, "\tUpdated");
        *(addresses[i]) = new_addr;
    }
}
