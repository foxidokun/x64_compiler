#ifndef HASHMAP_OPTIMISED_FILE_H
#define HASHMAP_OPTIMISED_FILE_H

#include "stdlib.h"
#include "stdio.h"

struct mmaped_file_t {
    unsigned char *data;
    size_t size;
};

mmaped_file_t mmap_file_or_warn(const char *name);
void mmap_close (mmaped_file_t file);
FILE *open_file_or_warn (const char *name, const char *modes);
uint count_lines(const char *data);
size_t get_file_size (int fd);

#endif //HASHMAP_OPTIMISED_FILE_H
