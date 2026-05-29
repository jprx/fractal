#include <fractal.h>
#include <arch/x86.h>
#include <io/hid/ps2-hid.h>
#include <oxide/compositor.h>

extern FRACTALCORE_CLASS global_cpu;
extern Compositor global_compositor;

// See: https://wiki.osdev.org/%228042%22_PS/2_Controller

// Issue a command byte, return its value
extern "C" u8 i8042_read_cmd(u8 command_byte) {
    usize retries = 0;
    outb(PS2_IO_COMMAND, command_byte);
    while((inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL) == 0) {
        if (retries++ > PS2_TIMEOUT_RETRIES) {
            printf("Warning: PS/2 Controller read command timed out\r\n");
            return 0;
        }
    }
    return inb(PS2_IO_DATA);
}

// Issue a command byte, write into it
extern "C" void i8042_write_cmd(u8 command_byte, u8 val) {
    usize retries = 0;
    outb(PS2_IO_COMMAND, command_byte);
    while((inb(PS2_IO_STATUS) & PS2_STATUS_INPUT_FULL) != 0) {
        if (retries++ > PS2_TIMEOUT_RETRIES) {
            printf("Warning: PS/2 Controller write command timed out\r\n");
            return;
        }
    }

#ifdef WARN_IF_CONFIG_IS_STRANGE
    if (0 == (inb(PS2_IO_STATUS) & PS2_STATUS_DATA_FOR_CONTROLLER)) {
        printf("[Warning] Sending data to controller, but status thinks it is for device\r\n");
    }
#endif // WARN_IF_CONFIG_IS_STRANGE

    outb(PS2_IO_DATA, val);
}

// Command with no arguments and no return value
extern "C" void i8042_pure_cmd(u8 command_byte) {
    outb(PS2_IO_COMMAND, command_byte);
}

extern "C" void ps2_send_byte_to_dev(ps2_deviceid device, u8 data) {
    usize retries = 0;
    if (PS2_DEVICE2 == device) {
        i8042_pure_cmd(PS2_NEXT_BYTE_PORT2);
    }

    while((inb(PS2_IO_STATUS) & PS2_STATUS_INPUT_FULL) != 0) {
        if (retries++ > PS2_TIMEOUT_RETRIES) {
            printf("Warning: PS/2 Device write command timed out\r\n");
            return;
        }
    }

#ifdef WARN_IF_CONFIG_IS_STRANGE
    if (0 != (inb(PS2_IO_STATUS) & PS2_STATUS_DATA_FOR_CONTROLLER)) {
        printf("[Warning] Sending data to device, but status thinks it is for controller\r\n");
    }
#endif // WARN_IF_CONFIG_IS_STRANGE

    outb(PS2_IO_DATA, data);
}

extern "C" bool is_ps2_data_available() {
    return (inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL) != 0;
}

// Should come from an interrupt context, so the caller should know which dev this is from:
extern "C" u8 ps2_read_byte_from_dev() {
    usize retries = 0;
    while((inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL) == 0) {
        if (retries++ > PS2_TIMEOUT_RETRIES) {
            printf("Warning: PS/2 Device read command timed out\r\n");
            return 0;
        }
    }
    return inb(PS2_IO_DATA);
}

extern "C" void ps2_reset_device(ps2_deviceid device) {
    volatile u8 tmp8;

#ifndef QUIET_BOOT
    printf("[PS/2] Resetting Device %d\r\n", device);
#endif // !QUIET_BOOT

    ps2_send_byte_to_dev(device, PS2_DEVICE_CMD_RESET);

    // tmp8 = ps2_read_byte_from_dev();
    // if (PS2_DEVICE_RESET_FAIL == tmp8) printf("Failed resetting PS/2 device %d", device);
    // if (PS2_DEVICE_RESET_OK1 != tmp8) printf("Unknown device reset response for device %d: 0x%X\r\n", device, tmp8);

    // tmp8 = ps2_read_byte_from_dev();
    // if (PS2_DEVICE_RESET_OK2 != tmp8) printf("Unknown second reset byte for device %d: 0x%X\r\n", device, tmp8);

    // // Parse device configuration bytes
    // switch(device) {
    //     case PS2_KEYBOARD:
    //         // No more bytes should be sent for the keyboard device
    //         // Some other kinds of keyboards may report more data here, we could just ignore all remaining bytes
    //         if ((inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL) != 0) printf("We finished resetting the keyboard but it has more to say\r\n");
    //     break;

    //     case PS2_MOUSE:
    //         // Real hardware doesn't seem to report any more ID bytes?
    //         if ((inb(PS2_IO_STATUS) & PS2_STATUS_OUTPUT_FULL) == 0) {
    //             // printf("Mouse isn't telling us it's a mouse");
    //             printf("Warning: Mouse has no device ID packets\r\n");
    //             return;
    //         }
    //         tmp8 = ps2_read_byte_from_dev();
    //         if ((PS2_DEVICE_ID_SCROLLWHEEL_MOUSE != tmp8) && (PS2_DEVICE_ID_STANDARD_MOUSE != tmp8)) printf("Mouse's device ID byte isn't a mouse we recognize\r\n");
    //     break;
    // }

    // Let the device yap until it's done
    bool should_break = false;
    while(!should_break) {
        should_break = true;
        for (usize count = 0; count < PS2_TIMEOUT_RETRIES; count++) {
            if (is_ps2_data_available()) {
                tmp8 = ps2_read_byte_from_dev();

#ifndef QUIET_BOOT
                printf("[r] (%d): 0x%X\r\n", count, tmp8);
#endif // !QUIET_BOOT

                should_break = false;
                break;
            }
        }
    }
}

extern "C" void ps2_keyboard_interrupt() {
    u8 code;
    char key_pressed;
    InterruptMode old_ints = global_cpu.SetInterrupts(INTS_DISABLED);

    code = inb(PS2_IO_DATA);
    key_pressed = ps2_decode_keypress(code);
    if (0 != key_pressed) global_compositor.SendKey(key_pressed);

    global_cpu.SetInterrupts(old_ints);
}

extern "C" void ps2_mouse_interrupt() {
    u8 code;
    InterruptMode old_ints = global_cpu.SetInterrupts(INTS_DISABLED);

    code = inb(PS2_IO_DATA);
    ps2_decode_mouse(code);

    global_cpu.SetInterrupts(old_ints);
}

extern "C" void initialize_8042() {
    volatile u8 tmp8;

    // Disable all ports and read any waiting data
    outb(PS2_IO_COMMAND, PS2_CMD_DISABLE_PORT1);
    outb(PS2_IO_COMMAND, PS2_CMD_DISABLE_PORT2);
    tmp8 = inb(PS2_IO_DATA);

    // Set controller configuration byte- enable clocks, disable interrupts
    tmp8 = i8042_read_cmd(PS2_CMD_READ_CONFIG_BYTE);
#ifndef QUIET_BOOT
    printf("Config Byte: 0x%X\n", tmp8);
#endif // !QUIET_BOOT

    i8042_write_cmd(PS2_CMD_WRITE_CONFIG_BYTE, PS2_CONTROLLER_CONFIG_INTERRUPTS_OFF);

    // Self-test
    tmp8 = i8042_read_cmd(PS2_CMD_PERFORM_SELFTEST);
    if (PS2_SELFTEST_PASS != tmp8) {
        printf("PS/2 Controller Failed Self-Test: 0x%X", tmp8);
    } else {
#ifndef QUIET_BOOT
        printf("PS/2 Controller Passed Self-Test\r\n");
#endif // !QUIET_BOOT
    }

    // Detect Ports- turn off clock for port 2 and try to enable it, clock should be re-enabled
    // Are there two ports? There better be.
    i8042_write_cmd(PS2_CMD_WRITE_CONFIG_BYTE, PS2_CONTROLLER_CONFIG_INTERRUPTS_OFF | PS2_CONTROLLER_CONFIG_P2_CLOCK_OFF);
    i8042_pure_cmd(PS2_CMD_ENABLE_PORT2);
    tmp8 = i8042_read_cmd(PS2_CMD_READ_CONFIG_BYTE);
    if ((tmp8 & PS2_CONTROLLER_CONFIG_P2_CLOCK_OFF) != 0) {
        printf("PS/2 Controller only supports 1 port, what is this nonsense?\r\n");
    } else {
#ifndef QUIET_BOOT
        printf("PS/2 Controller has two ports\r\n");
#endif // !QUIET_BOOT
    }

    // Check Interfaces
    if (0 != i8042_read_cmd(PS2_CMD_TEST_PORT1)) printf("PS/2 Port 1 test failed\r\n");
    if (0 != i8042_read_cmd(PS2_CMD_TEST_PORT2)) printf("PS/2 Port 2 test failed\r\n");

#ifndef QUIET_BOOT
    printf("PS/2 Ports passed interface tests\r\n");
#endif // !QUIET_BOOT

    // Enable Devices
    i8042_pure_cmd(PS2_CMD_ENABLE_PORT1);
    i8042_pure_cmd(PS2_CMD_ENABLE_PORT2);

    // Reset Devices
    ps2_reset_device(PS2_DEVICE1);
    ps2_reset_device(PS2_DEVICE2);

#ifndef QUIET_BOOT
    printf("All PS/2 Devices Reset\r\n");
#endif // !QUIET_BOOT

    // Enable Mouse Interrupts
    ps2_send_byte_to_dev(PS2_DEVICE2, PS2_MOUSE_ENABLE_DATA_REPORTING);
    ps2_read_byte_from_dev();

    // Set mouse scaling to 2:1
    // Doesn't "feel right" in Qemu, but works in real hardware
// #define OPTION_CONFIGURE_PS2_MOUSE_SCALING 1
#ifdef OPTION_CONFIGURE_PS2_MOUSE_SCALING
    ps2_send_byte_to_dev(PS2_DEVICE2, PS2_MOUSE_SET_2_TO_1_SCALING);
    ps2_read_byte_from_dev();
#endif // OPTION_CONFIGURE_PS2_MOUSE_SCALING

    // Enable Interrupts
    i8042_write_cmd(PS2_CMD_WRITE_CONFIG_BYTE, PS2_CONTROLLER_CONFIG_INTERRUPTS_ON);

#ifndef QUIET_BOOT
    printf("i8042 Init Done\r\n");
#endif // !QUIET_BOOT

    return;
}
