#include "keypad.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "gpio.h"

char keys[2];
unsigned temp_granica = 30;
int keysChanged = 1;

static int keyCount = 0;
static int keyReleased = 1;

TaskHandle_t KEY_TaskHandle;
TimerHandle_t KEY_TimerHandle;

const char KEY_MATRIX[4][3] = { { '1', '2', '3' }, { '4', '5', '6' }, { '7',
		'8', '9' }, { '*', '0', '#' } };

extern void homeworkOverflow();

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_7) {
		BaseType_t wokenTask = pdFALSE;
		vTaskNotifyGiveFromISR(KEY_TaskHandle, &wokenTask);
		portYIELD_FROM_ISR(wokenTask);
	} else if (GPIO_Pin == GPIO_PIN_10) {
		homeworkOverflow();
	}
}

static void KEY_Task(void *parameters) {
	UNUSED(parameters);
	keys[0] = '?';
	keys[1] = '?';
	while (1) {
		for (int row = 0; row < 4; ++row) {
			HAL_GPIO_WritePin(GPIOB, 1u << row, GPIO_PIN_SET);
		}
		vTaskDelay(pdMS_TO_TICKS(10));
		if (!keyReleased) {
			continue;
		}
		for (int row = 0; row < 4; ++row) {
			HAL_GPIO_WritePin(GPIOB, 1u << row, GPIO_PIN_RESET);
		}
		for (int row = 0; row < 4; ++row) {
			HAL_GPIO_WritePin(GPIOB, 1u << row, GPIO_PIN_SET);
			for (int column = 0; column < 3; ++column) {
				if (HAL_GPIO_ReadPin(GPIOB, 1u << (4 + column))
						== GPIO_PIN_SET) {
					keys[keyCount] = KEY_MATRIX[row][column];
					if (keyCount == 1) {
						temp_granica = (keys[0] - '0') * 10 + keys[1] - '0';
						keys[0] = '?';
						keys[1] = '?';
					}
					keyCount = 1 - keyCount;
					keysChanged = 1;
					keyReleased = 0;
					xTimerStart(KEY_TimerHandle, portMAX_DELAY);
				}
			}
			HAL_GPIO_WritePin(GPIOB, 1u << row, GPIO_PIN_RESET);
		}
	}
}

void KEY_Timer(TimerHandle_t xTimer) {
	UNUSED(xTimer);
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET) {
		xTimerStart(KEY_TimerHandle, portMAX_DELAY);
	} else {
		keyReleased = 1;
	}
}

void KEY_Init() {
	xTaskCreate(KEY_Task, "KEY_Task", 128, NULL, 5, &KEY_TaskHandle);
	KEY_TimerHandle = xTimerCreate("KEY_Timer", pdMS_TO_TICKS(10), pdFALSE,
	NULL, KEY_Timer);
}
