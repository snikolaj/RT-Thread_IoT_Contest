/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2021-01-28     flybreak       first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdbool.h>

#include "hardware/adc.h"
#include "DS1302/DS1302.h"

#define LED_PIN 25

#define ESP_UART_NAME "uart1"
#define ESP_UART_BAUDRATE 115200

#define ADC_PIN 27
#define ADC_REFERENCE_VOLTAGE 3.3f
#define ADC_BITS 12
#define ADC_RADAR_THRESHOLD 2.0f
#define ADC_RADAR_READING ((int)((float)(1 << ADC_BITS) * (ADC_RADAR_THRESHOLD / ADC_REFERENCE_VOLTAGE)))

// time during which the LED can be active (in hours, 24 hour format)
#define TIME_START 20
#define TIME_END 22

// how long the LED will be on in minutes
#define ILLUMINATION_DURATION 90

bool is_person_present = false;
bool is_led_on = false;
bool is_cycle_finished = false;

uint32_t led_seconds = 0;

static rt_device_t esp_serial;
struct serial_configure esp_config = RT_SERIAL_CONFIG_DEFAULT;

struct rtc_time ds1302;
struct rtc_time *rtc;

void initializeADC(){
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(1);
}

rt_err_t initializeUART(){
    rt_err_t current_error;
    esp_serial = rt_device_find(ESP_UART_NAME);
    current_error = rt_device_open(esp_serial, RT_DEVICE_FLAG_STREAM);
    if(current_error != RT_EOK){
        return current_error;
    }

    current_error = rt_device_control(esp_serial, RT_DEVICE_CTRL_CONFIG, &esp_config);
    if(current_error != RT_EOK){
        return current_error;
    }
}

void isPersonPresent(){
    if(adc_read() > ADC_RADAR_READING){
        is_person_present = true;
    }else{
        is_person_present = false;
    }
}

void isPersonPresentDebug(){
    if(is_person_present){
        rt_kprintf("A person is present\n");
    }else{
        rt_kprintf("A person is not present\n");
    }
}

bool isTime(){
    uint8_t hour = rtc->hour;

    if(TIME_START > TIME_END){
        if((TIME_START < hour) && ((TIME_END + 24) > hour)){
            return true;
        }
    }else{
        if((TIME_START < hour) && (TIME_END > hour)){
            return true;
        }
    }
    is_cycle_finished = false;
    return false;
}

void printTime(){
    rt_kprintf("Is it time: "); rt_kprintf(((isTime) ? "yes" : "no"));
    rt_kprintf("\nThe current time is: %d\n", rtc->hour);
}

void activateUVC(){
    char command = 't';
    if(!is_person_present){
        rt_device_write(esp_serial, 0, &command, 1);
        is_led_on = true;
    }
}

void deactivateUVC(){
    char command = 'f';
    rt_device_write(esp_serial, 0, &command, 1);
    is_led_on = false;
}

int main(void)
{
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    rt_kprintf("Hello, RT-Thread!\n");

    initializeADC();
    initializeUART();

    rtc = &ds1302;
    ds1302_init();

    while (1)
    {
        rt_thread_mdelay(1000);

        isPersonPresent();

        ds1302_update_time(rtc, HOUR);

        if(is_led_on){
            led_seconds++;
            rt_pin_write(LED_PIN, 1);
        }else{
            rt_pin_write(LED_PIN, 0);
        }

        if(led_seconds / 60 > ILLUMINATION_DURATION){
            deactivateUVC();
            led_seconds = 0;
            is_cycle_finished = true;
        }

        if(is_person_present){
            deactivateUVC();
        }

        if(isTime() && ((led_seconds / 60) < ILLUMINATION_DURATION) && !is_led_on && !is_cycle_finished){
            activateUVC();
        }
    }
}

MSH_CMD_EXPORT(activateUVC, activate LED);
MSH_CMD_EXPORT(deactivateUVC, deactivate LED);
MSH_CMD_EXPORT(printTime, checks time);
MSH_CMD_EXPORT(isPersonPresentDebug, is person present);
