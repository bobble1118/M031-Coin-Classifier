/******************************************************************************
 * @file     app.h
 * @version  V1.00
 * @brief    Header file for application logic.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#ifndef __APP_H__
#define __APP_H__

#include <stdint.h>

void uart_send_buffer(uint32_t len);
void send_header(void);
void send_footer(void);
void send_frame(void);
void predict(void);
void app_main(void);

#endif /* __APP_H__ */
