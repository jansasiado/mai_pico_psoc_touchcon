/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "tuner.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define CAPSENSE_INTR_PRIORITY          (4u)
#define CY_ASSERT_FAILED                (0u)

/* EZI2C interrupt priority must be higher than CapSense interrupt. */
#define I2C_INTR_PRIORITY             (2u)
#define I2C_INTR_NUM        CYBSP_I2C_IRQ


/* Define the conditions to check sensor status */
#define CYBSP_LED_OFF                   (0u)    
#define CYBSP_LED_ON                    (1u)

/* MPR121 Register Map Indices */
#define I2C_SLAVE_ADDR_START 0x5AU
#define MPR121_REG_TOUCH_STATUS_L   0x00
#define MPR121_REG_TOUCH_STATUS_H   0x01
#define MPR121_REG_CONFIG           0x5E
#define REG_MAP_SIZE                2

/*******************************************************************************
* Global Definitions
*******************************************************************************/
cy_stc_scb_i2c_context_t cybsp_i2c_context;

/* Memory buffer that the I2C hardware will expose to the Master */
uint8_t i2c_read_buffer[REG_MAP_SIZE] = {0};
uint8_t i2c_write_buffer[REG_MAP_SIZE] = {0};

#if CY_CAPSENSE_BIST_EN
/* Variables to hold sensor parasitic capacitances */
uint32_t sensor_cp = 0;
cy_en_capsense_bist_status_t status;
#endif /* CY_CAPSENSE_BIST_EN */


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void initialize_capsense(void);
static void capsense_isr(void);
static void initialize_i2c(void);
void i2c_isr(void);
void initialize_capsense_tuner(void);
void led_control();
void update_i2c_buffer(void);
uint8_t detect_i2c_addr(void);
void handle_i2c_events(uint32_t events);
cy_en_scb_i2c_command_t I2C_SlaveAddressHandler(uint32_t events);

#if CY_CAPSENSE_BIST_EN
static void measure_sensor_cp(void);
#endif /* CY_CAPSENSE_BIST_EN */


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  System entrance point. This function performs
*  - initial setup of device
*  - initialize CapSense
*  - initialize tuner communication
*  - perform Cp measurement if Built-in Self test (BIST) is enabled
*  - scan touch input continuously
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    
    //initialize_capsense();
    
    //initialize_capsense_tuner();
    
    initialize_i2c();
    __enable_irq();
    /* Start the first scan */
    //Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
    //Cy_GPIO_Write(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM, CYBSP_LED_ON);
    for (;;)
    {

        continue;
        if(CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context))
        {
            /* Process all widgets */
            Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

            /* Establishes synchronized communication with the CapSense Tuner tool */
            Cy_CapSense_RunTuner(&cy_capsense_context);

#if CY_CAPSENSE_BIST_EN
            /* Measure the self capacitance of sensor electrode using BIST */
            measure_sensor_cp();
#endif /* CY_CAPSENSE_BIST_EN */

            /* Start the next scan */
            Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
            //Cy_GPIO_Inv(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM);
            //led_control();
            //update_i2c_buffer();
        }
    }
}


/*******************************************************************************
* Function Name: initialize_capsense
********************************************************************************
* Summary:
*  This function initializes the CapSense and configures the CapSense
*  interrupt.
*
*******************************************************************************/
static void initialize_capsense(void)
{
    cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;

    /* CapSense interrupt configuration */
    const cy_stc_sysint_t capsense_interrupt_config =
    {
        .intrSrc = CYBSP_CSD_IRQ,
        .intrPriority = CAPSENSE_INTR_PRIORITY,
    };

    /* Capture the CSD HW block and initialize it to the default state. */
    status = Cy_CapSense_Init(&cy_capsense_context);

    if (CY_CAPSENSE_STATUS_SUCCESS == status)
    {
        /* Initialize CapSense interrupt */
        Cy_SysInt_Init(&capsense_interrupt_config, capsense_isr);
        NVIC_ClearPendingIRQ(capsense_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_interrupt_config.intrSrc);

        /* Initialize the CapSense firmware modules. */
        status = Cy_CapSense_Enable(&cy_capsense_context);
    }

    if(status != CY_CAPSENSE_STATUS_SUCCESS)
    {
        /* This status could fail before tuning the sensors correctly.
         * Ensure that this function passes after the CapSense sensors are tuned
         * as per procedure give in the Readme.md file */
    }
}


/*******************************************************************************
* Function Name: capsense_isr
********************************************************************************
* Summary:
*  Wrapper function for handling interrupts from CapSense block.
*
*******************************************************************************/
static void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}


/*******************************************************************************
* Function Name: initialize_capsense_tuner
********************************************************************************
* Summary:
* - UART module to communicate with the CapSense Tuner tool.
*
*******************************************************************************/
void initialize_capsense_tuner(void)
{
    initialize_uart_tuner();
}


#if CY_CAPSENSE_BIST_EN
/*******************************************************************************
* Function Name: measure_sensor_cp
********************************************************************************
* Summary:
*  Measures the self capacitance of the sensor electrode (Cp) in Femto Farad and
*  stores its value in the variable sensor_cp.
*
*******************************************************************************/
static void measure_sensor_cp(void)
{
    /* Measure the self capacitance of sensor electrode */
    status = Cy_CapSense_MeasureCapacitanceSensor(CY_CAPSENSE_BUTTON0_WDGT_ID,
                                                  CY_CAPSENSE_BUTTON0_SNS0_ID,
                                             &sensor_cp, &cy_capsense_context);
}
#endif /* CY_CAPSENSE_BIST_EN */


/* [] END OF FILE */

void led_control()
{
    if (Cy_CapSense_IsAnyWidgetActive(&cy_capsense_context))
    {
        Cy_GPIO_Write(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM, CYBSP_LED_ON);
    }
    else
    {
        Cy_GPIO_Write(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM, CYBSP_LED_OFF);
    }
}


void update_i2c_buffer(void){
    uint16_t touch_mask = 0;
    for(uint8_t i = 0; i < CY_CAPSENSE_TOTAL_WIDGET_COUNT; i++){
        for(uint8_t j = 0; j < 6; j++){
            if (Cy_CapSense_IsSensorActive(i, j, &cy_capsense_context)) {
                touch_mask |= (1 << (6*i + j));
            }

        }
        //touch_mask |= (cy_capsense_context.ptrWdContext[0].status & CY_CAPSENSE_WD_ACTIVE_MASK) << i;
    }
    // i2c_read_buffer[MPR121_REG_TOUCH_STATUS_L] = (uint8_t)(touch_mask & 0xFF);
    // i2c_read_buffer[MPR121_REG_TOUCH_STATUS_H] = (uint8_t)((touch_mask >> 8) & 0x0F);
}

static void initialize_i2c(void){
    
    /* Configure I2C slave */
    cy_en_scb_i2c_status_t status;
    status = Cy_SCB_I2C_Init(CYBSP_I2C_HW, &CYBSP_I2C_config, &cybsp_i2c_context);
    
    if(status != CY_SCB_I2C_SUCCESS){
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    const cy_stc_sysint_t i2c_isr_config =
    {
        .intrSrc      = I2C_INTR_NUM,
        .intrPriority = I2C_INTR_PRIORITY,
    };

    Cy_SysInt_Init(&i2c_isr_config, &i2c_isr);
    
    Cy_SCB_I2C_SlaveConfigReadBuf(CYBSP_I2C_HW, i2c_read_buffer, REG_MAP_SIZE, &cybsp_i2c_context);
    Cy_SCB_I2C_SlaveConfigWriteBuf(CYBSP_I2C_HW, i2c_write_buffer, REG_MAP_SIZE, &cybsp_i2c_context);   
    
    Cy_SCB_I2C_RegisterEventCallback(CYBSP_I2C_HW, (cy_cb_scb_i2c_handle_events_t) handle_i2c_events, &cybsp_i2c_context);
    Cy_SCB_I2C_RegisterAddrCallback(CYBSP_I2C_HW, I2C_SlaveAddressHandler, &cybsp_i2c_context);


    uint8_t address = detect_i2c_addr();
    Cy_SCB_I2C_SlaveSetAddress(CYBSP_I2C_HW, address);
    
    NVIC_EnableIRQ(i2c_isr_config.intrPriority);
    Cy_SCB_I2C_Enable(CYBSP_I2C_HW, &cybsp_i2c_context);
}   

/* Implement ISR for I2C */
void i2c_isr(void)
{
    Cy_SCB_I2C_SlaveInterrupt(CYBSP_I2C_HW, &cybsp_i2c_context);
}

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

void handle_i2c_events(uint32_t events) {
    /* Check write complete event */
    if (0UL != (CY_SCB_I2C_SLAVE_WR_CMPLT_EVENT & events))
    {
        /* Check for errors */
        if (0UL == (CY_SCB_I2C_SLAVE_ERR_EVENT & events))
        {
            /* Check packet length */
            if (2 == Cy_SCB_I2C_SlaveGetWriteTransferCount(CYBSP_I2C_HW,
                                                                    &cybsp_i2c_context))
            {
                /* Check start and end of packet markers */
            }
        }

        /* Configure write buffer for the next write */
        Cy_SCB_I2C_SlaveConfigWriteBuf(CYBSP_I2C_HW, i2c_write_buffer, REG_MAP_SIZE, &cybsp_i2c_context);
    }

    /* Check read complete event */
    if (0UL != (CY_SCB_I2C_SLAVE_RD_CMPLT_EVENT & events))
    {
        /* Configure read buffer for the next read */
        Cy_SCB_I2C_SlaveConfigReadBuf(CYBSP_I2C_HW, i2c_read_buffer, REG_MAP_SIZE, &cybsp_i2c_context);
    }
}

cy_en_scb_i2c_command_t I2C_SlaveAddressHandler(uint32_t events)
{
    cy_en_scb_i2c_command_t i2cCmd = CY_SCB_I2C_ACK;
    /* Ignore events */
    (void)events;
    /* Set a user flag indicating ACK is pending. */
    
    /* Implement application specific logic here to return ACK/NACK/WAIT command. */
    /* Return i2c command. */
    return i2cCmd;
}