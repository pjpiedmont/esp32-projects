// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "sdkconfig.h"
#include "bootloader_random.h"
#include "esp_log.h"
#include "soc/syscon_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/apb_saradc_reg.h"
#include "soc/system_reg.h"
#include "regi2c_ctrl.h"

void bootloader_random_enable(void)
{
    /* RNG module is always clock enabled */
    REG_SET_FIELD(RTC_CNTL_SENSOR_CTRL_REG, RTC_CNTL_FORCE_XPD_SAR, 0x3);
    SET_PERI_REG_MASK(RTC_CNTL_ANA_CONF_REG, RTC_CNTL_SAR_I2C_PU_M);

    // Bridging sar2 internal reference voltage
    REGI2C_WRITE_MASK(I2C_SAR_ADC, ADC_SARADC2_ENCAL_REF_ADDR, 1);
    REGI2C_WRITE_MASK(I2C_SAR_ADC, ADC_SARADC_DTEST_RTC_ADDR, 0);
    REGI2C_WRITE_MASK(I2C_SAR_ADC, ADC_SARADC_ENT_RTC_ADDR, 0);
    REGI2C_WRITE_MASK(I2C_SAR_ADC, ADC_SARADC_ENT_TSENS_ADDR, 0);

    // Enable SAR ADC2 internal channel to read adc2 ref voltage for additional entropy
    SET_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_APB_SARADC_CLK_EN_M);
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_APB_SARADC_RST_M);
    REG_SET_FIELD(APB_SARADC_APB_ADC_CLKM_CONF_REG, APB_SARADC_CLK_SEL, 0x2);
    SET_PERI_REG_MASK(APB_SARADC_APB_ADC_CLKM_CONF_REG, APB_SARADC_CLK_EN_M);
    SET_PERI_REG_MASK(APB_SARADC_CTRL_REG, APB_SARADC_SAR_CLK_GATED_M);
    REG_SET_FIELD(APB_SARADC_CTRL_REG, APB_SARADC_XPD_SAR_FORCE, 0x3);
    REG_SET_FIELD(APB_SARADC_CTRL_REG, APB_SARADC_SAR_CLK_DIV, 1);

    REG_SET_FIELD(APB_SARADC_FSM_WAIT_REG, APB_SARADC_RSTB_WAIT, 8);
    REG_SET_FIELD(APB_SARADC_FSM_WAIT_REG, APB_SARADC_XPD_WAIT, 5);
    REG_SET_FIELD(APB_SARADC_FSM_WAIT_REG, APB_SARADC_STANDBY_WAIT, 100);

    SET_PERI_REG_MASK(APB_SARADC_CTRL_REG, APB_SARADC_SAR_PATT_P_CLEAR_M);
    CLEAR_PERI_REG_MASK(APB_SARADC_CTRL_REG, APB_SARADC_SAR_PATT_P_CLEAR_M);
    REG_SET_FIELD(APB_SARADC_CTRL_REG, APB_SARADC_SAR_PATT_LEN, 0);
    REG_SET_FIELD(APB_SARADC_SAR_PATT_TAB1_REG, APB_SARADC_SAR_PATT_TAB1, 0x9cffff);// Set adc2 internal channel & atten
    REG_SET_FIELD(APB_SARADC_SAR_PATT_TAB2_REG, APB_SARADC_SAR_PATT_TAB2, 0xffffff);
    // Set ADC sampling frequency
    REG_SET_FIELD(APB_SARADC_CTRL2_REG, APB_SARADC_TIMER_TARGET, 100);
    REG_SET_FIELD(APB_SARADC_APB_ADC_CLKM_CONF_REG, APB_SARADC_CLKM_DIV_NUM, 15);
    CLEAR_PERI_REG_MASK(APB_SARADC_CTRL2_REG,APB_SARADC_MEAS_NUM_LIMIT);
    SET_PERI_REG_MASK(APB_SARADC_DMA_CONF_REG, APB_SARADC_APB_ADC_TRANS_M);
    SET_PERI_REG_MASK(APB_SARADC_CTRL2_REG,APB_SARADC_TIMER_EN);
}

void bootloader_random_disable(void)
{
    /* Restore internal I2C bus state */
    REGI2C_WRITE_MASK(I2C_SAR_ADC, ADC_SARADC2_ENCAL_REF_ADDR, 0);

    /* Restore SARADC to default mode */
    CLEAR_PERI_REG_MASK(APB_SARADC_CTRL2_REG,APB_SARADC_TIMER_EN);
    CLEAR_PERI_REG_MASK(APB_SARADC_DMA_CONF_REG, APB_SARADC_APB_ADC_TRANS_M);
    REG_SET_FIELD(APB_SARADC_SAR_PATT_TAB1_REG, APB_SARADC_SAR_PATT_TAB1, 0xffffff);
    REG_SET_FIELD(APB_SARADC_SAR_PATT_TAB2_REG, APB_SARADC_SAR_PATT_TAB2, 0xffffff);
    CLEAR_PERI_REG_MASK(APB_SARADC_APB_ADC_CLKM_CONF_REG, APB_SARADC_CLK_EN_M);
    REG_SET_FIELD(APB_SARADC_CTRL_REG, APB_SARADC_XPD_SAR_FORCE, 0);
    REG_SET_FIELD(RTC_CNTL_SENSOR_CTRL_REG, RTC_CNTL_FORCE_XPD_SAR, 0);
}
