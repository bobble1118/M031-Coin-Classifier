/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    Application entry point.
 *
 * @note     M031 Coin Classifier
 *****************************************************************************/
#include <stdio.h>
#include "NuMicro.h"
#include "app.h"
#include "camera.h"
#include "ov7670_driver.h"

void SYS_Init(void)	{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable UART0 clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Switch UART0 clock source to HIRC */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Enable I2C0 clock */
    CLK_EnableModuleClock(I2C0_MODULE);
		
		CLK_EnableModuleClock(PWM0_MODULE);
    CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL2_PWM0SEL_PCLK0, 0);
		
    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* Set I2C0 multi-function pins */
    SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC0MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk)) |
                    (SYS_GPC_MFPL_PC0MFP_I2C0_SDA | SYS_GPC_MFPL_PC1MFP_I2C0_SCL);
		
		SYS->GPB_MFPL = (SYS->GPB_MFPL &~(SYS_GPB_MFPL_PB5MFP_Msk)) | SYS_GPB_MFPL_PB5MFP_PWM0_CH0;
		SYS->GPC_MFPH = (SYS->GPC_MFPH &~(SYS_GPC_MFPH_PC13MFP_Msk))| SYS_GPC_MFPH_PC13MFP_CLKO;
    /* Lock protected registers */
    SYS_LockReg();
}

void I2C0_Init(void) {
    I2C_Open(I2C0, 100000);
}

int main() {
    SYS_Init();
		UART_Open(UART0,921600);
		I2C0_Init();
	
		CLK_Output();
		
		app_main();
}
