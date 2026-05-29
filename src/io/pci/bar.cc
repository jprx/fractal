#include <io/pci/bar.h>
#include <out.h>

#define BAR_SET_ALL_ONE ((0xFFFFFFFF))
#define BAR_TYPE_MASK ((0x01))

#define BAR_ADDRESS_MASK_FOR_PORT 0x03
#define BAR_ADDRESS_MASK_FOR_MMIO 0x0F

phys_t bar_get_addr(volatile bar_t *bar) {
    if (!bar) panic("Tried to read address of NULL BAR");
    u32 bar_contents = *(volatile bar_t*)bar;
    switch(bar_get_type(bar)) {
        case BAR_PORT:
        return bar_contents & ~BAR_ADDRESS_MASK_FOR_PORT;
        break;
        case BAR_MMIO:
        return bar_contents & ~BAR_ADDRESS_MASK_FOR_MMIO;
        break;
    }
    panic("Invalid BAR type\r\n");
}

bar_type_t bar_get_type(volatile bar_t *bar) {
    if (!bar) panic("Tried to read type of NULL BAR");
    u32 bar_contents = *(volatile bar_t*)bar;
    return (bar_type_t)(bar_contents & BAR_TYPE_MASK);
}


void bar_set_base32(volatile bar_t *bar, u32 base) {
    if (!bar) panic("Tried to set the base32 of NULL BAR");
    *(volatile bar_t*)bar = base;
}

// Returns the size of this BAR
// Side effects: temporarily changes the BAR's value to get the size
// You should make sure no other devices try to read
// from the BAR while a get_size call is running!
usize bar_get_size(volatile bar_t *bar) {
    if (!bar) return 0;

    // Don't need to save lower 4 bits, PCI will reset them correctly
    phys_t orig_paddr = bar_get_addr(bar);

    // Set all bits to 1
    // PCI will not allow log2(size) bits to change.
    bar_set_base32(bar,BAR_SET_ALL_ONE);

    // Read back the value with the flag bits masked out
    // Example: Size Val is 0xFFFFC000 => Size is 0x4000
    u32 size_val = bar_get_addr(bar);
    usize size = ~size_val + 1;

    // Reset BAR base address
    bar_set_base32(bar,orig_paddr);
    return size;
}