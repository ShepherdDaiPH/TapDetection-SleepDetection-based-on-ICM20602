// ================== Includes ==================
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "mpu6050.h"
// ================== Defines ===================

// ================== Variables =================
MPU6050_Data mpuData;
char uartBuffer[300];

void SystemClock_Config(void);
int threshold = 0x20;
int duration = 0x20;
// ==============================================

// Main Function
// ==============================================
int main(void)
{
    // Hardware Initialization
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();
  
    // MPU6050 Initialization
    if (MPU6050_Init(&hi2c1, ACCEL_RANGE_2G, GYRO_RANGE_250DPS) != HAL_OK) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"MPU6050 init failed!\r\n", 22, 100);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t*)"MPU6050 init success!\r\n", 23, 100);
    }

    // Main Loop
    while (1) {
        MPU6050_ZERO_MOTION_INIT(&hi2c1, threshold, duration);
        int status = CHECK_MPU6050_INTERRUPT(&hi2c1);
        switch(status) {
            case 1: 
                sprintf(uartBuffer, "Device is resting (STATUS=0x%02X)\r\n", status);
                break;
            case 0:
                sprintf(uartBuffer, "Device is moving (STATUS=0x%02X)\r\n", status);
                break;
            default:
                sprintf(uartBuffer, "Error (STATUS=0x%02X)\r\n", status);
        }
        HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
    }
}

// System Clock Configuration
// ==============================================
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

// Error Handler
// ==============================================
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    // User can add error handling here
}
#endif