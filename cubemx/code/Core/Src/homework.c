#include "homework.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "driver_lcd.h"
#include "driver_uart.h"
#include "driver_motor.h"
#include "driver_temp.h"
#include "keypad.h"

#include "gpio.h"

FanState fanState = TURNED_OFF;
TimerHandle_t homeworkTimer;
TimerHandle_t ledTimer;

unsigned passedMs = 0;
unsigned rainfall = 0;

static uint32_t tempValue;
static char tempText[4];
static void homeworkTask(void *parameters) {
	UNUSED(parameters);
	char messageTemp[9] = "Temp:   ";
	char messageKisa[7] = "Kisa: ";
#pragma GCC diagnostic ignored "-Wtrigraphs"
	char messageThld[14] = "Thld:(??)=>30";
	// Ekvivalentna adresa: 0x80
	LCD_CommandEnqueue(LCD_INSTRUCTION,
	LCD_SET_DD_RAM_ADDRESS_INSTRUCTION | 0x00);
	for (uint32_t i = 0; i < 8; i++) {
		UART_AsyncTransmitCharacter(messageTemp[i]);
	}
	for (uint32_t i = 0; i < 6; i++) {
		LCD_CommandEnqueue(LCD_DATA, messageKisa[i]);
	}
	// Ekvivalentna adresa: 0xD0
	LCD_CommandEnqueue(LCD_INSTRUCTION,
	LCD_SET_DD_RAM_ADDRESS_INSTRUCTION | 0x50);
	for (uint32_t i = 0; i < 13; i++) {
		LCD_CommandEnqueue(LCD_DATA, messageThld[i]);
	}

	while (1) {
		tempValue = TEMP_GetCurrentValue();
		itoa(rainfall, tempText, 10);

		FanState fanStateTarget;
		if (tempValue < temp_granica) {
			fanStateTarget = TURNED_OFF;
		} else {
			fanStateTarget = SLOW;
		}
		for (uint32_t i = 0; i < abs(fanStateTarget - fanState); i++) {
			if (fanStateTarget > fanState) {
				MOTOR_SpeedIncrease();
			} else {
				MOTOR_SpeedDecrease();
			}
		}
		fanState = fanStateTarget;

		LCD_CommandEnqueue(LCD_INSTRUCTION,
		LCD_SET_DD_RAM_ADDRESS_INSTRUCTION | 0x06);
		for (uint32_t i = 0; i < strlen(tempText); i++) {
			LCD_CommandEnqueue(LCD_DATA, tempText[i]);
		}
		UART_AsyncTransmitCharacter('\b');
		UART_AsyncTransmitCharacter('\b');
		if (tempValue < 10) {
			UART_AsyncTransmitCharacter(' ');
		} else {
			UART_AsyncTransmitCharacter(tempValue / 10 + '0');
		}
		UART_AsyncTransmitCharacter(tempValue % 10 + '0');

		if (keysChanged) {
			keysChanged = 0;
			LCD_CommandEnqueue(LCD_INSTRUCTION,
			LCD_SET_DD_RAM_ADDRESS_INSTRUCTION | 0x56);
			LCD_CommandEnqueue(LCD_DATA, keys[0]);
			LCD_CommandEnqueue(LCD_DATA, keys[1]);
			LCD_CommandEnqueue(LCD_INSTRUCTION,
			LCD_SET_DD_RAM_ADDRESS_INSTRUCTION | 0x5B);
			LCD_CommandEnqueue(LCD_DATA,
					(temp_granica < 10) ? ' ' : (temp_granica / 10) + '0');
			LCD_CommandEnqueue(LCD_DATA, (temp_granica % 10) + '0');
		}

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void homeworkOverflow() {
	rainfall = 36000 / passedMs;
	passedMs = 0;
}

void homeworkCounter(TimerHandle_t xTimer) {
	UNUSED(xTimer);
	++passedMs;
}

void ledCounter(TimerHandle_t xTimer) {
	UNUSED(xTimer);
	if (TEMP_GetCurrentValue() < temp_granica) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);
	}
}

void homeworkInit() {
	LCD_Init();
	UART_Init();
	MOTOR_Init();
	TEMP_Init();
	KEY_Init();
	xTaskCreate(homeworkTask, "homeworkTask", 64, NULL, 5, NULL);
	homeworkTimer = xTimerCreate("homeworkTimer", pdMS_TO_TICKS(1), pdTRUE,
	NULL, homeworkCounter);
	xTimerStart(homeworkTimer, portMAX_DELAY);
	ledTimer = xTimerCreate("ledTimer", pdMS_TO_TICKS(500), pdTRUE,
	NULL, ledCounter);
	xTimerStart(ledTimer, portMAX_DELAY);
}

