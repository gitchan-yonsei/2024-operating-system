#ifndef VM_H
#define VM_H

#include "types.h"
#include "mmu.h"

pde_t* walkpgdir(pde_t *pgdir, const void *va, int alloc);
int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);

#endif
