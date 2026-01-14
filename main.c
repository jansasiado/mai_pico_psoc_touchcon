/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "capsense.h"
#include "tuner.h"
#include "ezi2c.h"
#include "board_defs.h"
/*******************************************************************************
* Macros
*******************************************************************************/

/*******************************************************************************
* Global Definitions
*******************************************************************************/

#if CY_CAPSENSE_BIST_EN
/* Variables to hold sensor parasitic capacitances */
uint32_t sensor_cp = 0;
cy_en_capsense_bist_status_t status;
#endif /* CY_CAPSENSE_BIST_EN */
bool scan = 0;
/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void SysTick_Callback();

#if CY_CAPSENSE_BIST_EN
static void measure_sensor_cp(void);
#endif /* CY_CAPSENSE_BIST_EN */

int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }
    __enable_irq();
    uint8_t i2c_address = initialize_i2c();
    uint8_t numSns = initialize_capsense(i2c_address);
    bool tuner_en = initialize_capsense_tuner();
    Cy_SysTick_SetCallback(0UL, &SysTick_Callback);
    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
    uint8_t s0 = 0, s1= 0;
    for (;;)
    {       
        if(CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context))
        {
            check_reset();
            s0 = s1;
            s1++;
            if (s1 == numSns)
            {
                s1 = 0u;
            }
            Cy_CapSense_ScanSensor(CY_CAPSENSE_BUTTON0_WDGT_ID, s1, &cy_capsense_context);
            Cy_CapSense_ProcessSensorExt(CY_CAPSENSE_BUTTON0_WDGT_ID, s0, CY_CAPSENSE_PROCESS_ALL, &cy_capsense_context);
            s0++;
            if (s0 == numSns)
            {
                while(!scan){
                    Cy_SysLib_DelayUs(1);
                }
                Cy_CapSense_ProcessWidgetExt(CY_CAPSENSE_BUTTON0_WDGT_ID, CY_CAPSENSE_PROCESS_STATUS, &cy_capsense_context);
                update_i2c_buffer();
                update_param();
                if(tuner_en){
                    Cy_CapSense_RunTuner(&cy_capsense_context);
                }
                else{
                    Cy_GPIO_Inv(CYBSP_STATUS_LED_PORT, CYBSP_STATUS_LED_NUM);
                }
            }
            scan = 0;
        }
    }
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