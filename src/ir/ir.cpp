#include <assert.h>
#include <string.h>
#include "../common.h"
#include "ir.h"

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

ir::code_t *ir::code_new() {
    code_t *self = (code_t *) calloc(1, sizeof(code_t));
    return self;
}

//----------------------------------------------------------------------------------------------------------------------

void ir::code_delete(code_t *self) {
    if (self->instructions) {
        instruction_t *next = self->instructions;
        instruction_t *current = self->instructions;

        while (current) {
            next = current->next;
            free(current);
            current = next;
        }
    }

    free(self);
}

//----------------------------------------------------------------------------------------------------------------------

void ir::code_insert(code_t *self, instruction_t *instruction) {
    assert(self);
    assert(instruction);

    instruction_t *new_instruction = (instruction_t *) calloc(1, sizeof(instruction_t));
    memcpy(new_instruction, instruction, sizeof(instruction_t));

    new_instruction->next = nullptr;

    if (self->size > 0) {
        new_instruction->index = self->last_instruction->index + 1;
        self->last_instruction->next = new_instruction;
        self->last_instruction = new_instruction;
    } else {
        self->instructions            = new_instruction;
        self->last_instruction        = new_instruction;
        self->last_instruction->index = 0;
    }

    self->size++;
}

//----------------------------------------------------------------------------------------------------------------------

ir::instruction_t *ir::next_insruction(instruction_t *self) {
    return self->next;
}
