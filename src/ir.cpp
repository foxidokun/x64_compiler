#include <assert.h>
#include <string.h>
#include "common.h"
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

    instruction_t *new_instr_node = (instruction_t *) calloc(1, sizeof(instruction_t));
    memcpy(new_instr_node, instruction, sizeof (instruction_t));
    new_instr_node->next = nullptr;

    if (self->instructions) {
        new_instr_node->index        = self->last_instruction->index + 1;
        self->last_instruction->next = new_instr_node;
        self->last_instruction       = new_instr_node;
    } else {
        self->last_instruction = new_instr_node;
        self->instructions     = new_instr_node;
        new_instr_node->index = 0;
    }

    self->size++;
}

//----------------------------------------------------------------------------------------------------------------------

ir::instruction_t *ir::next_insruction(instruction_t *self) {
    return self->next;
}
