#include "board_defs.h"
#include "cycfg_capsense.h"
// #include "cycfg_peripherals.h"
#include "capsense.h"
extern uint8_t i2c_address;

const cy_stc_sysint_t capsense_interrupt_config =
{
    .intrSrc = CYBSP_CSD_IRQ,
    .intrPriority = CAPSENSE_INTR_PRIORITY,
};

uint8_t initialize_capsense(uint8_t i2c_address)
{
    volatile cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;
    
    
    
    status = Cy_CapSense_Init(&cy_capsense_context);
    if (CY_CAPSENSE_STATUS_SUCCESS == status)
    {
        Cy_SysInt_Init(&capsense_interrupt_config, capsense_isr);
        NVIC_ClearPendingIRQ(capsense_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_interrupt_config.intrSrc);
        status = Cy_CapSense_Enable(&cy_capsense_context);
    }
    if(status != CY_CAPSENSE_STATUS_SUCCESS)
    {
        CY_ASSERT(0);
    }
    if(i2c_address-I2C_SLAVE_ADDR_START==2){
        Cy_CapSense_CSDDisconnectSns(&cy_capsense_context.ptrPinConfig[10],&cy_capsense_context);
        Cy_CapSense_CSDDisconnectSns(&cy_capsense_context.ptrPinConfig[11],&cy_capsense_context);
        return 10;
    }
    else{
        return 12;
    }
}

void deinit_capsense(){
    NVIC_DisableIRQ(capsense_interrupt_config.intrSrc);
    Cy_CapSense_DeInit(&cy_capsense_context);
}

void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}

