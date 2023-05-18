#ifndef X64_TRANSLATOR_X64_STDLIB_H
#define X64_TRANSLATOR_X64_STDLIB_H

#include <stdint.h>

namespace x64 {
    void     stdlib_out (int64_t arg);
    int64_t stdlib_inp ();
    uint64_t stdlib_sqrt(uint64_t arg);
    [[noreturn]] void stdlib_halt();
}

#endif //X64_TRANSLATOR_X64_STDLIB_H
