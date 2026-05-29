#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint8_t regs[256];
  uint8_t reg_addr;
  bool expecting_reg_addr;
} chip_state_t;

static void put_u16_le(uint8_t *regs, uint8_t addr, uint16_t value) {
  regs[addr] = value & 0xff;
  regs[addr + 1] = value >> 8;
}

static void put_s16_le(uint8_t *regs, uint8_t addr, int16_t value) {
  put_u16_le(regs, addr, (uint16_t)value);
}

static void put_raw20(uint8_t *regs, uint8_t addr, uint32_t value) {
  regs[addr] = (value >> 12) & 0xff;
  regs[addr + 1] = (value >> 4) & 0xff;
  regs[addr + 2] = (value & 0x0f) << 4;
}

static void init_registers(chip_state_t *chip) {
  memset(chip->regs, 0, sizeof(chip->regs));

  chip->regs[0xd0] = 0x60; // BME280 chip ID
  chip->regs[0xf3] = 0x00; // status: no measuring/update active

  // Calibration data from the Bosch BME280 compensation example.
  put_u16_le(chip->regs, 0x88, 27504);
  put_s16_le(chip->regs, 0x8a, 26435);
  put_s16_le(chip->regs, 0x8c, -1000);
  put_u16_le(chip->regs, 0x8e, 36477);
  put_s16_le(chip->regs, 0x90, -10685);
  put_s16_le(chip->regs, 0x92, 3024);
  put_s16_le(chip->regs, 0x94, 2855);
  put_s16_le(chip->regs, 0x96, 140);
  put_s16_le(chip->regs, 0x98, -7);
  put_s16_le(chip->regs, 0x9a, 15500);
  put_s16_le(chip->regs, 0x9c, -14600);
  put_s16_le(chip->regs, 0x9e, 6000);
  chip->regs[0xa1] = 75;

  put_s16_le(chip->regs, 0xe1, 362);
  chip->regs[0xe3] = 0;
  chip->regs[0xe4] = 0x14;
  chip->regs[0xe5] = 0x25;
  chip->regs[0xe6] = 0x03;
  chip->regs[0xe7] = 30;

  // Raw sample values that compensate to roughly room conditions.
  put_raw20(chip->regs, 0xf7, 415148); // pressure
  put_raw20(chip->regs, 0xfa, 519888); // temperature
  chip->regs[0xfd] = 28475 >> 8;       // humidity
  chip->regs[0xfe] = 28475 & 0xff;
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (!read) {
    chip->expecting_reg_addr = true;
  }
  return address == 0x76;
}

static uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  uint8_t value = chip->regs[chip->reg_addr];
  chip->reg_addr++;
  return value;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (chip->expecting_reg_addr) {
    chip->reg_addr = data;
    chip->expecting_reg_addr = false;
    return true;
  }

  if (chip->reg_addr == 0xe0 && data == 0xb6) {
    init_registers(chip);
  } else {
    chip->regs[chip->reg_addr] = data;
  }
  chip->reg_addr++;
  return true;
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  init_registers(chip);
  chip->reg_addr = 0;
  chip->expecting_reg_addr = true;

  i2c_config_t config = {
      .address = 0x76,
      .scl = pin_init("SCL", INPUT_PULLUP),
      .sda = pin_init("SDA", INPUT_PULLUP),
      .connect = on_i2c_connect,
      .read = on_i2c_read,
      .write = on_i2c_write,
      .disconnect = NULL,
      .user_data = chip,
  };
  i2c_init(&config);
}
