#include <fractal.h>
#include <filesys/tar.h>

extern virt_t global_initrd;

// Convert octal ASCII string -> its value
usize read_tar_size(char *s) {
    usize accumulator = 0;
    usize sz = TAR_SIZE_STRLEN;
    char *cursor = s;
    while (sz > 0) {
        accumulator <<= 3;
        accumulator += *cursor - '0';
        cursor++;
        sz--;
    }
    return accumulator;
}

TARIterator TARIterator::operator++() {
    usize flen = read_tar_size(current->size);
    // The number of blocks is the tar size + sector size - 1 divided by sector size
    usize num_blocks = (flen + TAR_SECTOR_LEN - 1) / TAR_SECTOR_LEN;
    current = (struct tar_posix_header *)(((usize)current) + ((num_blocks+1) * TAR_SECTOR_LEN));
    return *this;
}

tar_t *TARIterator::operator*() {
    if (strequal(current->magic,TMAGIC)) {
        return current;
    }
    return NULL;
}
