/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "tuner.h"
#include "ezi2c.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define CAPSENSE_INTR_PRIORITY          (4u)
#define CY_ASSERT_FAILED                (0u)

/*******************************************************************************
* Global Definitions
*******************************************************************************/

#if CY_CAPSENSE_BIST_EN
/* Variables to hold sensor parasitic capacitances */
uint32_t sensor_cp = 0;
cy_en_capsense_bist_status_t status;
#endif /* CY_CAPSENSE_BIST_EN */
extern uint8_t i2c_buffer[REG_MAP_SIZE];

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void initialize_capsense(void);
static void capsense_isr(void);
void initialize_capsense_tuner(void);
void SysTick_Callback();
bool scan = 0, tuner_en = 1;

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
    i2c_buffer[0] = 0;
    i2c_buffer[1] = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    __enable_irq();
    initialize_capsense();

    initialize_capsense_tuner();
    initialize_i2c();


    //Cy_SysTick_SetCallback(0UL, &SysTick_Callback);

    /* Start the first scan */
    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
    uint8_t s0 = 0, s1= 0;
    volatile uint32_t test = Cy_SysTick_GetValue();
    for (;;)
    {       
        if(1){
            if(CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context))
            {
                /* Store the current sensor index */
                s0 = s1;
                /* Go to the next sensor */
                s1++;
                if (s1 == cy_capsense_context.ptrWdConfig[CY_CAPSENSE_BUTTON0_WDGT_ID].numSns)
                {
                    /* Reset sensor index to start from the first sensor */
                    s1 = 0u;
                }
                /* The next sensor scan start */
                Cy_CapSense_ScanSensor(CY_CAPSENSE_BUTTON0_WDGT_ID, s1, &cy_capsense_context);
                /* Process the previous sensor while scanning the next one */
                Cy_CapSense_ProcessSensorExt(CY_CAPSENSE_BUTTON0_WDGT_ID, s0, CY_CAPSENSE_PROCESS_ALL, &cy_capsense_context);
                s0++;
                if (s0 == cy_capsense_context.ptrWdConfig[CY_CAPSENSE_BUTTON0_WDGT_ID].numSns)
                {
                    /* All widget sensors are processed, therefore process only widget-related task */
                    Cy_CapSense_ProcessWidgetExt(CY_CAPSENSE_BUTTON0_WDGT_ID, CY_CAPSENSE_PROCESS_STATUS, &cy_capsense_context);
                    if(tuner_en){
                        Cy_CapSense_RunTuner(&cy_capsense_context);
                    }
                    else{
                        Cy_GPIO_Inv(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM);
                    }
                    test = Cy_SysTick_GetValue();
                    if(test > 40000){
                        test = 0;
                    }
                }
                scan = 0;
                // Cy_CapSense_ProcessWidgets(&cy_capsense_context);
                //led_control();
                //Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
                // Cy_CapSense_SetupWidgetExt(0, s, &cy_capsense_context);
                // Cy_CapSense_ScanExt(&cy_capsense_context);
            }
            else{
                update_i2c_buffer();
            }
        }
    }
}


static void initialize_capsense(void)
{
    cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;
    const cy_stc_sysint_t capsense_interrupt_config =
    {
        .intrSrc = CYBSP_CSD_IRQ,
        .intrPriority = CAPSENSE_INTR_PRIORITY,
    };
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
}

static void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}

void initialize_capsense_tuner(void)
{
    Cy_GPIO_Write(TUNER_EN_PORT,TUNER_EN_NUM, 1);
    if(Cy_GPIO_Read(TUNER_EN_PORT, TUNER_EN_NUM)){
        tuner_en = 0;
        return;
    }
    
    initialize_uart_tuner();
}

void SysTick_Callback(){
    scan = 1;
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