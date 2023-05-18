#ifndef X64_TRANSLATOR_COMMON_H
#define X64_TRANSLATOR_COMMON_H

#include "lib/log.h"

enum class result_t {
    ERROR = -1,
    OK    = 0
};

#define UNWRAP_ERROR(code) { if ((code) == result_t::ERROR) { return result_t::ERROR; } }
#define UNWRAP_NULLPTR(ptr) { if ((ptr) == nullptr) { return result_t::ERROR; } }

#define todo_assert(condition, message) {                               \
    if (condition) {                                                    \
        log(ERROR, message);                                            \
        log(ERROR, "TODO code executed, aborting...\n");                \
        abort();                                                        \
    }                                                                   \
}

const int FIXED_PRECISION_MULTIPLIER = 100;
const int ERROR = -1;


#endif //X64_TRANSLATOR_COMMON_H
