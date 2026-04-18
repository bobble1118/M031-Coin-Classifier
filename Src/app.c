/******************************************************************************
 * @file     app.c
 * @version  V1.00
 * @brief    Application main logic and CNN inference integration.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#include "app.h"
#include <stdio.h>
#include "NuMicro.h"
#include "camera.h"
#include "cnn_api.h"
#include "ov7670_driver.h"

void send_header(void)																		{	//send a header to sync the frame
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		UART_WRITE(UART0, 0xAA);
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		UART_WRITE(UART0, 0x55);
}

void send_footer(void)																		{ //send a footer to sync the frame
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		UART_WRITE(UART0, 0x55);
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		UART_WRITE(UART0, 0xAA);
}

void uart_send_buffer(uint32_t len)										{ //send every data in frame_buf[]
    uint32_t i = 0;
    while (i < len) {
        while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
				UART_WRITE(UART0, frame_buf[i]);
				i++;
    }
}

void send_frame(void)																			{ //send one frame (the whole frame_buf)
		send_header();
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		uart_send_buffer(FRAME_BYTES);
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		send_footer();
}


void predict(void)																				{ //predict function (CNN predict)
		uint8_t digit = -1;
    run_cnn_inference_40x30(frame_buf, &digit);
		while (UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		switch (digit){
			case 0:
				printf("\rCoin : 1        ");
				break;

			case 1:
				printf("\rCoin : 5        ");
				break;
			
			case 2:
				printf("\rCoin : 10       ");
				break;
			
			case 3:
				printf("\rCoin : 20       ");
				break;
			
			case 4:
				printf("\rCoin : 50       ");
				break;
			
			case 5:
				printf("\rCoin : Nothing  ");
				break;
			
			default:
				printf("\rCoin : Nothing  ");
				break;
			
		}
}

void app_main(void) {
		ov7670_write_reg(0x12,0x80); 	/* restart all register */									
		check_register(0x12,0x00);		/* check whether the restart value is equal to 0x00 ? */
		
		ov7670_init_rgb565();
		Data_line_init();
		for(int i=0;i<3;i++){
			CLK_SysTickDelay(340000);
		}
		uint8_t reg_val = OV7670_Read(0x13);
		
		for(int i=0;i<10;i++){
			CLK_SysTickDelay(340000);
		}
    reg_val &= ~(1 << 0); // Clear Bit 0 (AEC Disable)
//    reg_val &= ~(1 << 2); // Clear Bit 2 (AGC Disable - optional)
    
    ov7670_write_reg(0x13, reg_val);
		printf("\nCoin Classfication Starting...\n");
		while (!read_vsync()); /* wait VSYNC high */
		while(1){
				capture_frame();
				//send_frame();
				predict();
		}
}
