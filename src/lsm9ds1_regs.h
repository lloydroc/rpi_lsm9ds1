#ifndef LSM9DS1_REGS_H
#define LSM9DS1_REGS_H

enum lsm9ds1_odr
{
  ODR_POWER_DOWN,
  ODR_14p9_HZ,
  ODR_59p5_HZ,
  ODR_119_HZ,
  ODR_238_HZ,
  ODR_476_HZ,
  ODR_952_HZ
};

#define ACT_THS 0x04
#define SLEEP_ON_INACT_EN 0b10000000

#define INT_GEN_CFG_XL 0x06
#define AOI_XL         0b10000000
#define SIX_D          0b01000000
#define ZHIE_XL        0b00100000
#define ZLIE_XL        0b00010000
#define YHIE_XL        0b00001000
#define YLIE_XL        0b00000100
#define XHIE_XL        0b00000010
#define XLIE_XL        0b00000001

#define REFERENCE_G 0x0B

#define INT1_CTRL   0x0C
#define INT1_IG_G   0b10000000
#define INT1_IG_XL  0b01000000
#define INT_FSS5    0xb0100000
#define INT_OVR     0b00010000
#define INT_FTH     0b00001000
#define INT_Boot    0b00000100
#define INT_DRDY_G  0b00000010
#define INT_DRDY_XL 0b00000001

#define INT2_CTRL      0x0D
#define INT2_INACT     0b10000000
#define INT2_FSS5      0b00100000
#define INT2_OVR       0b00010000
#define INT2_FTH       0b00001000
#define INT2_DRDY_TEMP 0b00000100
#define INT2_DRDY_G    0b00000010
#define INT2_DRDY_XL   0b00000001

#define WHO_AM_I     0x0F
#define WHO_AM_I_VAL 0b01101000

#define CTRL_REG1_G 0x10
#define CTRL_REG2_G 0x11
#define INT_SEL     0b00001100
#define OUT_SEL     0b00000011

#define CTRL_REG3_G 0x12

#define INT_GEN_SRC_G 0x14
#define IA_G          0b01000000
#define ZH_G          0b00100000
#define ZL_G          0b00010000
#define YH_G          0b00001000
#define YL_G          0b00000100
#define XH_G          0b00000010
#define XL_G          0b00000001

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
#define CTRL_REG6_XL 0x20
#define CTRL_REG7_XL 0x21

#define CTRL_REG8  0x22
#define BOOT       0b10000000
#define BDU        0b01000000
#define H_LACTIVE  0b00100000
#define PP_OD      0b00010000
#define SIM        0b00001000
#define IF_ADD_INC 0b00000100
#define BLE        0b00000010
#define SW_RESET   0b00000001

#define CTRL_REG9     0x23
#define SLEEP_G       0b01000000
#define FIFO_TEMP_EN  0b00010000
#define DRDY_mask_bit 0b00001000
#define I2C_DISABLE   0b00000100
#define FIFO_EN       0b00000010
#define STOP_ON_FTH   0b00000001

#define CTRL_REG10 0x24
#define ST_G       0b00000100
#define ST_XL      0b00000001

#define INT_GEN_SRC_XL 0x26
#define IA_XL          0b01000000
#define ZH_XL          0b00100000
#define ZL_XL          0b00010000
#define YH_XL          0b00001000
#define YL_XL          0b00000100
#define XH_XL          0b00000010
#define XL_XL          0b00000001

#define OUT_X_XL      0x28
#define OUT_Y_XL      0x2A
#define OUT_Z_XL      0x2C

#define FIFO_CTRL 0x2E
#define FIFO_STATUS 0x2F

#define FIFO_SRC 0x2F
#define FTH      0b10000000
#define OVRN     0b01000000
#define FSS      0b00111111

#define INT_GEN_CFG_G 0x30
#define AOI_G         0b10000000
#define LIR_G         0b01000000
#define ZHIE_G        0b00100000
#define ZLIE_G        0b00010000
#define YHIE_G        0b00001000
#define YLIE_G        0b00000100
#define XHIE_G        0b00000010
#define XLIE_G        0b00000001

#define INT_GEN_THS_X_G 0x31
#define DCRM_G          0b10000000

#define INT_GEN_THS_Y_G 0x33
#define INT_GEN_THS_Z_G 0x33
#define INT_GEN_DUR_G 0x37
#define WAIT_G          0b10000000

#endif
