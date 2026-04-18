/******************************************************************************
 * @file     camera.c
 * @version  V1.00
 * @brief    Camera capture operations and hardware interface.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#include "camera.h"
#include "NuMicro.h"

uint8_t frame_buf[FRAME_BYTES];														/* buffer to save the image */

uint8_t read_data_bus_pa(void)	{return (uint8_t)(PA->PIN & 0xFFu);}	/* Read the value of 8 image signal pin 						*/
uint8_t read_pclk				(void)	{return ( (PB->PIN & 0x1u));			 }	/* Read the value of pclk signal 										*/
uint8_t read_href				(void)  {return ( (PB->PIN & 0x2u));			 }	/* Read the value of href signal 										*/
uint8_t read_vsync			(void) 	{return ( (PB->PIN & 0x4u)); 			 }	/* Read the value of vsync signal 									*/
void wait_pclk_low			(void)  { while (read_pclk()); 						 }  /* wait PCLK until 0 start to sample (falling edge) */
void wait_pclk_high			(void) 	{ while (!read_pclk()); 					 }  /* wait PCLK until 1 (raising edge) 								*/


void Data_line_init(void)																	{ //Setting the line to collect data and frame signal
		GPIO_SetMode(PB, 0x7, GPIO_MODE_INPUT); /*Pclk, Hs, Vs*/
		GPIO_SetMode(PA, 0xFF, GPIO_MODE_INPUT);/* D0~D7 */
}

void CLK_Output(void)																			{ //output 12MHz CLK signal
		CLK->CLKOCTL = CLK_CLKOCTL_CLKOEN_Msk | 1 | (0 << CLK_CLKOCTL_DIV1EN_Pos);
		CLK->APBCLK0 |= CLK_APBCLK0_CLKOCKEN_Msk;
		CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_CLKOSEL_Msk)) | (CLK_CLKSEL1_CLKOSEL_HIRC); //48MHZ
}

void capture_frame(void)																	{ //sample the image data from ov7670
		int idx = 0;
		uint8_t pixel_byte;
				while (!read_vsync());     /* wait VSYNC high  */
				while (read_vsync());      /* wait VSYNC low   */
				for (int i = 0; i < SRC_HEIGHT; i++) {
						while (!read_href());  /* wait HREF high   */
						for (int j = 0; j < SRC_WIDTH * 2; j++) {
								while (!read_pclk());
								pixel_byte = read_data_bus_pa();
								/* down sample by 2 to save memory */
								// grey
//								if ((i % 2 == 0) && (j % 4 == 0)) {		//change this condition in order to sample the data type you need
//									frame_buf[idx] = pixel_byte;
//									idx++;
//								}
							
								// rgb
								frame_buf[idx] = pixel_byte;
								idx++;
								while (read_pclk());
						}
						while (read_href());
				}
				while (!read_vsync());
}
