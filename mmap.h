#ifndef MMAP_H
#define MMAP_H

#include "types.h"
#include "file.h"
#include "proc.h"

int munmap(void *addr, int length);

#endif
