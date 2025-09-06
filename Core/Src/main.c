/* USER CODE BEGIN Header */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "icm20602.h"
#include "tap_detection.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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

/* USER CODE BEGIN PV */
char uartBuffer[256];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void EnterSTOPMODE(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  static uint32_t lastDetectTime   = 0;
  static uint32_t lastSendTime     = 0;
  static uint32_t lastActivityTime = 0;

  lastActivityTime = HAL_GetTick();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  if (ICM20602_Init(&hi2c1, ACCEL_RANGE_8G, GYRO_RANGE_1000DPS) != HAL_OK) {
    HAL_UART_Transmit(&huart2, (uint8_t*)"ICM20602 Init Failed!\r\n", 22, 100);
  } else {
    HAL_UART_Transmit(&huart2, (uint8_t*)"ICM20602 Ready (Tap/Swing Mode)\r\n", 34, 100);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t currTime = HAL_GetTick();

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
			lastActivityTime = currTime;
		}
		
		if(currTime - lastActivityTime > 5000){
			sprintf(uartBuffer, "No motion detected for 5 seconds, preparing to enter sleep mode...\r\n");
      HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
      lastActivityTime = HAL_GetTick(); // 避免唤醒后马上进入休眠
			
			ICM20602_ENTER_LOW_POWER_MODE(&hi2c1, 0x20);
		
			__disable_irq();
			
			HAL_SuspendTick();
		
			HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
		
			HAL_ResumeTick();
			
			SystemClock_Config();
			
			__enable_irq();
		
			ICM20602_EXIT_LOW_POWER_MODE(&hi2c1);
			
			HAL_Delay(1000);
		}

    HAL_Delay(1);
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_0)  // MOTION_INT_PIN
  {		
    sprintf(uartBuffer, "EXTI triggered!\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), HAL_MAX_DELAY);
  }
}


void EnterStopMode(void){
	__disable_irq(); // 关闭中断，避免干扰
	
	ICM20602_ENTER_LOW_POWER_MODE(&hi2c1, 0x20); // 调整ICM进入休眠模式
	
	__HAL_RCC_USART2_CLK_DISABLE(); // 关闭USART2时钟
  __HAL_RCC_I2C1_CLK_DISABLE();   // 关闭I2C1时钟
  __HAL_RCC_GPIOA_CLK_DISABLE();  // 关闭GPIOA时钟		
			
	HAL_SuspendTick(); // 停止SysTick中断，避免唤醒后立即进入STOP模式
		
	HAL_PWR_EnterSTANDBYMode(); // 进入STOP模式
	//HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI); // 进入STOP模式
		
	HAL_ResumeTick(); // 恢复SysTick中断
			
	SystemClock_Config(); // 重新配置系统时钟
	
	ICM20602_EXIT_LOW_POWER_MODE(&hi2c1); // 让ICM退出休眠模式
			
	__enable_irq();		// 重新使能中断
  
  HAL_Delay(1000); // 等待ICM稳定
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
