/******************************************************************************
 * @file     ov7670_driver.h
 * @version  V1.00
 * @brief    Header file for OV7670 camera sensor I2C driver.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#ifndef __OV7670_DRIVER_H__
#define __OV7670_DRIVER_H__

#include <stdint.h>

uint8_t OV7670_Read(uint8_t sub_reg);
void check_register(uint8_t reg, uint8_t target_value);
void ov7670_write_reg(uint8_t sub_reg, uint8_t data);
void ov7670_init_rgb565(void);

#endif /* __OV7670_DRIVER_H__ */
