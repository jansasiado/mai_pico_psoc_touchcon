#include "ezi2c.h"
#include "cycfg.h"
#include "cycfg_capsense.h"

cy_stc_scb_ezi2c_context_t cybsp_ezi2c_context;
uint8_t i2c_buffer[REG_MAP_SIZE];

const uint8_t widget_to_out[2][6] = {
    {0, 2,4, 6, 8, 10},
    {1, 3,5, 7, 9, 11},
};

uint8_t detect_i2c_addr(void){
    Cy_GPIO_Write(ADDR_SEL_PORT,ADDR_SEL_NUM, 1);
    if(Cy_GPIO_Read(ADDR_SEL_PORT,ADDR_SEL_NUM)){
        return I2C_SLAVE_ADDR_START;
    }
    Cy_GPIO_Set(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM);
    if(Cy_GPIO_Read(ADDR_SEL_PORT,ADDR_SEL_NUM)!=0){
        Cy_GPIO_Clr(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM);
        return I2C_SLAVE_ADDR_START + 1;
    }
    Cy_GPIO_Clr(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM);
    return I2C_SLAVE_ADDR_START + 2;
}

void initialize_i2c(void){
    
    cy_en_scb_ezi2c_status_t status;
        status = Cy_SCB_EZI2C_Init(CYBSP_I2C_HW, &CYBSP_I2C_config, &cybsp_ezi2c_context);
    
    if(status != CY_SCB_EZI2C_SUCCESS){
        CY_ASSERT(0);
    }

    const cy_stc_sysint_t ezi2c_isr_config =
    {
        .intrSrc      = EZI2C_INTR_NUM,
        .intrPriority = EZI2C_INTR_PRIORITY,
    };

    Cy_SysInt_Init(&ezi2c_isr_config, &ezi2c_isr);
    
    Cy_SCB_EZI2C_SetBuffer1(CYBSP_I2C_HW, i2c_buffer, REG_MAP_SIZE, 2, &cybsp_ezi2c_context);

    uint8_t address = detect_i2c_addr();
    Cy_SCB_EZI2C_SetAddress1(CYBSP_I2C_HW, address, &cybsp_ezi2c_context);
    
    NVIC_EnableIRQ(ezi2c_isr_config.intrSrc);
    Cy_SCB_EZI2C_Enable(CYBSP_I2C_HW);
}  


void ezi2c_isr(void)
{
    Cy_SCB_EZI2C_Interrupt(CYBSP_I2C_HW, &cybsp_ezi2c_context);
}

void update_i2c_buffer(void){
    uint16_t touch_mask = 0;
    for(uint8_t i = 0; i < CY_CAPSENSE_TOTAL_WIDGET_COUNT; i++){
        for(uint8_t j = 0; j < 6; j++){
            if (Cy_CapSense_IsSensorActive(i, j, &cy_capsense_context)) {
                touch_mask |= (1 << widget_to_out[i][j]);
            }
        }
        //touch_mask |= (cy_capsense_context.ptrWdContext[0].status & CY_CAPSENSE_WD_ACTIVE_MASK) << i;
    }
    i2c_buffer[0] = (uint8_t)(touch_mask & 0xFF);
    i2c_buffer[1] = (uint8_t)((touch_mask >> 8) & 0x0F);
}