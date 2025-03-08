#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "driver/pwm.h"
#include "driver/adc.h"
#include "driver/uart.h"
#include "freertos/queue.h"


#define PWM_OUTPUT  2
#define BUF_SIZE    1024



// PWM period 1000us(1Khz), same as depth
#define PWM_PERIOD    (1000)


QueueHandle_t serial_data_queue;
static uint32_t serial_value = 10;

// pwm pin number
const uint32_t pin_num[1] = {
    PWM_OUTPUT
};

// duties table, real_duty = duties[x]/PERIOD
static uint32_t duties[1] = {
    500
};

// phase table, delay = (phase[x]/360)*PERIOD
float phase[1] = {
    0
};

// map serial value to pwm duty
uint32_t map_serial_to_pwm(uint32_t * serial_value)
{
    uint32_t duty = (1024 * (*serial_value) ) / 100;
    return duty;
}


void adc_Reader(void *pvParameter)
{

    while(1)
    {
        xQueuePeek(serial_data_queue, &serial_value, 0);
        duties[0] = map_serial_to_pwm(&serial_value);
        pwm_set_duty( 0, duties[0]);
        pwm_start();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

void uart_connection(void *pvParameter)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while(1)
    {
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if(len>0)
        {   
            uint32_t value = 0;
            for(int i = 0; i < len; i++)
            {
                if (data[i] >= '0' && data[i] <= '9')  // Convert ASCII to number
                {
                    value = value * 10 + (data[i] - '0');
                }
            }
            if(value <= 100)
            {
                xQueueOverwrite(serial_data_queue, &value);
            }
            else
            {
                printf("value must be between 0 and 100\n");
            }
            // uart_write_bytes(UART_NUM_0, (const char *) data, len);
        }
        
    }
}


void app_main()
{
    // Configure UART
    uart_config_t uart_config;

    uart_config.baud_rate = 9600;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);

    // Configure ADC
    adc_config_t adc_config;

    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8;

    adc_init(&adc_config);

    // Configure PWM
    pwm_init(PWM_PERIOD, duties, 1, pin_num);
    pwm_set_phases(phase);
    pwm_start();
    
    // Create queue
    serial_data_queue = xQueueCreate(1, sizeof(uint32_t));
    xQueueOverwrite(serial_data_queue, &serial_value);

    // Create tasks
    xTaskCreate(adc_Reader, "PWM_task", 1024 * 4, NULL, 3, NULL);
    xTaskCreate(uart_connection, "uart_connection", 1024, NULL, 2, NULL);
}