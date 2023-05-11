#include <stdio.h>
#include <math.h>
#include "../common.h"
#include "x64_stdlib.h"

//----------------------------------------------------------------------------------------------------------------------

uint64_t x64::stdlib_inp() {
    printf("INPUT: ");

    const int SAFE_ALIGNMENT = 32; // scanf requires alignment to 16 bytes, but out program can't provide it
    alignas(SAFE_ALIGNMENT) uint64_t res = 0;

    scanf("%ld", &res);
    printf("\n");

    return res * FIXED_PRECISION_MULTIPLIER;
}

//----------------------------------------------------------------------------------------------------------------------

void x64::stdlib_out(uint64_t arg) {
    printf("OUTPUT: %ld.%02ld\n", arg/FIXED_PRECISION_MULTIPLIER, arg%FIXED_PRECISION_MULTIPLIER);
}

//----------------------------------------------------------------------------------------------------------------------

uint64_t x64::stdlib_sqrt(uint64_t arg) {
    double d_arg = (double) arg *  FIXED_PRECISION_MULTIPLIER;
    return (uint64_t) sqrt(d_arg);
}

//----------------------------------------------------------------------------------------------------------------------

[[noreturn]]
void x64::stdlib_halt() {
    abort();
}