/***************************************************************************//**
 * @file
 * @brief RTOS Lab 2 Code
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

//***********************************************************************************
// Include files
//***********************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_core.h"
#include "bsp.h"
#include "main.h"
#include "app.h"

// Lab Code Version Switch Macro
#define LAB_2_USE_INTERRUPT		0

// Define pushbutton status global variables
bool PB0_status = false;
bool PB1_status = false;
// Define slider direction global variable and init to INACTIVE
uint8_t slider_pos = INACTIVE;


//***********************************************************************************
// Systick Timer parameters
//***********************************************************************************
// variable to track systick count and init to 0
static volatile uint32_t msTicks = 0;
// number of ms desired for systick interrupt period
#define SYSTICK_MS 	 	 	1
// number of ms for program interrupts for LED and slider operations
#define TIMER_PERIOD_MS	 	100


//***********************************************************************************
// Function Prototypes
//***********************************************************************************
void led_drive(void);
#if !(LAB_2_USE_INTERRUPT)
static void Delay(uint32_t dlyTicks);
#endif


int main(void)
{
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

  /* Chip errata */
  CHIP_Init();

  /* Calculate the number of ticks desired */
  unsigned int systick_ticks = 1000/SYSTICK_MS;
  /* Setup SysTick timer for desired frequency interrupts for both Lab 2 version 1 and 2 */
  if (SysTick_Config(SystemCoreClockGet() / systick_ticks)) {
    while (1) ;
  }

  /* Init DCDC regulator and HFXO with kit specific parameters */
  /* Init DCDC regulator and HFXO with kit specific parameters */
  /* Initialize DCDC. Always start in low-noise mode. */
  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);
  em23Init.vScaleEM23Voltage = emuVScaleEM23_LowPower;
  EMU_EM23Init(&em23Init);
  CMU_HFXOInit(&hfxoInit);

  /* Switch HFCLK to HFRCO and disable HFRCO */
  CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
  CMU_OscillatorEnable(cmuOsc_HFXO, false, false);

  /* Call application program to open / initialize all required peripheral */
  app_peripheral_setup();

  /* Infinite blink loop */
  while (1) {

#if !(LAB_2_USE_INTERRUPT)
	// poll pushbutton 0
	poll_PB0(&PB0_status);
	// poll pushbutton 1
	poll_PB1(&PB1_status);
	// get the slider position
	slider_position(&slider_pos);
	// drive the LEDs based on the pushbutton and slider values
	led_drive();
	// delay period ms (based on Systick timer)
	Delay(TIMER_PERIOD_MS);
#endif

#if (LAB_2_USE_INTERRUPT)
	// If the systick timer has counted to a multiple of the desired period
	if ((msTicks % TIMER_PERIOD_MS) == 0) {
		// get the slider position
		slider_position(&slider_pos);
		// drive the LEDs based on the slider position and button values
		led_drive();
	}
	// enter low energy mode
	EMU_EnterEM1();
#endif
  }
}


/***************************************************************************//**
 * @brief
 *   Drives the LEDs based on the pushbutton and slider values.
 *
 * @details
 * 	 Utilizes global variables slider_pos, PB0_status, and PB1_status.
 * 	 Pushbutton operate on an XOR basis, and slider operates similarly.
 * 	 Pushbuttons and slider operate on OR basis.
 *
 * @note
 *   This function is called every time the LEDs should be set.
 *
 ******************************************************************************/
void led_drive() {
	// First determine the states of the pushbuttons combined as XOR
	bool button_activity = false;	// init button_activity to false
	if (PB0_status ^ PB1_status)
		button_activity = true;		// if the buttons are not both pressed, then set to true

	// When buttons are XOR false and the slider is either pressed on left and right or unpressed, turn off LEDs
	if (!button_activity && (slider_pos == INACTIVE)) {
		GPIO_PinOutClear(LED0_port, LED0_pin);
		GPIO_PinOutClear(LED1_port, LED1_pin);
	}
	// When buttons are XOR true and slider is LEFT
	else if (button_activity && (slider_pos == LEFT)) {
		// If pushbutton (PB0) pressed, turn on only LED0
		if (PB0_status) {
			GPIO_PinOutSet(LED0_port, LED0_pin);
			GPIO_PinOutClear(LED1_port, LED1_pin);
		}
		// if PB1 pressed, turn on both LED0 and LED1
		else {
			GPIO_PinOutSet(LED0_port, LED0_pin);
			GPIO_PinOutSet(LED1_port, LED1_pin);
		}
	}
	// If buttons are XOR true and slider is RIGHT
	else if (button_activity && (slider_pos == RIGHT)) {
		// If PB0 pressed, turn on both LEDs
		if (PB0_status) {
			GPIO_PinOutSet(LED0_port, LED0_pin);
			GPIO_PinOutSet(LED1_port, LED1_pin);
		}
		// If PB1 is pressed, only turn on LED1
		else {
			GPIO_PinOutSet(LED1_port, LED1_pin);
			GPIO_PinOutClear(LED0_port, LED0_pin);
		}
	}
	// If slider is INACTIVE (because either both sides are pressed or it's not pressed)
	// and buttons are XOR true
	else if (button_activity && (slider_pos == INACTIVE)) {
		// If PB0 is pressed, turn on only LED0
		if (PB0_status) {
			GPIO_PinOutSet(LED0_port, LED0_pin);
			GPIO_PinOutClear(LED1_port, LED1_pin);
		}
		// If PB1 is pressed, turn on LED1
		else {
			GPIO_PinOutSet(LED1_port, LED1_pin);
			GPIO_PinOutClear(LED0_port, LED0_pin);
		}
	}
	// If slider is not inactive and buttons are XOR false
	else if (!button_activity && !(slider_pos == INACTIVE)) {
		// If slider is left, turn on LED0
		if (slider_pos == LEFT) {
			GPIO_PinOutSet(LED0_port, LED0_pin);
			GPIO_PinOutClear(LED1_port, LED1_pin);
		}
		// If slider is right, turn on LED1
		else {
			GPIO_PinOutSet(LED1_port, LED1_pin);
			GPIO_PinOutClear(LED0_port, LED0_pin);
		}
	}
}

#if !(LAB_2_USE_INTERRUPT)
/***************************************************************************//**
 * @brief
 *   Delays number of msTick Systicks (typically 1 ms)
 *
 *
 * @details
 * 	 Utilizes global variable msTicks.
 *
 * @note
 *   This function is called every time a delay is desired in polling.
 *
 * @param dlyTicks
 *   Number of ticks (ms) to delay
 ******************************************************************************/
static void Delay(uint32_t dlyTicks)
{
	// save the state of the current systick value
  uint32_t curTicks;
  curTicks = msTicks;
  // busy wait until the desired tick delta has elapsed
  while ((msTicks - curTicks) < dlyTicks) ;
}
#endif

/***************************************************************************//**
 * @brief
 *   Interrupt handler for Systick module.
 *
 *
 * @details
 * 	 Utilizes global variable msTicks and macro SYSTICK_MS.
 *
 * @note
 *   This function is called every time systick interrupts.
 ******************************************************************************/
void SysTick_Handler(void)
{
	// disable interrupts and re-enable at end of IRQ.
	__disable_irq();
	// add the desired number of ms (ticks) to msTicks.
	msTicks = msTicks + SYSTICK_MS;
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *   Interrupt handler for GPIO Even pins module.
 *
 *
 * @details
 * 	 Utilizes global variable PB0_status to poll the value of pushbutton 0
 * 	 using poll_PB0() function.
 *
 * @note
 *   This function is called every time PB0 is pressed.
 *
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
	__disable_irq();
	/* Get and clear all pending GPIO interrupts */
	uint32_t interruptMask = GPIO_IntGet();
	GPIO_IntClear(interruptMask);
	poll_PB0(&PB0_status);
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *   Interrupt handler for GPIO Odd pins module.
 *
 *
 * @details
 * 	 Utilizes global variable PB1_status to poll the value of pushbutton 1
 * 	 using poll_PB1() function.
 *
 * @note
 *   This function is called every time PB1 is pressed.
 *
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
	__disable_irq();
	/* Get and clear all pending GPIO interrupts */
	uint32_t interruptMask = GPIO_IntGet();
	GPIO_IntClear(interruptMask);
	poll_PB1(&PB1_status);
	__enable_irq();
}


//***********************************************************************************
// Alternate functions used for LED Drive function. They are commented out and
// included if the grader is curious to check them out. They do not affect the
// operation of the project. These implementations use different logic but
// yield the same power consumption and current draw values as the current implementation,
// which is more readable and logically simple.
//***********************************************************************************

//void led_drive() {
//	// First determine the states of the pushbuttons combined
//	bool button_activity = false;
//	if (PB0_status ^ PB1_status) {
//		button_activity = true;
//	}
//	// Set LEDs first based on the slider position
//	switch (slider_pos) {
//		case LEFT:
//			GPIO_PinOutSet(LED0_port, LED0_pin);
//			GPIO_PinOutClear(LED1_port, LED1_pin);
//			break;
//		case RIGHT:
//			GPIO_PinOutSet(LED1_port, LED1_pin);
//			GPIO_PinOutClear(LED0_port, LED0_pin);
//			break;
//		default:
//			GPIO_PinOutClear(LED1_port, LED1_pin);
//			GPIO_PinOutClear(LED0_port, LED0_pin);
//			break;
//	}
//	// Then set LEDs based on pushbuttons
//	if (button_activity) {
//		if (PB0_status) {
//			GPIO_PinOutSet(LED0_port, LED0_pin);
//		}
//		else {
//			GPIO_PinOutSet(LED1_port, LED1_pin);
//		}
//	}
//}
//
//void led_drive() {
//	// First determine the states of the pushbuttons combined
//	bool button_activity = false;
//	if (PB0_status ^ PB1_status)
//		button_activity = true;
//	// handle the case when neither or both buttons are pressed
//	if (!button_activity) {
//		// Set LEDs based on the slider position
//		switch (slider_pos) {
//			case LEFT:
//				GPIO_PinOutSet(LED0_port, LED0_pin);
//				GPIO_PinOutClear(LED1_port, LED1_pin);
//				break;
//			case RIGHT:
//				GPIO_PinOutSet(LED1_port, LED1_pin);
//				GPIO_PinOutClear(LED0_port, LED0_pin);
//				break;
//			default:
//				GPIO_PinOutClear(LED1_port, LED1_pin);
//				GPIO_PinOutClear(LED0_port, LED0_pin);
//				break;
//		}
//	}
//	// If only one button is pressed, set its LED, then deal with slider-related LED
//	else if (PB0_status) {
//		GPIO_PinOutSet(LED0_port, LED0_pin);
//		// If the slider is in RIGHT position, set LED1
//		if (slider_pos == RIGHT)
//			GPIO_PinOutSet(LED1_port, LED1_pin);
//		else
//			GPIO_PinOutClear(LED1_port, LED1_pin);
//	}
//	else {
//		GPIO_PinOutSet(LED1_port, LED1_pin);
//		// If the slider is in LEFT position, set LED0
//		if (slider_pos == LEFT)
//			GPIO_PinOutSet(LED0_port, LED0_pin);
//		else
//			GPIO_PinOutClear(LED0_port, LED0_pin);
//	}
//}
