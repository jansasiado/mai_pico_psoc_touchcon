#include "cy_pdl.h"
#include "cybsp.h"

/* EZI2C interrupt priority must be higher than CapSense interrupt. */
#define EZI2C_INTR_PRIORITY             (2u)
#define EZI2C_INTR_NUM        CYBSP_I2C_IRQ

#define I2C_SLAVE_ADDR_START 0x5AU
#define REG_MAP_SIZE 2



uint8_t detect_i2c_addr(void);
void ezi2c_isr(void);
void update_i2c_buffer(void);
void initialize_i2c(void);