//*****************************************************************************
// Author: Sandeep Raj Kumbargeri
// Date: 6-April 2018
//
// hw5 q2 - Simple example for creation of two tasks to blink different LEDs at
//          different frequencies in FreeRTOS using timer callback handler and
//          semaphores.
//
// Written for ECEN 5013 at University of Colorado Boulder in Spring 2018
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <strings.h>

#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"
#include "portable.h"
#include "semphr.h"
#include "timers.h"

#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"

#define CLOCK_FREQUENCY     120000000

#define TIMER_FREQUENCY     4
#define TIMER_PERIOD        1000/(1.5 * 2 * TIMER_FREQUENCY)

#define ON                  pdTRUE
#define OFF                 pdFALSE

SemaphoreHandle_t sem_2hz, sem_4hz;

void Task_2Hz(void *pvParameters)
{
    BaseType_t led_status = OFF;

    sem_2hz = xSemaphoreCreateCounting(1, 0);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    while(1)
    {
        xSemaphoreTake(sem_2hz, portMAX_DELAY);

        if(led_status)
        {
            //Switch off LED
            led_status = OFF;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
        }

        else
        {
            //Switch on LED
            led_status = ON;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
        }
    }
}

void Task_4Hz(void *pvParameters)
{
    BaseType_t led_status = OFF;

    sem_4hz = xSemaphoreCreateCounting(1, 0);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    while(1)
    {
        xSemaphoreTake(sem_4hz, portMAX_DELAY);

        if(led_status)
        {
            //Switch off LED
            led_status = OFF;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
        }

        else
        {
            //Switch on LED
            led_status = ON;
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
        }
    }
}

void timer_callback(TimerHandle_t timer)
{
    BaseType_t pxHigherPriorityTaskWoken = pdTRUE;
    static int count = 0;

    //For every two times, give the semaphore to 2hz task. Otherwise, give it  to 4hz task.
    if(count == 2)
    {
        xSemaphoreGiveFromISR(sem_2hz, &pxHigherPriorityTaskWoken);
        count = -1;
    }
    else
        xSemaphoreGiveFromISR(sem_4hz, &pxHigherPriorityTaskWoken);

    count++;
}


int main(void)
{
    uint32_t g_ui32SysClock;
    BaseType_t timer_id = 0;
    TimerHandle_t timer;

    // Set the clocking to run directly from the crystal at 120MHz.
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), CLOCK_FREQUENCY);

    // Enable the GPIO port that is used for the on-board LED.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    // Check if the peripheral access is enabled.
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));

    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    // Create task for 2Hz Blinking LED
    xTaskCreate(Task_2Hz, "LED 2Hz", 1000, NULL, 1, NULL );

    // Create task for 4Hz Blinking LED
    xTaskCreate(Task_4Hz, "LED 4Hz", 1000, NULL, 1, NULL );

    //Create timer for 4hz and register the timer_callback handler
    timer = xTimerCreate("4Hz Timer", pdMS_TO_TICKS(TIMER_PERIOD), pdTRUE, (void *)&timer_id, timer_callback);
    xTimerStart(timer, portMAX_DELAY);

    // Start the scheduler
    vTaskStartScheduler();

    // Loop forever.
    while(1){
    }
}
