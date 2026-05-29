#ifndef PCI_BAR_H
#define PCI_BAR_H

#include <fractal.h>
#include <io/pci.h>

BEGIN_C_HEADER

phys_t bar_get_addr(volatile bar_t *bar);
bar_type_t bar_get_type(volatile bar_t *bar);
void bar_set_base32(volatile bar_t *bar, u32 base);
usize bar_get_size(volatile bar_t *bar);

END_C_HEADER

#endif // PCI_BAR_H
