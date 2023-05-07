#include <assert.h>
#include <string.h>
#include "common.h"
#include "ir.h"

//----------------------------------------------------------------------------------------------------------------------

namespace ir {
    static result_t code_resize(code_t *self);
}

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

ir::code_t *ir::code_new() {
    code_t *self = (code_t *) calloc(1, sizeof(code_t));
    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void ir::code_delete(code_t *self) {
    free(self->instructions);
    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

void ir::code_insert(code_t *self, instruction_t *instruction) {
    assert(self);
    assert(instruction);

    if (self->size == self->capacity) {
        code_resize(self);
    }

    memcpy(self->instructions + self->size, instruction, sizeof (instruction_t));
    self->size++;
}

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

static result_t ir::code_resize(code_t *self) {
    assert(self);

    size_t new_capacity = (self->capacity) ? 2*self->capacity : 1;

    instruction_t *tmp = (instruction_t *) realloc(self->instructions, new_capacity * sizeof (instruction_t));
    if (!tmp) { return result_t::ERROR; }

    self->instructions = tmp;
    self->capacity     = new_capacity;
    return result_t::OK;
}