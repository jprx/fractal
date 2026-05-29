#ifndef PS2_HID_H
#define PS2_HID_H

// Drivers for the IBM compat PS/2 keyboard and mouse peripherals

#include <fractal.h>

BEGIN_C_HEADER

#define PS2_TIMEOUT_RETRIES ((100000))

// PS2 IO ports
#define PS2_IO_DATA    ((0x60))
#define PS2_IO_COMMAND ((0x64))
#define PS2_IO_STATUS  ((0x64))

// Commands for the controller
#define PS2_CMD_READ_CONFIG_BYTE  ((0x20))
#define PS2_CMD_WRITE_CONFIG_BYTE ((0x60))
#define PS2_CMD_DISABLE_PORT2     ((0xA7))
#define PS2_CMD_ENABLE_PORT2      ((0xA8))
#define PS2_CMD_TEST_PORT2        ((0xA9))
#define PS2_CMD_PERFORM_SELFTEST  ((0xAA))
#define PS2_CMD_TEST_PORT1        ((0xAB))
#define PS2_CMD_DISABLE_PORT1     ((0xAD))
#define PS2_CMD_ENABLE_PORT1      ((0xAE))
#define PS2_NEXT_BYTE_PORT2       ((0xD4))

// Commands for devices
#define PS2_DEVICE_CMD_RESET ((0xFF))

#define PS2_DEVICE_RESET_OK1  ((0xFA))
#define PS2_DEVICE_RESET_OK2  ((0xAA))
#define PS2_DEVICE_RESET_FAIL ((0xFC))

#define PS2_DEVICE_ID_STANDARD_MOUSE    ((0x00))
#define PS2_DEVICE_ID_SCROLLWHEEL_MOUSE ((0x03))

// Mouse-Specific Commands
#define PS2_MOUSE_SET_2_TO_1_SCALING    ((0xE7))
#define PS2_MOUSE_ENABLE_DATA_REPORTING ((0xF4))

#define PS2_SELFTEST_PASS ((0x55))
#define PS2_SELFTEST_FAIL ((0xFC))

/*
 PS2 Status Register

 |      7     |    6    |5   4|     3     |    2     |      1       |       0       |
 +----------------------------------------------------------------------------------+
 | Parity Err | Timeout | Unk | Cmd/ Data | Sys Flag | Input Status | Output Status |
 +----------------------------------------------------------------------------------+

 Parity Error: 0 = no error, 1 = error
 Timeout Error: 0 = no error, 1 = error
 Unknown: Chipset-Specific
 Command/ Data: 0 = data is for device, 1 = data is for controller
 System Flag: Something related to POST
 Input Status: 0 = empty, 1 = full (must be clear before trying to write)
 Output Status: 0 = empty, 1 = full (must be set before trying to read)
*/

#define PS2_STATUS_OUTPUT_FULL             BIT(0)
#define PS2_STATUS_INPUT_FULL              BIT(1)
#define PS2_STATUS_DATA_FOR_CONTROLLER     BIT(3)

/*
 PS2 Controller Configuration Byte (first byte of i8042 RAM)

 |  7  |     6    |     5    |    4     |  3  |    2    |    1   |    0   |
 +------------------------------------------------------------------------+
 | MBZ | P1 Xlate | P2 Clock | P1 Clock | SBZ | Sysflag | P2 Int | P1 Int |
 +------------------------------------------------------------------------+

 MBZ / SBZ = "Must" / "Should" be zero
 P1 Xlate (First Port Translation): 0 = off, 1 = on (on = translate scan code set 2 -> scan code set 1, the older one)
 P1/ P2 Clock: 0 = clock on, 1 = clock off (active low!)
 System Flag: 1 = system passed POST, 0 = impossible (OS should not be running?)
 P1/ P2 Interrupt: 0 = interrupt disabled, 1 = interrupt enabled (active high)
*/

#define PS2_CONTROLLER_CONFIG_P1_CLOCK_OFF BIT(4)
#define PS2_CONTROLLER_CONFIG_P2_CLOCK_OFF BIT(5)

// Clocks on, translation + interrupts off.
#define PS2_CONTROLLER_CONFIG_INTERRUPTS_OFF ((0x00))

// Clocks on, interrupts on
#define PS2_CONTROLLER_CONFIG_INTERRUPTS_ON  ((0x03))

typedef enum ps2_deviceid {
    PS2_DEVICE1 = 1,
    PS2_DEVICE2 = 2,
} ps2_deviceid;

#define PS2_KEYBOARD ((PS2_DEVICE1))
#define PS2_MOUSE    ((PS2_DEVICE2))

// After this many frames with no mouse interrupts, the mouse resets itself
#define PS2_MOUSE_RESET_FRAMECOUNT ((5))

/*
 * initialize_8042
 * Detect and initialize the 8042 PS/2 controller.
 *
 * This will disable both ports, flush any remaining inputs,
 * reset and reconfigure all devices, and returns with the controller
 * fully initialized and ready to go.
 */
void initialize_8042();

u8 i8042_read_cmd(u8 command_byte);
void i8042_write_cmd(u8 command_byte, u8 val);
void i8042_pure_cmd(u8 command_byte);
void ps2_send_byte_to_dev(ps2_deviceid device, u8 data);
u8 ps2_read_byte_from_dev();
void ps2_reset_device(ps2_deviceid device);
void ps2_keyboard_interrupt();
void ps2_mouse_interrupt();
char ps2_decode_keypress(u8 code);
void ps2_decode_mouse(i8 pkt);
bool is_ps2_data_available();

END_C_HEADER

#endif // PS2_HID_H
