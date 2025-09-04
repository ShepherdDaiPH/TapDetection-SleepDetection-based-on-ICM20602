#include "icm20602.h"
#include <stdio.h>
#include "usart.h"
#include <string.h>
#include <math.h>

// 必要变量定义
int      counter        = 0;
uint32_t timer          = 0;
uint32_t desired_timer  = 100;
SystemState POWER_MODE = NORMAL_MODE;
uint8_t low_power_mode_1    = 0x28;
uint8_t low_power_mode_2    = 0x07;
uint8_t normal_power_mode = 0x00;
float init_sum_x = 0.0f;
float init_sum_y = 0.0f;
float init_sum_z = 0.0f;
SensorState ICM20602_SensorState;

// 加速度计量程对应的满量程范围（g）
#define ACCEL_FS_2G     2.0f
#define ACCEL_FS_4G     4.0f
#define ACCEL_FS_8G     8.0f
#define ACCEL_FS_16G    16.0f

// 陀螺仪量程对应的满量程范围（°/s）
#define GYRO_FS_250DPS  250.0f
#define GYRO_FS_500DPS  500.0f
#define GYRO_FS_1000DPS 1000.0f
#define GYRO_FS_2000DPS 2000.0f

// 16位ADC的量程范围
#define ADC_FULL_SCALE  32768.0f

// 角度转弧度系数（π/180）
#define DEG_TO_RAD      0.017453292519943295f

AccelRange current_accel_range;
GyroRange current_gyro_range;

HAL_StatusTypeDef ICM20602_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data);
ICM20602_Data icm20602_Data;

/**
  * @brief  初始化 ICM20602
  */
HAL_StatusTypeDef ICM20602_Init(I2C_HandleTypeDef *hi2c, AccelRange accelRange, GyroRange gyroRange) {
  uint8_t whoAmI;
  
  current_accel_range = accelRange;
  current_gyro_range = gyroRange;

  if (ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x00) != HAL_OK) {
    return HAL_ERROR;
  }
  
  HAL_Delay(100);
  
  if (ICM20602_ReadReg(hi2c, WHO_AM_I_REG, &whoAmI) != HAL_OK) {
    return HAL_ERROR;
  }
  
  if (whoAmI != 0x12) {
    return HAL_ERROR;
  }
  
  if (ICM20602_WriteReg(hi2c, SMPLRT_DIV_REG, 0x07) != HAL_OK) {
    return HAL_ERROR;
  }
  
  if (ICM20602_WriteReg(hi2c, ACCEL_CONFIG_REG, accelRange) != HAL_OK) {
    return HAL_ERROR;
  }

  if (ICM20602_WriteReg(hi2c, GYRO_CONFIG_REG, gyroRange) != HAL_OK) {
    return HAL_ERROR;
  }

  if (ICM20602_WriteReg(hi2c, INT_PIN_CFG, 0x00) != HAL_OK) {
    return HAL_ERROR;
  }
  
  return HAL_OK;
}

/**
  * @brief  读取 ICM20602 数据
  */
HAL_StatusTypeDef ICM20602_ReadData(I2C_HandleTypeDef *hi2c, ICM20602_Data *data) {
  uint8_t buffer[14];
  int16_t raw_accelX, raw_accelY, raw_accelZ;
  int16_t raw_gyroX, raw_gyroY, raw_gyroZ;
  float accel_fs, gyro_fs;
  
  if (HAL_I2C_Mem_Read(hi2c, ICM20602_ADDR, ACCEL_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 14, 100) != HAL_OK) {
    return HAL_ERROR;
  }
  
  raw_accelX = (int16_t)(buffer[0] << 8 | buffer[1]);
  raw_accelY = (int16_t)(buffer[2] << 8 | buffer[3]);
  raw_accelZ = (int16_t)(buffer[4] << 8 | buffer[5]);
  
  switch(current_accel_range) {
    case ACCEL_RANGE_2G:  accel_fs = ACCEL_FS_2G;  break;
    case ACCEL_RANGE_4G:  accel_fs = ACCEL_FS_4G;  break;
    case ACCEL_RANGE_8G:  accel_fs = ACCEL_FS_8G;  break;
    case ACCEL_RANGE_16G: accel_fs = ACCEL_FS_16G; break;
    default: return HAL_ERROR;
  }
  data->accelX = ((float)raw_accelX / ADC_FULL_SCALE) * accel_fs;
  data->accelY = ((float)raw_accelY / ADC_FULL_SCALE) * accel_fs;
  data->accelZ = ((float)raw_accelZ / ADC_FULL_SCALE) * accel_fs;
  
  int16_t tempRaw = (int16_t)(buffer[6] << 8 | buffer[7]);
  data->temp = (tempRaw / 340.0f) + 36.53f;
  
  raw_gyroX = (int16_t)(buffer[8] << 8 | buffer[9]);
  raw_gyroY = (int16_t)(buffer[10] << 8 | buffer[11]);
  raw_gyroZ = (int16_t)(buffer[12] << 8 | buffer[13]);
  
  switch(current_gyro_range) {
    case GYRO_RANGE_250DPS:  gyro_fs = GYRO_FS_250DPS;  break;
    case GYRO_RANGE_500DPS:  gyro_fs = GYRO_FS_500DPS;  break;
    case GYRO_RANGE_1000DPS: gyro_fs = GYRO_FS_1000DPS; break;
    case GYRO_RANGE_2000DPS: gyro_fs = GYRO_FS_2000DPS; break;
    default: return HAL_ERROR;
  }
  data->gyroX = ((raw_gyroX / ADC_FULL_SCALE) * 2 * gyro_fs) * DEG_TO_RAD;
  data->gyroY = ((raw_gyroY / ADC_FULL_SCALE) * 2 * gyro_fs) * DEG_TO_RAD;
  data->gyroZ = ((raw_gyroZ / ADC_FULL_SCALE) * 2 * gyro_fs) * DEG_TO_RAD;
  
  return HAL_OK;
}

/**
  * @brief  写寄存器
  */
HAL_StatusTypeDef ICM20602_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data) {
  uint8_t txData[2] = {reg, data};
  return HAL_I2C_Master_Transmit(hi2c, ICM20602_ADDR, txData, 2, 100);
}

/**
  * @brief  读寄存器
  */
HAL_StatusTypeDef ICM20602_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data) {
  if (HAL_I2C_Master_Transmit(hi2c, ICM20602_ADDR, &reg, 1, 100) != HAL_OK) {
    return HAL_ERROR;
  }
  return HAL_I2C_Master_Receive(hi2c, ICM20602_ADDR, data, 1, 100);
}

/**
  * @brief  进入低功耗模式
  */
HAL_StatusTypeDef ICM20602_ENTER_LOW_POWER_MODE(I2C_HandleTypeDef *hi2c, uint8_t THRESHOLD) 
{   
    // Step1: 确保加速度计在工作，陀螺仪关闭
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x08) != HAL_OK) return HAL_ERROR; // TEMP_DIS=1, CYCLE=0
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_2_REG, 0x07) != HAL_OK) return HAL_ERROR; // Gyro off, Accel on

    // Step2: 配置加速度计 (低功耗滤波)
    if(ICM20602_WriteReg(hi2c, ACCEL_CONFIG2, (1<<3) | 0x01) != HAL_OK) return HAL_ERROR;

    // Step3: WOM 中断使能
    if(ICM20602_WriteReg(hi2c, INT_ENABLE, 0xE0) != HAL_OK) return HAL_ERROR;

    // Step4: 设置阈值
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_X_THR, THRESHOLD) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Y_THR, THRESHOLD) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Z_THR, THRESHOLD) != HAL_OK) return HAL_ERROR;

    // Step5: 设置中断模式
    if(ICM20602_WriteReg(hi2c, ACCEL_INTEL_CTRL, (1<<7) | (1<<6)) != HAL_OK) return HAL_ERROR;

    // Step6: 设置采样率
    if(ICM20602_WriteReg(hi2c, SMPLRT_DIV, 19) != HAL_OK) return HAL_ERROR;

    // Step7: 最后进入低功耗循环模式
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x28) != HAL_OK) return HAL_ERROR; // CYCLE=1

    sprintf(uartBuffer, "Enter Low Power Mode!\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);

    POWER_MODE = LOW_POWER_MODE;

    return HAL_OK;
}


/**
  * @brief  退出低功耗模式
  */
HAL_StatusTypeDef ICM20602_EXIT_LOW_POWER_MODE(I2C_HandleTypeDef *hi2c)
{
    // Step1: 退出循环模式 (CYCLE=0, TEMP_DIS=0/1 根据需求)
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x00) != HAL_OK) return HAL_ERROR; 
    // 上电后默认值 0x00: 时钟=内部8MHz, CYCLE=0, SLEEP=0
    // 如果你需要关闭温度，可以用 0x08 (TEMP_DIS=1)

    // Step2: 关闭 WOM 中断
    if(ICM20602_WriteReg(hi2c, INT_ENABLE, 0x00) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_INTEL_CTRL, 0x00) != HAL_OK) return HAL_ERROR;

    // Step3: 恢复加速度计配置 (正常模式滤波)
    if(ICM20602_WriteReg(hi2c, ACCEL_CONFIG2, 0x07) != HAL_OK) return HAL_ERROR;  
    // 默认 0x07: ACCEL_FCHOICE_B=0, A_DLPF_CFG=111 (BW=~5Hz)
    
    // Step4: 清除 WOM 阈值
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_X_THR, 0x00) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Y_THR, 0x00) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Z_THR, 0x00) != HAL_OK) return HAL_ERROR;

    // Step5: 恢复采样率 (根据你应用需要，这里给个常见值)
    if(ICM20602_WriteReg(hi2c, SMPLRT_DIV, 0x07) != HAL_OK) return HAL_ERROR; 
    // 对应采样率 = 1kHz / (1+7) = 125Hz

    // Step6: 恢复正常供电配置 (gyro + accel 全开)
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_2_REG, 0x00) != HAL_OK) return HAL_ERROR; 
    // 0x00 表示 accel+gyro 全开

    sprintf(uartBuffer, "Exit Low Power Mode!\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);

    POWER_MODE = NORMAL_MODE;

    return HAL_OK;
}


/**
  * @brief  打印加速度
  */
void Print_Accel(ICM20602_Data *data) 
{
  float raw_magnitude = sqrtf(
      data->accelX * data->accelX +
      data->accelY * data->accelY +
      data->accelZ * data->accelZ
  );

  sprintf(uartBuffer, "Raw:    [%6.3f, %6.3f, %6.3f] |Mag|: %.3f\r\n",
          data->accelX, data->accelY, data->accelZ, raw_magnitude);
  HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);

  sprintf(uartBuffer, "========================================\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
}

/**
  * @brief  运动状态估计
  */
void MovingEstimation(ICM20602_Data *data) 
{
  float accel_magnitude = sqrtf(
      data->accelX * data->accelX +
      data->accelY * data->accelY +
      data->accelZ * data->accelZ
  );

  if (accel_magnitude < 0.90f || accel_magnitude > 1.10f) {
      ICM20602_SensorState = IsMoving;
  } else {
      ICM20602_SensorState = IsResting;
  }

  switch (ICM20602_SensorState) {
      case IsMoving:
          counter++;
          timer++;
          break;
      case IsResting:
          counter--;
          timer++;
          break;
      default:
          break;
  }
}

/**
  * @brief  检测静止/运动
  */
SensorState ICM20602_INACTIVE_MOTION_DETECTION(ICM20602_Data *data)
{
  float accel_magnitude = sqrtf(
      data->accelX * data->accelX +
      data->accelY * data->accelY +
      data->accelZ * data->accelZ
  );

  if (accel_magnitude < 0.90f || accel_magnitude > 1.10f) {
      ICM20602_SensorState = IsMoving;
  }else {
      ICM20602_SensorState = IsResting;
  }
  
  return ICM20602_SensorState;
}

void PrintICM20602Registers(void) {
    uint8_t regs[] = {WHO_AM_I_REG, PWR_MGMT_1_REG, PWR_MGMT_2_REG, SMPLRT_DIV_REG, ACCEL_CONFIG_REG,
                      ACCEL_CONFIG2, GYRO_CONFIG_REG, INT_ENABLE, INT_STATUS, ACCEL_INTEL_CTRL,
                      ACCEL_WOM_X_THR, ACCEL_WOM_Y_THR, ACCEL_WOM_Z_THR};
    const char* names[] = {"WHO_AM_I","PWR_MGMT_1", "PWR_MGMT_2", "SMPLRT_DIV","ACCEL_CONFIG",
                           "ACCEL_CONFIG2", "GYRO_CONFIG","INT_ENABLE","INT_STATUS", "ACCEL_INTEL_CTRL",
                           "ACCEL_WOM_X_THR", "ACCEL_WOM_Y_THR", "ACCEL_WOM_Z_THR"};

    for(int i=0;i<14;i++){
        sprintf(uartBuffer,"%s: 0x%02X\r\n", names[i], ReadRegister(regs[i]));
        HAL_UART_Transmit(&huart2,(uint8_t*)uartBuffer,strlen(uartBuffer),HAL_MAX_DELAY);
    }
}