/******************************************************************************
 * @file     camera.h
 * @version  V1.00
 * @brief    Header file for camera capture operations.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdint.h>

#define BYTE_PER_PIXEL     2

#define SRC_WIDTH   80
#define SRC_HEIGHT  60
#define DST_WIDTH   80 // target width
#define DST_HEIGHT  60 // target height
#define FRAME_BYTES (DST_WIDTH * DST_HEIGHT * BYTE_PER_PIXEL)

extern uint8_t frame_buf[FRAME_BYTES];

uint8_t read_data_bus_pa(void);
uint8_t read_pclk(void);
uint8_t read_href(void);
uint8_t read_vsync(void);
void wait_pclk_low(void);
void wait_pclk_high(void);

void Data_line_init(void);
void CLK_Output(void);

void capture_frame(void);

#endif /* __CAMERA_H__ */
