//*****************************************************************************
// Author: Sandeep Raj Kumbargeri
// Date: 6-April 2018
//
// hw5 q3 - Simple example for creation of three tasks to blink an LED at
//          a frequency in FreeRTOS and prnting tick count of a task using UART
//          with the use of timer callback handler, event group notifications
//          and semaphores.
//
// Copyright (c) 2013-2016 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.1.3.156 of the EK-TM4C1294XL Firmware Package.
//
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
#include "event_groups.h"

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
#include "utils/uartstdio.h"

#define CLOCK_FREQUENCY     120000000
#define UART_PORT_0         0
#define UART_BAUD_RATE      115200

#define TIMER_FREQUENCY     4
#define TIMER_PERIOD        1000/(1.5 * 2 * TIMER_FREQUENCY)

#define ON                  pdTRUE
#define OFF                 pdFALSE


#define EVENT_TOGGLE_LED            ((EventBits_t) 1<<0)        //Event bit for toggling the LED
#define EVENT_PRINT_STRING            ((EventBits_t) 1<<1)        //Event bit for printing the string

SemaphoreHandle_t sem_2hz, sem_4hz;
EventGroupHandle_t event_group;
QueueHandle_t print_queue;

void Task_2Hz(void *pvParameters)
{
    sem_2hz = xSemaphoreCreateCounting(1, 0);

    while(1)
    {
        xSemaphoreTake(sem_2hz, portMAX_DELAY);                     //Block on sem_2hz semaphore
        xEventGroupSetBits(event_group, EVENT_TOGGLE_LED);          //Set event group with EVENT_TOGGLE_LED bits
    }
}

void Task_4Hz(void *pvParameters)
{
    TickType_t count;
    char print_string[64];

    sem_4hz = xSemaphoreCreateCounting(1, 0);

    while(1)
    {
        xSemaphoreTake(sem_4hz, portMAX_DELAY);                     //Block on sem_4hz semaphore

        count = xTaskGetTickCount();                                //Get current task's tick count
        bzero(print_string, sizeof(print_string));                  //Empty the string bufferr
        sprintf(print_string, "Tick Count = %u\r\n", count);        //Write tick count to the string buffer

        xQueueSend(print_queue, print_string, portMAX_DELAY);       //Send the string buffer into the queue
        xEventGroupSetBits(event_group, EVENT_PRINT_STRING);        //Set event group with EVENT_PRINT_STRING bits
    }
}


void Task_EventHandler(void *pvParameters)
{
    BaseType_t led_status = OFF;
    EventBits_t event;
    char print_buffer[64];

    while(1)
    {
        //Check for which events have occurred
        event = xEventGroupWaitBits(event_group, (EVENT_TOGGLE_LED | EVENT_PRINT_STRING), ON, OFF, portMAX_DELAY);

        if(event & EVENT_TOGGLE_LED)
        {
            //When LED toggle event occurs, toggle the LED
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

        if(event & EVENT_PRINT_STRING)
        {
            //When event for printing the string occurs, print on UART
            bzero(print_buffer, sizeof(print_buffer));
            xQueueReceive(print_queue, print_buffer, portMAX_DELAY);
            UARTprintf(print_buffer);
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

    //Create the event group and create the queue
    event_group = xEventGroupCreate();
    print_queue = xQueueCreate(10, 64);

    // Enable the peripherals used by this example.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Set GPIO A0 and A1 as UART pins.
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //Config UART on UART_PORT with UART_BAUD_RATE
    UARTStdioConfig(UART_PORT_0, UART_BAUD_RATE, g_ui32SysClock);

    // Create task for 2Hz Blinking LED
    xTaskCreate(Task_2Hz, "LED 2Hz", 1000, NULL, 1, NULL );

    // Create task for 4Hz Blinking LED
    xTaskCreate(Task_4Hz, "LED 4Hz", 1000, NULL, 1, NULL );

    // Create task for handling event notifications
    xTaskCreate(Task_EventHandler, "Task for handling notifications from the first two tasks", 1000, NULL, 1, NULL );

    timer = xTimerCreate("4Hz Timer", pdMS_TO_TICKS(TIMER_PERIOD), pdTRUE, (void *)&timer_id, timer_callback);
    xTimerStart(timer, portMAX_DELAY);

    // Start the scheduler
    vTaskStartScheduler();

    // Loop forever.
    while(1){
    }
}
