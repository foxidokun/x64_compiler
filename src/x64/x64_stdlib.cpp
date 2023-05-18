#include <stdio.h>
#include <math.h>
#include "../common.h"
#include "x64_stdlib.h"

//----------------------------------------------------------------------------------------------------------------------

extern "C" int64_t input_asm();

int64_t x64::stdlib_inp() {
    return input_asm();
}

//----------------------------------------------------------------------------------------------------------------------

extern "C" void output_asm(int64_t);

void x64::stdlib_out(int64_t arg) {
    output_asm(arg);
}

//----------------------------------------------------------------------------------------------------------------------

extern "C" uint64_t sqrt_asm(uint64_t sqrt);

uint64_t x64::stdlib_sqrt(uint64_t arg) {
    return sqrt_asm(arg);
}

//----------------------------------------------------------------------------------------------------------------------

extern "C" [[noreturn]] void exit_asm();

[[noreturn]]
void x64::stdlib_halt() {
    exit_asm();
}