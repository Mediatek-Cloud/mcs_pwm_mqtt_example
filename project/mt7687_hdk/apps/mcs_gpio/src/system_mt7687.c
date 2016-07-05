/*
** $Id: //MT7687 $
*/

/*! \file   "system_mt7687.c"
    \brief  This file provide utility functions for the driver

*/



#include <stdint.h>
#include "mt7687.h"
#include "system_mt7687.h"
#include "mt7637_cm4_hw_memmap.h"
#include "exception_mt7687.h"


/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

static uint32_t gXtalFreq;
static uint32_t gCpuFrequency;
uint32_t SystemCoreClock = CPU_FREQUENCY;

static uint32_t SysTick_Set(uint32_t ticks)
{
    uint32_t val;

    if ((ticks - 1) > SysTick_LOAD_RELOAD_Msk) {
        return (1);    /* Reload value impossible */
    }

    val = SysTick->CTRL;                                   /* backup CTRL register */

    SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk |          /* disable sys_tick */
                       SysTick_CTRL_ENABLE_Msk);

    SysTick->LOAD  = ticks - 1;                            /* set reload register */
    SysTick->VAL   = 0;                                    /* Load the SysTick Counter Value */

    SysTick->CTRL = val;                                   /* restore CTRL register */

    return (0);                                            /* Function successful */
}

/*----------------------------------------------------------------------------
  Clock functions
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate(void)             /* Get Core Clock Frequency      */
{
    SystemCoreClock = top_mcu_freq_get();
}

/* Determine clock frequency according to clock register values             */


/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System.
 */
void SystemInit(void)
{
    SCB->VTOR  = NVIC_RAM_VECTOR_ADDRESS;
    SystemCoreClockUpdate();
}

#define CFG_FPGA 0
void top_xtal_init(void)
{
    uint32_t  u4RegVal = 0;
    unsigned long reg = HAL_REG_32(TOP_AON_CM4_STRAP_STA);
    reg = (reg >> 13) & 0x00000007;

#if (CFG_FPGA == 0)
    u4RegVal = HAL_REG_32(TOP_AON_CM4_PWRCTLCR);
    u4RegVal &= (~(CM4_PWRCTLCR_CM4_XTAL_FREQ_MASK));

    switch (reg) {
        case 0:
            gXtalFreq = 20000000;  /* 20Mhz */
            u4RegVal |= BIT(CM4_PWRCTLCR_CM4_XTAL_FREQ_20M_OFFSET);
            break;
        case 1:
            gXtalFreq = 40000000;  /* 40Mhz */
            u4RegVal |= BIT(CM4_PWRCTLCR_CM4_XTAL_FREQ_40M_OFFSET);
            break;
        case 2:
            gXtalFreq = 26000000;  /* 26Mhz */
            u4RegVal |= BIT(CM4_PWRCTLCR_CM4_XTAL_FREQ_26M_OFFSET);
            break;
        case 3:
            gXtalFreq = 52000000;  /* 52Mhz */
            u4RegVal |= BIT(CM4_PWRCTLCR_CM4_XTAL_FREQ_52M_OFFSET);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            gXtalFreq = 40000000;  /* fall through */
            u4RegVal |= BIT(CM4_PWRCTLCR_CM4_XTAL_FREQ_40M_OFFSET);
            break;
    }

#else
    gXtalFreq = 30000000;
    u4RegVal |= BIT(CM4_PWRCTLCR_CM4_XTAL_FREQ_40M_OFFSET);
#endif
    HAL_REG_32(TOP_AON_CM4_PWRCTLCR) = u4RegVal;
    gCpuFrequency = gXtalFreq;
    SystemCoreClockUpdate();
    SysTick_Set(SystemCoreClock / 1000); /* 1ms trigger */
}

uint32_t top_xtal_freq_get(void)
{
    return gXtalFreq;
}

uint32_t top_mcu_freq_get(void)
{
    return gCpuFrequency;
}

void cmnPLL1ON(void)
{
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4PWRCtl = (volatile uint32_t *)TOP_CFG_CM4_PWR_CTL_CR;

    reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
    reg = (reg >>  CM4_MPLL_EN_SHIFT) & CM4_MPLL_EN_MASK;

    if (reg == CM4_MPLL_EN_PLL1_OFF_PLL2_OFF) {
        reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
        reg = reg & ~(CM4_NEED_RESTORE_MASK <<  CM4_NEED_RESTORE_SHIFT);    // avoid W1C
        reg = reg | (CM4_MPLL_EN_PLL1_ON_PLL2_OFF << CM4_MPLL_EN_SHIFT);    // Or only, so PLL2 setting won't be cleared
        cmnWriteRegister32(pTopCfgCM4PWRCtl, reg);

        do {
            reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
            reg = reg & (CM4_BT_PLL_RDY_MASK << CM4_BT_PLL_RDY_SHIFT);
        } while (!reg);
    }
    return;
}

void cmnPLL1ON_PLL2ON(uint8_t fg960M)
{
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4PWRCtl = (volatile uint32_t *)TOP_CFG_CM4_PWR_CTL_CR;

    reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
    reg = (reg >>  CM4_MPLL_EN_SHIFT) & CM4_MPLL_EN_MASK;

    if (reg != CM4_MPLL_EN_PLL1_ON_PLL2_ON) {
        reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
        reg = reg & ~(CM4_MCU_960_EN_MASK << CM4_MCU_960_EN_SHIFT);
        reg = reg & ~(CM4_NEED_RESTORE_MASK <<  CM4_NEED_RESTORE_SHIFT);    // avoid W1C
        reg = reg | (CM4_MCU_960_EN_DISABLE << CM4_MCU_960_EN_SHIFT);
        cmnWriteRegister32(pTopCfgCM4PWRCtl, reg);

        reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
        reg = reg & ~(CM4_MPLL_EN_MASK << CM4_MPLL_EN_SHIFT);
        reg = reg & ~(CM4_NEED_RESTORE_MASK <<  CM4_NEED_RESTORE_SHIFT);    // avoid W1C
        reg = reg | (CM4_MPLL_EN_PLL1_ON_PLL2_ON << CM4_MPLL_EN_SHIFT);
        cmnWriteRegister32(pTopCfgCM4PWRCtl, reg);

        do {
            reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
            reg = reg & (CM4_WF_PLL_RDY_MASK << CM4_WF_PLL_RDY_SHIFT);
        } while (!reg);

        reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
        reg = reg & ~(CM4_MCU_960_EN_MASK << CM4_MCU_960_EN_SHIFT);
        reg = reg & ~(CM4_NEED_RESTORE_MASK <<  CM4_NEED_RESTORE_SHIFT);    // avoid W1C
    }

    if (fg960M) {
        reg = reg | (CM4_MCU_960_EN_ENABLE << CM4_MCU_960_EN_SHIFT);
    } else {
        reg = reg | (CM4_MCU_960_EN_DISABLE << CM4_MCU_960_EN_SHIFT);
    }
    cmnWriteRegister32(pTopCfgCM4PWRCtl, reg);
    return;
}


void cmnPLL1OFF_PLL2OFF(void)
{
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4PWRCtl = (volatile uint32_t *)TOP_CFG_CM4_PWR_CTL_CR;

    reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
    reg = reg & ~(CM4_MPLL_EN_MASK << CM4_MPLL_EN_SHIFT);
    reg = reg & ~(CM4_NEED_RESTORE_MASK <<  CM4_NEED_RESTORE_SHIFT);    // avoid W1C
    reg = reg | (CM4_MPLL_EN_PLL1_OFF_PLL2_OFF << CM4_MPLL_EN_SHIFT);
    cmnWriteRegister32(pTopCfgCM4PWRCtl, reg);

    reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
    reg = reg & ~(CM4_MCU_960_EN_MASK << CM4_MCU_960_EN_SHIFT);
    reg = reg & ~(CM4_NEED_RESTORE_MASK <<  CM4_NEED_RESTORE_SHIFT);    // avoid W1C
    reg = reg | (CM4_MCU_960_EN_DISABLE << CM4_MCU_960_EN_SHIFT);
    cmnWriteRegister32(pTopCfgCM4PWRCtl, reg);

    return;
}

void cmnCpuClkConfigureToXtal(void)
{
    TOP_CFG_AON_BondingAndStrap bs;
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4CKG = (volatile uint32_t *)TOP_CFG_CM4_CKG_EN0;
    volatile uint32_t *pBS = (volatile uint32_t *)TOP_CFG_CM4_CM4_STRAP_STA;

    // Step1. CM4_HCLK_SW set to XTAL
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_HCLK_SEL_MASK << CM4_HCLK_SEL_SHIFT);
    reg = reg | (CM4_HCLK_SEL_OSC << CM4_HCLK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    // Step2. CM4_RF_CLK_SW set to XTAL
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_WBTAC_MCU_CK_SEL_MASK << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    reg = reg | (CM4_WBTAC_MCU_CK_SEL_XTAL << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    while (reg != cmnReadRegister32(pTopCfgCM4CKG));


    // Get frequency of Xtal/hclk

    bs.AsUint32 = cmnReadRegister32(pBS);

    switch (bs.XTAL_FREQ) {
        case 0:
            gXtalFreq = 20000000;  /* 20Mhz */
            break;
        case 1:
            gXtalFreq = 40000000;  /* 40Mhz */
            break;
        case 2:
            gXtalFreq = 26000000;  /* 26Mhz */
            break;
        case 3:
            gXtalFreq = 52000000;  /* 52Mhz */
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            gXtalFreq = 40000000;  /* fall through */
            break;
    }
    gCpuFrequency = gXtalFreq;
    SystemCoreClockUpdate();
    SysTick_Set(SystemCoreClock / 1000); /* 1ms trigger */
    return;
}



void cmnCpuClkConfigureTo192M(void)
{
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4CKG = (volatile uint32_t *)TOP_CFG_CM4_CKG_EN0;

    // Step1. Power on PLL1 & 2
    cmnPLL1ON_PLL2ON(TRUE);

    // Step2. CM4_RF_CLK_SW set to PLL2(960)
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_WBTAC_MCU_CK_SEL_MASK << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    reg = reg | (CM4_WBTAC_MCU_CK_SEL_WIFI_PLL_960 << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    while (reg != cmnReadRegister32(pTopCfgCM4CKG));

    // Step3. set divider to 1+8/2=5, ->  960/5=192Mhz
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_MCU_DIV_SEL_MASK << CM4_MCU_DIV_SEL_SHIFT);
    reg = reg | (8 << CM4_MCU_DIV_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    // Step4. CM4_HCLK_SW set to PLL_CK
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_HCLK_SEL_MASK << CM4_HCLK_SEL_SHIFT);
    reg = reg | (CM4_HCLK_SEL_PLL << CM4_HCLK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);
    gCpuFrequency = MCU_FREQUENCY_192MHZ;
    SystemCoreClockUpdate();
    SysTick_Set(SystemCoreClock / 1000); /* 1ms trigger */
    return;
}

//
//  Configure CM4 MCU to 160Mhz
//

void cmnCpuClkConfigureTo160M(void)
{
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4CKG = (volatile uint32_t *)TOP_CFG_CM4_CKG_EN0;

    // Step1. Power on PLL1 & 2
    cmnPLL1ON_PLL2ON(FALSE);
#if 0
    volatile uint32_t *pTopCfgCM4PWRCtl = (volatile uint32_t *)TOP_CFG_CM4_PWR_CTL_CR;
    reg = cmnReadRegister32(pTopCfgCM4PWRCtl);
    reg = reg | (CM4_MCU_960_EN_DISABLE << CM4_MCU_960_EN_SHIFT);
#endif
    // Step2. CM4_RF_CLK_SW set to PLL2(320)
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_WBTAC_MCU_CK_SEL_MASK << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    reg = reg | (CM4_WBTAC_MCU_CK_SEL_WIFI_PLL_320 << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    while (reg != cmnReadRegister32(pTopCfgCM4CKG));

    // Step3. set divider to 1+2/2=2, ->  320/2=160Mhz
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_MCU_DIV_SEL_MASK << CM4_MCU_DIV_SEL_SHIFT);
    reg = reg | (2 << CM4_MCU_DIV_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    // Step4. CM4_HCLK_SW set to PLL_CK
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_HCLK_SEL_MASK << CM4_HCLK_SEL_SHIFT);
    reg = reg | (CM4_HCLK_SEL_PLL << CM4_HCLK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);
    gCpuFrequency = MCU_FREQUENCY_160MHZ;
    SystemCoreClockUpdate();
    SysTick_Set(SystemCoreClock / 1000); /* 1ms trigger */
    return;
}


//
//  Configure CM4 MCU to 64Mhz
//
void cmnCpuClkConfigureTo64M(void)
{
    volatile uint32_t reg;
    volatile uint32_t *pTopCfgCM4CKG = (volatile uint32_t *)TOP_CFG_CM4_CKG_EN0;

    // Step1. Power on PLL1
    cmnPLL1ON();

    // Step2. CM4_RF_CLK_SW set to XTAL
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_WBTAC_MCU_CK_SEL_MASK << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    reg = reg | (CM4_WBTAC_MCU_CK_SEL_XTAL << CM4_WBTAC_MCU_CK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);

    while (reg != cmnReadRegister32(pTopCfgCM4CKG));

    // Step3. CM4_HCLK_SW set to SYS_64M
    reg = cmnReadRegister32(pTopCfgCM4CKG);
    reg = reg & ~(CM4_HCLK_SEL_MASK << CM4_HCLK_SEL_SHIFT);
    reg = reg | (CM4_HCLK_SEL_SYS_64M << CM4_HCLK_SEL_SHIFT);
    cmnWriteRegister32(pTopCfgCM4CKG, reg);
    gCpuFrequency = MCU_FREQUENCY_64MHZ;
    SystemCoreClockUpdate();
    SysTick_Set(SystemCoreClock / 1000); /* 1ms trigger */
    return;
}


