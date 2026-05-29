#include <fractal.h>
#include <oxide/compositor.h>
#include <io/mouse.h>
#include <io/hid/virtio-hid.h>
#include <io/gpu/virtio-gpu.h>

extern FractalGPU *global_gpu;
extern Compositor global_compositor;

kret_t VirtioHID::Negotiate() {
    kret_t rval;

    // Steps 1-6: Negotiate the feature bits
    rval = ResetBegin();

    if (ALL_GOOD != rval) return rval;

    // 7. Perform device-specific setup, including discovery of virtqueues for the device
    enable_virtqueue(header, VIRTIO_HID_EVENTQ_IDX, eventq);
    enable_virtqueue(header, VIRTIO_HID_STATUSQ_IDX, statusq);

    // Step 8: Finalize init
    return ResetEnd();
}

kret_t VirtioHID::Setup() {
    // Populate eventq with buffers from event_buffer
    if (SMALL_PAGE_SIZE < VIRTQUEUE_NUM_DESCRIPTORS * sizeof(virtio_input_event)) {
        panic("1 page is too small to hold all the event descriptors for a VirtioHID device");
    }
    for (usize i = 0; i < VIRTQUEUE_NUM_DESCRIPTORS; i++) {
        event_buffer[i] = {
            .type = 0,
            .code = 0,
            .value = 0,
        };

        eventq->desc[i].addr = KERN_V2P((usize)&event_buffer[i]);
        eventq->desc[i].flags = VIRTQ_DESC_F_WRITE;
        eventq->desc[i].len = sizeof(event_buffer[i]);
        eventq->desc[i].next = 0;
        eventq->avail->ring[i] = i;
        eventq->avail->idx++;
        __sync_synchronize();
    }

    prev_used_idx = 0;
    return ALL_GOOD;
}

void dump_VirtioHID_events(struct virtio_input_event *ebuf) {
    if (!ebuf) return;

    for (usize i = 0; i < VIRTQUEUE_NUM_DESCRIPTORS; i++) {
        printf("Buffer %d:\r\n", i);
        printf("\tType: 0x%X\r\n\tCode: 0x%X\r\n\tValue: 0x%X\r\n", ebuf[i].type, ebuf[i].code, ebuf[i].value);
    }
}

void dump_virtqueue(volatile Virtqueue *vq) {
    if (!vq) return;

    printf("used.idx: %d\r\n", vq->used->idx);
    for (usize i = 0; i < VIRTQUEUE_NUM_DESCRIPTORS; i++) {
        printf("\tring[%d].id: %d\r\n", i, vq->used->ring[i].id);
        printf("\tdesc[%d].addr: 0x%X\r\n", i, vq->desc[i].addr);
        printf("\tdesc[%d].next: 0x%X\r\n", i, vq->desc[i].next);
    }
}

void VirtioHID::DetermineKind() {
    volatile struct virtio_input_config *conf = (volatile struct virtio_input_config *)device_config;
    conf->select = VIRTIO_INPUT_CFG_ID_NAME;
    conf->subsel = 0;

#ifndef QUIET_BOOT
    printf("Detected %s\r\n", conf->u.string);
#endif // !QUIET_BOOT
    memcpy(device_name, (void *)&conf->u.string, VIRTIO_HID_NAME_LEN);

    if (strequal(device_name, QEMU_VIRTIO_HID_KEYBOARD_NAME)) {
        device_type = HID_KEYBOARD;
        strncpy(pci_name, VIRTIO_KEYBOARD_DEVICE_NAME, PCI_DEVICE_NAMELEN);
    } else if (strequal(device_name, QEMU_VIRTIO_HID_MOUSE_NAME)) {
        device_type = HID_MOUSE;
        strncpy(pci_name, VIRTIO_MOUSE_DEVICE_NAME, PCI_DEVICE_NAMELEN);
    } else if (strequal(device_name, QEMU_VIRTIO_HID_TABLET_NAME)) {
        device_type = HID_TABLET;
        strncpy(pci_name, VIRTIO_TABLET_DEVICE_NAME, PCI_DEVICE_NAMELEN);
    } else {
        printf("Unknown HID device\r\n");
        device_type = HID_UNKNOWN;
    }
}

extern i32 mouse_x, mouse_y;

void decode_virtio_key_packet(struct virtio_input_event *ev, fractal_keyboard_state_t *keystate) {
    if (!ev || ev->type != EV_KEY) return;

    if (ev->code == BTN_MOUSE) {
        mouse_click(ev->value != 0);
    }

    char key_pressed = decode_virtio_keypress(ev, keystate);

    if (ev->value == INPUT_EVENT_VALUE_PRESS) {
        global_compositor.SendKey(key_pressed);
    }
}

void decode_virtio_rel_packet(struct virtio_input_event *ev) {
    if (!ev || ev->type != EV_REL) return;
    switch(ev->code) {
        case REL_X:
        mouse_x += ev->value;
        break;

        case REL_Y:
        mouse_y += ev->value;
        break;
    }
}

void decode_virtio_abs_packet(struct virtio_input_event *ev) {
    switch(ev->code) {
        case ABS_X:
        mouse_x = ev->value;
        break;

        case ABS_Y:
        mouse_y = ev->value;
        break;
    }

    mouse_moved();
}

void decode_virtio_event(struct virtio_input_event *ev, fractal_keyboard_state_t *keystate) {
    switch (ev->type) {
        case EV_SYN:
        // Sync packet, do nothing
        break;

        case EV_KEY:
        // Key press
        decode_virtio_key_packet(ev, keystate);
        break;

        case EV_REL:
        // Mouse movement
        decode_virtio_rel_packet(ev);
        break;

        case EV_ABS:
        decode_virtio_abs_packet(ev);
        break;

        default:
        printf("Unknown virtio event: %d\r\n", ev->type);
        break;
    }
}

void VirtioHID::ReceiveEvent() {
    // Consume an event, and advance the available ring accordingly
    // Keyboard events use two descriptors, mouse events use three
    u16 cur_idx = eventq->used->idx;
    u16 packets_to_process = cur_idx - prev_used_idx;

    if (packets_to_process == 0) return;

    for (usize packet = 0; packet < packets_to_process; packet++) {
        usize descriptor_idx = (packet + prev_used_idx) % VIRTQUEUE_NUM_DESCRIPTORS;
        struct virtio_input_event *event = &event_buffer[eventq->used->ring[descriptor_idx].id];
        decode_virtio_event(event, &keyboard_state);
    }

    eventq->avail->idx += (prev_used_idx - eventq->used->idx);
    prev_used_idx = eventq->used->idx;
}

void VirtioHID::Poll() {
    if (eventq->used->idx == prev_used_idx) return;

#ifdef DEBUG_VIRTIO_POLL
    printf("%d => %d\r\n", orig_used_idx, eventq->used->idx);
#endif // DEBUG_VIRTIO_POLL

    ReceiveEvent();

    switch(device_type) {
        case HID_KEYBOARD:
        break;

        case HID_MOUSE:
        case HID_TABLET:
        if (!global_gpu) {
            return;
        }
        // Don't need to repaint after this, just set the mouse cursor
        global_gpu->SetMouse(mouse_x, mouse_y);
        break;
    }
}

VirtioHID::VirtioHID(volatile virtio_pci_common_cfg *h_in, volatile u64 *notify_in, void *dev_conf_in, void *isr_conf_in, volatile PCIeHeader *pci_header_in) : VirtioPCI(h_in, notify_in, dev_conf_in, isr_conf_in, pci_header_in) {
    if (!h_in) panic("Invalid virtio_pci_common_cfg header for HID");
    header = h_in;
    notify = notify_in;
    device_config = dev_conf_in;
    isr_config = isr_conf_in;
    pci_header = pci_header_in;
    keyboard_state = {0};
    prev_used_idx = 0;
    status_flags = 0;
    memset(device_name, 0, VIRTIO_HID_NAME_LEN);
    device_type = HID_UNKNOWN;

    statusq = new Virtqueue(notify);
    eventq  = new Virtqueue(notify);

    if (!statusq || !eventq) {
        panic("Failed to initialize VirtioHID virtqueues");
    }

    event_buffer = (struct virtio_input_event *)AllocPage(SMALL_PAGE);
    if (!event_buffer) {
        panic("Failed to allocate an event buffer for VirtioHID");
    }
}

kret_t VirtioHID::HandleInterrupt() {
    u32 tmp;
    // Reading the isr config port clears the interrupt
    // There are two reasons this signal could have been raised-
    // either a device configuration change or a used virtqueue notification
    // We already can just assume its the used queue notif tho so just call Poll()
    tmp = *(volatile u32 *)(this->isr_config);
    this->Poll();
    return ALL_GOOD;
}

VirtioHID::~VirtioHID() {
    delete statusq;
    delete eventq;

    statusq = NULL;
    eventq  = NULL;

    FreePage((usize)event_buffer);
}
