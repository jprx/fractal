#ifndef VIRTIO_HID_H
#define VIRTIO_HID_H

#include <fractal.h>
#include <io/virtio.h>
#include <io/virtio/virtqueue.h>
#include <io/virtio/virtio-pci.h>
#include <io/hid/decode-keys.h>
#include <io/hid/input.h>

BEGIN_C_HEADER

#define VIRTIO_HID_EVENTQ_IDX  0
#define VIRTIO_HID_STATUSQ_IDX 1

#define VIRTIO_HID_NAME_LEN 128

#define QEMU_VIRTIO_HID_KEYBOARD_NAME "QEMU Virtio Keyboard"
#define QEMU_VIRTIO_HID_MOUSE_NAME    "QEMU Virtio Mouse"
#define QEMU_VIRTIO_HID_TABLET_NAME "QEMU Virtio Tablet"

// ev->value during an EV_KEY event
// release means "key was let go", press means "key was just pressed"
#define INPUT_EVENT_VALUE_RELEASE  0
#define INPUT_EVENT_VALUE_PRESS    1

// My struct for representing what kind of device we have detected:
typedef enum {
    HID_UNKNOWN = 0,
    HID_KEYBOARD = 1,
    HID_MOUSE,
    HID_TABLET,
} hid_device_t;

enum virtio_input_config_select {
    VIRTIO_INPUT_CFG_UNSET      = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME    = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS  = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS    = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO   = 0x12,
};

struct virtio_input_absinfo {
    u32  min;
    u32  max;
    u32  fuzz;
    u32  flat;
    u32  res;
};

struct virtio_input_devids {
    u16  bustype;
    u16  vendor;
    u16  product;
    u16  version;
};

struct virtio_input_config {
    u8    select;
    u8    subsel;
    u8    size;
    u8    reserved[5];
    union {
        char string[VIRTIO_HID_NAME_LEN];
        u8   bitmap[VIRTIO_HID_NAME_LEN];
        struct virtio_input_absinfo abs;
        struct virtio_input_devids ids;
    } u;
};

struct virtio_input_event {
    u16 type;
    u16 code;
    u32 value;
};

// Packet processing methods- these translate Virtio events
// to Fractal device-independent keyboard/ mouse inputs
extern i32 mouse_x, mouse_y;
void decode_virtio_key_packet(struct virtio_input_event *ev, fractal_keyboard_state_t *keystate);
void decode_virtio_rel_packet(struct virtio_input_event *ev);
void decode_virtio_event(struct virtio_input_event *ev, fractal_keyboard_state_t *keystate);

PCIDevice *InitVirtioHID(PCIHeader_t, usize, usize);

class VirtioHID : public VirtioPCI {
public:
    // Points to the PCI common configuration structure for this HID device
    volatile virtio_pci_common_cfg *header;
    volatile u64 *notify;

    // A page we use to store events
    struct virtio_input_event *event_buffer;

    // What was the previous value of used_idx, before
    // the most recent event?
    // Current used index - prev_used_index will tell us
    // how many packets we have to process.
    usize prev_used_idx;

    // What kind of device is this?
    char device_name[VIRTIO_HID_NAME_LEN];
    hid_device_t device_type;

    // This is used eg. to set keyboard LEDs:
    // Us -> Device
    volatile Virtqueue *statusq;

    // This is how the device tells us something happened
    // Device -> Us
    volatile Virtqueue *eventq;

    // All state bits for this keyboard
    fractal_keyboard_state_t keyboard_state;

    /*
     * Negotiate
     * Reset and configure this device.
     */
    virtual kret_t Negotiate() override;

    /*
     * Setup
     * Now that the virtqueue has been created,
     * send initial packets to "turn on" the device.
     */
    virtual kret_t Setup() override;

    /*
     * DetermineKind
     * There are several possible kinds of input device.
     * This reads the device configuration region (pointed to by
     * the PCI Capability VIRTIO_PCI_CAP_DEVICE_CFG) to
     * learn the device's name, and sets internal flags accordingly.
     */
    void DetermineKind();

    /*
     * Poll
     * Test method- watch for new events.
     */
    void Poll();

    /*
     * ReceiveEvent
     * Parse any changes to the event queue made by the driver.
     * This calls decode_virtio_event for each input event in the list.
     */
    void ReceiveEvent();

    /*
     * VirtioHID
     * Creates a VirtioHID using a common PCI configuration header,
     * discovered by enumerating the PCI bus.
     *
     * This will allocate all required virtqueues but
     * not initialize the device just yet.
     */
    VirtioHID(volatile virtio_pci_common_cfg *header_in, volatile u64 *notify_in, void *dev_conf_in, void *isr_conf_in, volatile PCIeHeader *pci_header_in);

    // We actually have work to do on interrupts for HID devices
    kret_t HandleInterrupt() override;

    /*
     * ~VirtioHID
     * Cleans up all allocated virtqueues and frees this device.
     */
    ~VirtioHID();
};

END_C_HEADER

#endif // VIRTIO_HID_H
