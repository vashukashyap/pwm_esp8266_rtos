#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "driver/pwm.h"
#include "driver/adc.h"


#define PWM_3_OUT_IO_NUM   2

// PWM period 1000us(1Khz), same as depth
#define PWM_PERIOD    (1000)


// pwm pin number
const uint32_t pin_num[1] = {
    PWM_3_OUT_IO_NUM
};

// duties table, real_duty = duties[x]/PERIOD
static uint32_t duties[1] = {
    500
};


uint32_t map_adc_to_pwm(uint16_t adc_value)
{
    uint32_t duty = ( adc_value * 1000) / 1024;
    return duty;
}


// phase table, delay = (phase[x]/360)*PERIOD
float phase[1] = {
    0
};


void adc_Reader(void *pvParameter)
{

    uint16_t adc_value = 10;

    while(1)
    {
        if(ESP_OK == adc_read(&adc_value))
        {
            duties[0] = (uint32_t) map_adc_to_pwm(adc_value);
            printf("ADC Value: %d ==> DUTYCYCLE: %d\n", adc_value, duties[0]);
            pwm_set_duty( 0, duties[0]);
            pwm_start();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

}


void app_main()
{

    adc_config_t adc_config;

    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8;

    adc_init(&adc_config);

    pwm_init(PWM_PERIOD, duties, 1, pin_num);
    pwm_set_phases(phase);
    pwm_start();    

    xTaskCreate(adc_Reader, "PWM_task", 1024 * 4, NULL, 2, NULL);
}