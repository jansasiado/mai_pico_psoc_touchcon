#include "ezi2c.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "board_defs.h"
#include "capsense.h"
#include "tuner.h"
cy_stc_scb_ezi2c_context_t cybsp_ezi2c_context;
uint8_t i2c_buffer[REG_MAP_SIZE];

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

uint8_t initialize_i2c(void){
    
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

    memset(i2c_buffer,0,sizeof(i2c_buffer));
    Cy_SCB_EZI2C_SetBuffer1(CYBSP_I2C_HW, i2c_buffer, REG_MAP_SIZE, REG_MAP_SIZE, &cybsp_ezi2c_context);

    uint8_t i2c_address = detect_i2c_addr();
    Cy_SCB_EZI2C_SetAddress1(CYBSP_I2C_HW, i2c_address, &cybsp_ezi2c_context);
    
    NVIC_EnableIRQ(ezi2c_isr_config.intrSrc);
    Cy_SCB_EZI2C_Enable(CYBSP_I2C_HW);
    return i2c_address;
}  


void ezi2c_isr(void)
{
    Cy_SCB_EZI2C_Interrupt(CYBSP_I2C_HW, &cybsp_ezi2c_context); 
    load_sns_val();
}

void load_sns_val(void){
    if(!i2c_buffer[PSOC_READ_CMD_REG]){return;}
    if(i2c_buffer[PSOC_READ_CMD_REG] & 1<<0){
        //load raw values
        for(uint8_t i = 0; i < 12; i++){
            uint16_t buf = cy_capsense_context.ptrWdConfig[0].ptrSnsContext[i].raw;
            i2c_buffer[PSOC_RAW_VALUE_REG_START+2*i] = (buf>>8)&0x0f;
            i2c_buffer[PSOC_RAW_VALUE_REG_START+2*i+1] = (buf)&0xff;
        }
    }
    if(i2c_buffer[PSOC_READ_CMD_REG] & 1<<1){
        //load baseline values
        for(uint8_t i = 0; i < 12; i++){
            uint16_t buf = cy_capsense_context.ptrWdConfig[0].ptrSnsContext[i].bsln;
            i2c_buffer[PSOC_RAW_VALUE_REG_START+2*i] = (buf>>8)&0x0f;
            i2c_buffer[PSOC_RAW_VALUE_REG_START+2*i+1] = (buf)&0xff;
        }
    }
    if(i2c_buffer[PSOC_READ_CMD_REG] & 1<<2){
        volatile uint8_t i = i2c_buffer[PSOC_IDAC_COMP_N_REG];
        if(i>11){
            return;
        }
        i2c_buffer[PSOC_IDAC_COMP_VAL_REG]=cy_capsense_context.ptrWdConfig[0].ptrSnsContext[i].idacComp;
    }
    i2c_buffer[PSOC_READ_CMD_REG] = 0;
}
void update_i2c_buffer(void){
    uint16_t touch_mask = 0;
    for(uint8_t i = 0; i < CY_CAPSENSE_SENSOR_COUNT; i++){
        touch_mask |= (cy_capsense_context.ptrWdConfig[0].ptrSnsContext[i].status & CY_CAPSENSE_WD_ACTIVE_MASK) << i;
    }
    memcpy(i2c_buffer, &touch_mask, 2);
}

void update_param(){
    if(!i2c_buffer[PSOC_CMD_REG]){return;}
    if(i2c_buffer[PSOC_CMD_REG]&(0b11111110)){
        if(i2c_buffer[PSOC_CMD_REG]&(1<<1)){
            Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_FINGER_TH_PARAM_ID, i2c_buffer[PSOC_FINGER_THRESHOLD_REG+1] | i2c_buffer[PSOC_FINGER_THRESHOLD_REG]<<8,&cy_capsense_tuner, &cy_capsense_context);
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<1);
        }
        if(i2c_buffer[PSOC_CMD_REG]&(1<<2)){
            Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_NOISE_TH_PARAM_ID, i2c_buffer[PSOC_NOISE_THRESHOLD_REG+1] | i2c_buffer[PSOC_NOISE_THRESHOLD_REG]<<8,&cy_capsense_tuner, &cy_capsense_context);
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<2);
        }
        if(i2c_buffer[PSOC_CMD_REG]&(1<<3)){
            Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_NNOISE_TH_PARAM_ID, i2c_buffer[PSOC_NEG_NOISE_THRESHOLD_REG+1] | i2c_buffer[PSOC_NEG_NOISE_THRESHOLD_REG]<<8,&cy_capsense_tuner, &cy_capsense_context);
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<3);
        }
        if(i2c_buffer[PSOC_CMD_REG]&(1<<4)){
            Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_LOW_BSLN_RST_PARAM_ID, i2c_buffer[PSOC_LOW_BASELINE_RESET_REG], &cy_capsense_tuner, &cy_capsense_context);
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<4);
        }
        if(i2c_buffer[PSOC_CMD_REG]&(1<<5)){
            Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_HYSTERESIS_PARAM_ID, i2c_buffer[PSOC_HYSTERESIS_REG], &cy_capsense_tuner, &cy_capsense_context);
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<5);
        }
        if(i2c_buffer[PSOC_CMD_REG]&(1<<6)){
            Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_ON_DEBOUNCE_PARAM_ID, i2c_buffer[PSOC_ON_DEBOUNCE_REG], &cy_capsense_tuner, &cy_capsense_context);
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<6);
        }
        if(i2c_buffer[PSOC_CMD_REG]&(1<<7)){
            switch(i2c_buffer[PSOC_IDAC_COMP_N_REG]){
                case 0:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS0_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 1:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS1_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 2:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS2_IDAC0_PARAM_ID,  i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 3:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS3_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 4:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS4_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 5:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS5_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 6:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS6_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 7:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS7_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 8:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS8_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 9:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS9_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 10:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS10_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                case 11:
                Cy_CapSense_SetParam(CY_CAPSENSE_BUTTON0_SNS11_IDAC0_PARAM_ID, i2c_buffer[PSOC_IDAC_COMP_VAL_REG], &cy_capsense_tuner, &cy_capsense_context);
                break;
                default:
                    return; 
                break;
            }
            i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~(1<<7);
        }
        i2c_buffer[PSOC_CMD_REG] &= ~(1<<1);
    }
}


void check_reset(){ 
    if(i2c_buffer[PSOC_CMD_REG]&1){
        Cy_GPIO_Inv(STATUS1_PORT,STATUS1_NUM);
        Cy_CapSense_CalibrateAllWidgets(&cy_capsense_context);
        Cy_CapSense_InitializeAllBaselines(&cy_capsense_context);
        i2c_buffer[PSOC_CMD_REG] = i2c_buffer[PSOC_CMD_REG] & ~1;
        Cy_GPIO_Inv(STATUS1_PORT,STATUS1_NUM);
    }
}