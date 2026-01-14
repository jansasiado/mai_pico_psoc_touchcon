#include "cy_pdl.h"
#include "cybsp.h"

uint8_t detect_i2c_addr(void);
void ezi2c_isr(void);
void update_i2c_buffer(void);
uint8_t initialize_i2c(void);
void update_param(void);
void load_sns_val(void);
void check_reset(void);