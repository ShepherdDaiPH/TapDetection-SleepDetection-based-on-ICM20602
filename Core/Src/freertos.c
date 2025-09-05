/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "icm20602.h"
#include "i2c.h"
#include "tap_detection.h"
#include "usart.h"
#include "string.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};
/* Definitions for Testing_Task */
osThreadId_t Testing_TaskHandle;
const osThreadAttr_t Testing_Task_attributes = {
  .name = "Testing_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for BatteryManage */
osThreadId_t BatteryManageHandle;
const osThreadAttr_t BatteryManage_attributes = {
  .name = "BatteryManage",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for SleepEnterPoint */
osSemaphoreId_t SleepEnterPointHandle;
const osSemaphoreAttr_t SleepEnterPoint_attributes = {
  .name = "SleepEnterPoint"
};
/* Definitions for SleepExitPoint */
osSemaphoreId_t SleepExitPointHandle;
const osSemaphoreAttr_t SleepExitPoint_attributes = {
  .name = "SleepExitPoint"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call osDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
}
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of SleepEnterPoint */
  SleepEnterPointHandle = osSemaphoreNew(1, 1, &SleepEnterPoint_attributes);

  /* creation of SleepExitPoint */
  SleepExitPointHandle = osSemaphoreNew(1, 1, &SleepExitPoint_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Testing_Task */
  Testing_TaskHandle = osThreadNew(StartTask02, NULL, &Testing_Task_attributes);

  /* creation of BatteryManage */
  BatteryManageHandle = osThreadNew(StartTask03, NULL, &BatteryManage_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(osWaitForever);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Testing_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
  static TickType_t lastDetectTime   = 0;
  static TickType_t lastSendTime     = 0;
  static TickType_t lastActivityTime = 0;

  lastActivityTime = xTaskGetTickCount();
  for (;;)
  {
    TickType_t currTime = xTaskGetTickCount();

    // 读取ICM20602数据
    if (ICM20602_ReadData(&hi2c1, &icm20602_Data) == HAL_OK) {
      // 定时检测拍击
      if (currTime - lastDetectTime >= TAP_DETECT_INTERVAL) {
        lastDetectTime = currTime;
        DetectTap(&icm20602_Data);
      }

      // 定时发送传感器数据
      if (currTime - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = currTime;
        sprintf(uartBuffer, 
                "Accel: X=%.2fg, Y=%.2fg, Z=%.2fg | "
                "Gyro: X=%.2frad/s, Y=%.2frad/s, Z=%.2frad/s | "
                "Temp: %.2f°C\r\n",
                icm20602_Data.accelX, icm20602_Data.accelY, icm20602_Data.accelZ,
                icm20602_Data.gyroX, icm20602_Data.gyroY, icm20602_Data.gyroZ,
                icm20602_Data.temp);
        HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
      }
    } else {
      HAL_UART_Transmit(&huart2, (uint8_t*)"ICM20602 Read Err\r\n", 19, 100);
    }
    if(motionFlag == 1){
        motionFlag = 0;
        lastActivityTime = currTime; // 重置最后活动时间
    }
    if(currTime - lastActivityTime > 5000){ // 10分钟无运动
      sprintf(uartBuffer, "No motion detected for 10 minutes, preparing to enter sleep mode...\r\n");
      HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
      lastActivityTime = xTaskGetTickCount(); // 避免唤醒后马上进入休眠
			
			osSemaphoreRelease(SleepEnterPointHandle);
			vTaskSuspend(NULL);
    }
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the BatteryManageme thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
  /* Infinite loop */
  for(;;)
  {
		osSemaphoreAcquire(SleepEnterPointHandle, osWaitForever);
		
		 // MCU 空闲时进入低功耗
    ICM20602_ENTER_LOW_POWER_MODE(&hi2c1, 0x20);
		
    // 进入 SLEEP 或 STOP
    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		
    ICM20602_EXIT_LOW_POWER_MODE(&hi2c1);
		
		vTaskResume(Testing_TaskHandle);
		
    osDelay(1);
  }
  /* USER CODE END StartTask03 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

