#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "file.h"

// ---------------------------------------------------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------------------------------------------------

mmaped_file_t mmap_file_or_warn(const char *name) {
    int fd = open(name, O_RDWR);
    if (fd < 0) {
        fprintf (stderr, "Failed to open file '%s', reason: %s\n", name, strerror(errno));
        return {.data=nullptr};
    }

    size_t filesize = get_file_size(fd);

    unsigned char *mmap_memory = (unsigned char *) mmap(nullptr, filesize, PROT_READ | PROT_WRITE,
                                                        MAP_PRIVATE, fd, 0);

    if (mmap_memory == MAP_FAILED) {
        fprintf (stderr, "Failed to map memory on file '%s', reason: %s\n", name, strerror(errno));
        return {.data = nullptr};
    }

    return {.data=mmap_memory, .size=filesize};
}

//----------------------------------------------------------------------------------------------------------------------

void mmap_close (mmaped_file_t file) {
    munmap(file.data, file.size);
}

//----------------------------------------------------------------------------------------------------------------------

FILE *open_file_or_warn (const char *name, const char *modes) {
    FILE *file = fopen(name, modes);
    if (file) {
        return file;
    }

    fprintf (stderr, "Failed to open file '%s', reason: %s\n", name, strerror(errno));
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

size_t get_file_size (int fd)
{
    assert (fd > 0 && "Invalid file descr");

    struct stat st = {};
    fstat(fd, &st);
    return st.st_size;
}

//----------------------------------------------------------------------------------------------------------------------

uint count_lines(const char *data) {
    uint counter = 0;
    data--;

    while ((data = strchr(data+1, '\n'))) {
        counter++;
    }

    return counter;
}