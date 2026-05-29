#include <fractal.h>
#include <io/pci.h>
#include <io/pci/msi.h>

void MSI_XCapability::enable() {
    volatile MSI_XCapability *cap = (volatile MSI_XCapability *)(this);
    cap->message_control |= MSI_X_CAP_MSG_CONTROL_ENABLED;
}

usize MSI_XCapability::table_offset() {
    volatile MSI_XCapability *cap = (volatile MSI_XCapability *)(this);
    return (cap->table_offset_and_bir & ~MSI_X_CAP_BAR_MASK);
}

usize MSI_XCapability::table_bar() {
    volatile MSI_XCapability *cap = (volatile MSI_XCapability *)(this);
    return (cap->table_offset_and_bir & MSI_X_CAP_BAR_MASK);
}

usize MSI_XCapability::num_entries() {
    volatile MSI_XCapability *cap = (volatile MSI_XCapability *)(this);
    return (cap->message_control & MSI_X_CAP_MSG_CONTROL_TABLE_SIZE_MASK);
}
