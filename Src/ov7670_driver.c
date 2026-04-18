/******************************************************************************
 * @file     ov7670_driver.c
 * @version  V1.00
 * @brief    OV7670 camera sensor I2C driver.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#include "ov7670_driver.h"
#include <stdio.h>
#include "NuMicro.h"

uint8_t OV7670_Read(uint8_t sub_reg)									{	//Read value from [sub_reg] register
		/* Send Start */
		I2C_START(I2C0);
		CLK_SysTickDelay(100);
		/* SLA + W */
		I2C_SET_DATA(I2C0, 0x42);
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);
		/* SUB ADDR */
		I2C_SET_DATA(I2C0, sub_reg);
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);
		/* STOP */
		I2C_STOP(I2C0);
		CLK_SysTickDelay(100);
		/* Start */
		I2C_START(I2C0);
		CLK_SysTickDelay(100);
		/* SLA + R*/
		I2C_SET_DATA(I2C0, 0x43);
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);
		/* RECEIVE ACK AND CLR SI */
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);
		/* READ DATA */
		uint8_t data = I2C_GET_DATA(I2C0);
		CLK_SysTickDelay(100);
		
		I2C_STOP(I2C0);
		return data;
}

void check_register(uint8_t reg, uint8_t target_value){ //Check the value[target_value] writed to ov7670 register [reg]
		uint8_t compare = OV7670_Read(reg);
		if(target_value != compare)
			printf("write unsuccess at [0x%x], return value is 0x%x\n",reg, compare);
}

void ov7670_write_reg(uint8_t sub_reg, uint8_t data)	{ //Write value [data] to ov7670 register [sub_reg]
		/* Send Start */
		I2C_START(I2C0);
		CLK_SysTickDelay(100);

		/* SLA + W */
		I2C_SET_DATA(I2C0, 0x42);
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);
		
		/* Sub register address */
		I2C_SET_DATA(I2C0, sub_reg);
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);		
	
		/* Data */
		I2C_SET_DATA(I2C0, data);
		I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
		CLK_SysTickDelay(100);
		
		/* Send Stop */
		I2C_STOP(I2C0);
		CLK_SysTickDelay(100);
		//check_register(sub_reg, data);
}

void ov7670_init_rgb565(void)													{ //80x60 RGB565 setting
    
	ov7670_write_reg(0x12, 0x80); // COM7: Reset all registers
    CLK_SysTickDelay(100000);

    // ========================================================
    // Format Setup (RGB565)
    // ========================================================
    ov7670_write_reg(0x12, 0x04); // COM7: RGB output format
    ov7670_write_reg(0x40, 0xD0); // COM15: RGB565 format, Full Output Range (00->FF)
    ov7670_write_reg(0x8C, 0x00); // RGB444: Disable RGB444 format

    // ========================================================
    // Image Quality (Brightness & Contrast)
    // ========================================================
    ov7670_write_reg(0x55, 0x20); // BRIGHT: Brightness control 
    ov7670_write_reg(0x56, 0x7F); // CONTRAS: Contrast control 

    ov7670_write_reg(0x13, 0xe7); // COM8: Fast AGC, AEC unlimited step, Auto Banding, AWB, AGC, AEC enable
    ov7670_write_reg(0x6a, 0x60); // GGA: G Channel AWB Gain

    // ========================================================
    // Clock setup (PCLK)
    // ========================================================
    ov7670_write_reg(0x11, 0x03); // CLKRC: Internal clock pre-scalar (divided by 4)
    ov7670_write_reg(0x3E, 0x1B); // COM14: PCLK divider, manual scaling enable

    ov7670_write_reg(0x76, 0x00); // REG76: Black Level Calibration (BLC) parameters
    ov7670_write_reg(0x4c, 0x00); // DNSTH: De-noise strength
			
    // ========================================================
    // Scaling and Resolution (80x60)
    // ========================================================
    ov7670_write_reg(0x70, 0x3A); // SCALING_XSC: Horizontal scale factor
    ov7670_write_reg(0x71, 0x35); // SCALING_YSC: Vertical scale factor
    ov7670_write_reg(0x72, 0x33); // SCALING_DCWCTR: Downsample by 8 (HV)
    ov7670_write_reg(0x73, 0xF3); // SCALING_PCLK_DIV: Clock divider control for DSP scale
    ov7670_write_reg(0xA2, 0x02); // SCALING_PCLK_DELAY: Scaling PCLK delay

    // Frame Start/Stop window (HREF and VSYNC)
    ov7670_write_reg(0x17, 0x18); // HSTART: Horizontal frame start high 8-bit
    ov7670_write_reg(0x18, 0x06); // HSTOP: Horizontal frame end high 8-bit
    ov7670_write_reg(0x32, 0x24); // HREF: HREF Edge control (LSBs)
    ov7670_write_reg(0x19, 0x02); // VSTART: Vertical frame start high 8-bit
    ov7670_write_reg(0x1A, 0x7A); // VSTOP: Vertical frame end high 8-bit
    ov7670_write_reg(0x03, 0x00); // VREF: Vertical frame control (LSBs)


    ov7670_write_reg(0x6f, 0x9f); // AWBCTR0: Auto White Balance Control 0
    ov7670_write_reg(0xb0, 0x84); // Reserved/Test pattern
    ov7670_write_reg(0x3B, 0x42); // COM11: Night mode, Flicker detect and banding filter
    ov7670_write_reg(0x0C, 0x04); // COM3: Enable scale, DCW enable
    ov7670_write_reg(0x3d, 0x40); // COM13: Gamma enable, UV saturation
		
    // Gamma Curve (15 points)
    ov7670_write_reg(0x7a, 0x20); // GAM1
    ov7670_write_reg(0x7b, 0x10); // GAM2
    ov7670_write_reg(0x7c, 0x1e); // GAM3
    ov7670_write_reg(0x7d, 0x35); // GAM4
    ov7670_write_reg(0x7e, 0x5a); // GAM5
    ov7670_write_reg(0x7f, 0x69); // GAM6
    ov7670_write_reg(0x80, 0x76); // GAM7
    ov7670_write_reg(0x81, 0x80); // GAM8
    ov7670_write_reg(0x82, 0x88); // GAM9
    ov7670_write_reg(0x83, 0x8f); // GAM10
    ov7670_write_reg(0x84, 0x96); // GAM11
    ov7670_write_reg(0x85, 0xa3); // GAM12
    ov7670_write_reg(0x86, 0xaf); // GAM13
    ov7670_write_reg(0x87, 0xc4); // GAM14
    ov7670_write_reg(0x88, 0xd7); // GAM15
    ov7670_write_reg(0x89, 0xe8); // GAM16

		// ========================================================
		// Color Matrix Correction
		// ========================================================
		ov7670_write_reg(0x4f, 0x80); // MTX1
		ov7670_write_reg(0x50, 0x80); // MTX2
		ov7670_write_reg(0x51, 0x00); // MTX3
		ov7670_write_reg(0x52, 0x22); // MTX4
		ov7670_write_reg(0x53, 0x5e); // MTX5
		ov7670_write_reg(0x54, 0x80); // MTX6
		ov7670_write_reg(0x58, 0x9e); // MTXS: Matrix signs
		ov7670_write_reg(0x41, 0x08); // AWBCS: AWB Control Sign
}
