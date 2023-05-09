#include <assert.h>
#include "log.h"
#include "address_translator32.h"

const uint32_t DEBUG_UNRESOLVED_VALUE = (uint32_t) -1;

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

static int  mapping_comp(const void *lhs_void, const void *rhs_void);
static int  addr_comp   (const void *lhs_void, const void *rhs_void);

static void update_addrs(mapping_t32 *mapping, uint32_t new_addr);


//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------

addr_transl32_t *addr_transl32_new() {
    addr_transl32_t *self = (addr_transl32_t *) calloc(1, sizeof(addr_transl32_t));
    sortvec_ctor(&self->mappings, sizeof(mapping_t32), mapping_comp);
    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl_delete(addr_transl32_t *self) {
    mapping_t32 *mappings = (mapping_t32 *) self->mappings.data;

    for (size_t i = 0; i < self->mappings.size; ++i) {
        if (mappings[i].type == mapping_t32ype_t::UNRESOLVED) {
            assert(0 && "Unresolved mapping in the end");
        }
    }

    sortvec_dtor(&self->mappings);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl_insert(addr_transl32_t* self, uint32_t old_addr, uint32_t new_addr) {
    log(INFO, "Saving %lx -> %lx", old_addr, new_addr);

    mapping_t32 map = {
            .type = mapping_t32ype_t::KNOWN,
            .old_addr = old_addr,
            .new_addr = new_addr
    };

    mapping_t32 *existing_map = (mapping_t32 *) bsearch(&map, self->mappings.data, self->mappings.size, sizeof(mapping_t32), mapping_comp);

    if (existing_map) {
        assert(existing_map->old_addr == old_addr);

        update_addrs(existing_map, new_addr);
        sortvec_dtor(&existing_map->new_addr_vec);

        existing_map->type = mapping_t32ype_t::KNOWN,
        existing_map->new_addr = new_addr;
    } else {
        sortvec_insert(&self->mappings, &map);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void addr_transl32_translate(addr_transl32_t* self, uint32_t old_addr, uint32_t *new_addr_ptr) {
    mapping_t32 map = {
            .type = mapping_t32ype_t::UNRESOLVED,
            .old_addr = old_addr,
            .new_addr = 0
    };

    log(INFO, "Translating old addr = %d", old_addr);

    mapping_t32 *existing_map = (mapping_t32 *) bsearch(&map, self->mappings.data,  self->mappings.size, sizeof(mapping_t32), mapping_comp);

    if (existing_map) {
        if (existing_map->type == mapping_t32ype_t::KNOWN) {
            log(INFO, "\tSuccessfully resolved to %d", existing_map->new_addr);
            *new_addr_ptr += existing_map->new_addr;
        } else {
            log(INFO, "\tAdding to existed queue of unresolved");
            sortvec_insert(&existing_map->new_addr_vec, &new_addr_ptr);
        }
    } else {
        log(INFO, "\tCreated new queue of unresolved");
        sortvec_ctor(&map.new_addr_vec, sizeof(uint32_t *), addr_comp);
        sortvec_insert(&map.new_addr_vec, &new_addr_ptr);

        sortvec_insert(&self->mappings, &map);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static int mapping_comp(const void *lhs_void, const void *rhs_void) {
    const mapping_t32 *lhs = (const mapping_t32 *) lhs_void;
    const mapping_t32 *rhs = (const mapping_t32 *) rhs_void;

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

static void update_addrs(mapping_t32 *mapping, uint32_t new_addr) {
    uint32_t **addresses = (uint32_t **) mapping->new_addr_vec.data;

    log (INFO, "Updating existing addresses...")

    for (size_t i = 0; i < mapping->new_addr_vec.size; ++i) {
        log(INFO, "\tUpdated");
        *(addresses[i]) += new_addr;
    }
}
