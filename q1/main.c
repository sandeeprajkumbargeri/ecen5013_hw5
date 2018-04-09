//*****************************************************************************
// Author: Sandeep Raj Kumbargeri
// Date: 6-April 2018
//
// hw5 q1 - Simple example to blink the on-board LED and use UART to print out the
// incremental count of the LED blinking.
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
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#define CLOCK_FREQUENCY     120000000
#define UART_PORT_0         0
#define UART_BAUD_RATE      115200
#define FREQUENCY_HZ        2
#define DELAY_FACTOR        (3 * 2 * FREQUENCY_HZ)

int main(void)
{
    uint32_t g_ui32SysClock;
    uint32_t blink_count = 0;

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

    // Enable the peripherals used by this example.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Set GPIO A0 and A1 as UART pins.
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(UART_PORT_0, UART_BAUD_RATE, g_ui32SysClock);

    // Printing the string.
    UARTprintf("Project for: Homework-5     6-April-2018\r\n");

    // Loop forever.
    while(1)
    {
        // Printing the count;
        UARTprintf("Blink Count: %u\r\n", ++blink_count);

        // Turn on the LED.
        MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        // Delay for a bit.
        MAP_SysCtlDelay(g_ui32SysClock / DELAY_FACTOR);

        // Turn off the LED.
        MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0x0);

        // Delay for a bit.
        MAP_SysCtlDelay(g_ui32SysClock / DELAY_FACTOR);
    }
}
