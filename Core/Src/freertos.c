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
uint8_t ReadRegister(uint8_t reg) {
    uint8_t value = 0;
    if(HAL_I2C_Mem_Read(&hi2c1, ICM20602_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 1000) != HAL_OK) {
        sprintf(uartBuffer, "Read reg 0x%02X failed!\r\n", reg);
        HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
    }
    return value;
}
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
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TapDetection */
osThreadId_t TapDetectionHandle;
const osThreadAttr_t TapDetection_attributes = {
  .name = "TapDetection",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);

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
   specified, or call vTaskDelay()). If the application makes use of the
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

/* USER CODE BEGIN PREPOSTSLEEP */
__weak void PreSleepProcessing(TickType_t *ulExpectedIdleTime)
{   

    // 打印进入休眠信息
    sprintf(uartBuffer, "Entering sleep mode!\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);  

    // 关闭tick
    HAL_SuspendTick();

		// 关闭中断避免干扰
		__disable_irq();
		
		//调节ICM20602进入低功耗模式
		ICM20602_ENTER_LOW_POWER_MODE(&hi2c1, 0x20);
	
		//关闭不必要外设
		__HAL_RCC_USART2_CLK_DISABLE();
    __HAL_RCC_TIM4_CLK_DISABLE();
		
		//调节MCU进入STOP模式
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	
		//恢复中断功能
		__enable_irq();

}

__weak void PostSleepProcessing(TickType_t *ulExpectedIdleTime)
{   
    // 恢复tick
    HAL_ResumeTick();

		// 开启外设
		__HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
	
		// ICM20602退出低功耗模式
		ICM20602_EXIT_LOW_POWER_MODE(&hi2c1);

    // 打印退出休眠信息
    sprintf(uartBuffer, "Exited sleep mode!\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);

}
/* USER CODE END PREPOSTSLEEP */

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

  /* creation of TapDetection */
  TapDetectionHandle = osThreadNew(StartTask02, NULL, &TapDetection_attributes);

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
    osDelay(15000);
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
    if(currTime - lastActivityTime > 600000){ // 10分钟无运动
      sprintf(uartBuffer, "No motion detected for 10 minutes, preparing to enter sleep mode...\r\n");
      HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
      lastActivityTime = xTaskGetTickCount(); // 避免唤醒后马上进入休眠
      vTaskDelay(30000); // 等待30秒以进入休眠模式
    }

    xTaskDelay(5);
  }
  /* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

