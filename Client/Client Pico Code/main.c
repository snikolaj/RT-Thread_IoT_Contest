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


static struct rt_timer buzzer_timer;
static struct rt_timer indicator_timer;

#define INDICATOR_LED_PIN 22
#define BUZZER_PIN 20
#define ADC_PIN 27
#define UVC_LED_PIN 21

#define ADC_REFERENCE_VOLTAGE 3.3f
#define ADC_BITS 12
#define ADC_OVERCURRENT_VOLTAGE_LIMIT 1.8f
#define ADC_RAW_OVERCURRENT_MAX ((int)((float)(1 << ADC_BITS) * (ADC_OVERCURRENT_VOLTAGE_LIMIT / ADC_REFERENCE_VOLTAGE)))

bool is_buzzer_on = false;
bool is_indicator_on = false;
bool is_overcurrent = false;

uint32_t buzzer_time_on = 1;
uint32_t indicator_time_on = 1000;

static void buzzerOn(){
    if(!is_buzzer_on){
        rt_pin_write(BUZZER_PIN, 1);
        is_buzzer_on = true;
    }else{
        rt_pin_write(BUZZER_PIN, 0);
        is_buzzer_on = false;
    }
}

static void indicatorOn(){
    if(!is_indicator_on){
        rt_pin_write(INDICATOR_LED_PIN, 1);
        is_indicator_on = true;
    }else{
        rt_pin_write(INDICATOR_LED_PIN, 0);
        is_indicator_on = false;
    }
}

static void indicatorPanic(){
    rt_pin_write(INDICATOR_LED_PIN, 1);
    is_indicator_on = true;
}


static void LEDOn(){
    rt_pin_write(UVC_LED_PIN, 1);
    rt_timer_start(&indicator_timer);
    rt_timer_start(&buzzer_timer);
}

static void LEDOff(){
    rt_pin_write(UVC_LED_PIN, 0);
    rt_timer_stop(&indicator_timer);
    rt_timer_stop(&buzzer_timer);
}

static void detectOvercurrent(){
    if(adc_read() > ADC_RAW_OVERCURRENT_MAX){
        rt_kprintf("OVERCURRENT DETECTED: %i", adc_read());
        LEDOff();
        indicatorPanic();
    }
}

static void LEDControl(int argc, char **argv){
    if(is_overcurrent){
        rt_kprintf("Over-current detected. UV LED disabled for safety.\n");
        return;
    }

    if(argc < 2){
        rt_kprintf("Too few variables. Write 'LEDControl on' or 'LEDControl off' to turn the LED on or off.\n");
        return;
    }
    if(argc > 2){
        rt_kprintf("Too many variables. Write 'LEDControl on' or 'LEDControl off' to turn the LED on or off.\n");
        return;
    }

    if(!rt_strcmp(argv[1], "on")){
        LEDOn();
        return;
    }
    if(!rt_strcmp(argv[1], "off")){
        LEDOff();
        return;
    }

    rt_kprintf("Invalid argument. Write 'LEDControl on' or 'LEDControl off' to turn the LED on or off.\n");
}



int main(void)
{

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(1);

    rt_timer_init(&buzzer_timer, "Buzzer", buzzerOn, RT_NULL, buzzer_time_on, RT_TIMER_FLAG_PERIODIC);
    rt_timer_init(&indicator_timer, "Indicator", indicatorOn, RT_NULL, indicator_time_on, RT_TIMER_FLAG_PERIODIC);

    rt_pin_mode(BUZZER_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(UVC_LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(INDICATOR_LED_PIN, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_thread_mdelay(1000);
        detectOvercurrent();
    }
}

MSH_CMD_EXPORT(LEDControl, Write 'LEDControl on' or 'LEDControl off' to turn the LED on or off.);
