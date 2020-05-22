#ifndef LSM9DS1_REGS_H
#define LSM9DS1_REGS_H

#define ACT_THS 0x04
#define SLEEP_ON_INACT_EN 0b10000000
#define INT1_CTRL   0x0C
#define INT1_IG_G   0b10000000
#define INT1_IG_XL  0b01000000
#define INT_FSS5    0xb0100000
#define INT_OVR     0b00010000
#define INT_FTH     0b00001000
#define INT_Boot    0b00000100
#define INT_DRDY_G  0b00000010
#define INT_DRDY_XL 0b00000001

#define WHO_AM_I 0x0F

#define CTRL_REG1_G 0x10
#define CTRL_REG2_G 0x11
#define INT_SEL     0b00001100
#define OUT_SEL     0b00000011

#define CTRL_REG3_G 0x12

#define OUT_TEMP_L 0x15
#define OUT_TEMP_H 0x16

#define STATUS_REG  0x17
#define IG_XL       0b01000000
#define IG_G        0b00100000
#define INACT       0b00010000
#define BOOT_STATUS 0b00001000
#define TDA         0b00000100
#define GDA         0b00000010
#define XLDA        0b00000001

#define OUT_X_G 0x18
#define OUT_Y_G 0x1A
#define OUT_Z_G 0x1C

#define CTRL_REG4 0x1E
#define Zen_G     0b00100000
#define Yen_G     0b00010000
#define Xen_G     0b00001000
#define LIR_XL1   0b00000010
#define FOURD_XL1 0b00000001

#define CTRL_REG5_XL 0x1F
#define CTRL_REG6_XL 0x21

#define CTRL_REG9     0x23
#define SLEEP_G       0b01000000
#define FIFO_TEMP_EN  0b00010000
#define DRDY_mask_bit 0b00001000
#define I2C_DISABLE   0b00000100
#define FIFO_EN       0b00000010
#define STOP_ON_FTH   0b00000001

#define OUT_X_XL      0x28
#define OUT_Y_XL      0x2A
#define OUT_Z_XL      0x2C

#define FIFO_CTRL 0x2E
#define FIFO_STATUS 0x2F

#define FIFO_SRC 0x2F
#define FTH      0b10000000
#define OVRN     0b01000000
#define FSS      0b00111111

#endif
