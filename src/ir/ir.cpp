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
    //FIXME
//    if (self->instructions) {
//        instruction_t *next = self->instructions;
//        instruction_t *current = self->instructions;
//
//        while (current) {
//            next = current->next;
//            free(current);
//            current = next;
//        }
//    }

    free(self);
}

//----------------------------------------------------------------------------------------------------------------------


// TODO Refactor к нормальному блять виду
void ir::code_insert(code_t *self, instruction_t *instruction) {
    assert(self);
    assert(instruction);

    if (self->size == self->capacity) {
        self->instructions = (instruction_t *) realloc(self->instructions, (2*self->capacity+1) * sizeof(instruction_t));
        self->capacity = 2*self->capacity+1;

        for (uint i = 0; i+1 < self->size; ++i) {
            self->instructions[i].next = self->instructions + i + 1;
        }
        self->instructions[self->size].next = nullptr;
    }

    memcpy(self->instructions + self->size, instruction, sizeof(instruction_t));
    self->instructions[self->size].index = self->size;

    if (self->size > 0) {
        self->instructions[self->size - 1].next = self->instructions + self->size;
        self->last_instruction = self->instructions + self->size;
    } else {
        self->last_instruction = self->instructions + self->size;
        self->last_instruction->next = nullptr;
    }

    self->size++;
}

//----------------------------------------------------------------------------------------------------------------------

ir::instruction_t *ir::next_insruction(instruction_t *self) {
    assert(0 && "Not implemented");
}
