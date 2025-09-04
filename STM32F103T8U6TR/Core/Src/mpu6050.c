/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mpu6050.c
  * @brief          : MPU6050 driver with corrected conversion formulas
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "mpu6050.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static AccelRange current_accel_range;  // 存储当前加速度计量程
static GyroRange current_gyro_range;    // 存储当前陀螺仪量程
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static HAL_StatusTypeDef MPU6050_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data);
static HAL_StatusTypeDef MPU6050_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  初始化MPU6050
  * @param  hi2c: I2C句柄指针
  * @param  accelRange: 加速度计量程
  * @param  gyroRange: 陀螺仪量程
  * @retval HAL状态
  */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c, AccelRange accelRange, GyroRange gyroRange) {
  uint8_t whoAmI;
  
  // 保存当前量程用于转换计算
  current_accel_range = accelRange;
  current_gyro_range = gyroRange;
  
  // 唤醒MPU6050（退出睡眠模式）
  if (MPU6050_WriteReg(hi2c, PWR_MGMT_1_REG, 0x00) != HAL_OK) {
    return HAL_ERROR;
  }
  
  // 延时等待MPU6050启动
  HAL_Delay(100);
  
  // 读取WHO_AM_I寄存器确认设备连接
  if (MPU6050_ReadReg(hi2c, WHO_AM_I_REG, &whoAmI) != HAL_OK) {
    return HAL_ERROR;
  }
  
  // WHO_AM_I寄存器默认值应为0x68
  if (whoAmI != 0x68) {
    return HAL_ERROR;
  }
  
  // 设置采样率分频器
  if (MPU6050_WriteReg(hi2c, SMPLRT_DIV_REG, 0x07) != HAL_OK) {
    return HAL_ERROR;
  }
  
  // 配置陀螺仪量程
  if (MPU6050_WriteReg(hi2c, GYRO_CONFIG_REG, gyroRange) != HAL_OK) {
    return HAL_ERROR;
  }
  
  // 配置加速度计量程
  if (MPU6050_WriteReg(hi2c, ACCEL_CONFIG_REG, accelRange) != HAL_OK) {
    return HAL_ERROR;
  }
  
  return HAL_OK;
}

/**
  * @brief  读取MPU6050数据（使用修正的转换公式）
  * @param  hi2c: I2C句柄指针
  * @param  data: 存储读取数据的结构体指针
  * @retval HAL状态
  */
HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data *data) {
  uint8_t buffer[14];
  int16_t raw_accelX, raw_accelY, raw_accelZ;
  int16_t raw_gyroX, raw_gyroY, raw_gyroZ;
  float accel_fs, gyro_fs;  // 满量程范围
  
  // 读取14字节原始数据
  if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, ACCEL_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 14, 100) != HAL_OK) {
    return HAL_ERROR;
  }
  
  // 提取原始加速度数据（16位有符号整数）
  raw_accelX = (int16_t)(buffer[0] << 8 | buffer[1]);
  raw_accelY = (int16_t)(buffer[2] << 8 | buffer[3]);
  raw_accelZ = (int16_t)(buffer[4] << 8 | buffer[5]);
  
  // 加速度转换公式：(原始值 / ADC满量程) * 2 * 满量程范围
  // 修正点：使用满量程范围计算，而非固定灵敏度，公式更直观且不易出错
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
  
  // 温度转换（公式不变，来自数据手册）
  int16_t tempRaw = (int16_t)(buffer[6] << 8 | buffer[7]);
  data->temp = (tempRaw / 340.0f) + 36.53f;
  
  // 提取原始陀螺仪数据（16位有符号整数）
  raw_gyroX = (int16_t)(buffer[8] << 8 | buffer[9]);
  raw_gyroY = (int16_t)(buffer[10] << 8 | buffer[11]);
  raw_gyroZ = (int16_t)(buffer[12] << 8 | buffer[13]);
  
  // 陀螺仪转换公式：[(原始值 / ADC满量程) * 2 * 满量程范围(°/s)] * 弧度系数
  // 修正点：先转换为°/s，再转为弧度/秒，步骤更清晰
  switch(current_gyro_range) {
    case GYRO_RANGE_250DPS: gyro_fs = GYRO_FS_250DPS; break;
    case GYRO_RANGE_500DPS: gyro_fs = GYRO_FS_500DPS; break;
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
  * @brief  向MPU6050寄存器写入数据
  * @param  hi2c: I2C句柄指针
  * @param  reg: 寄存器地址
  * @param  data: 要写入的数据
  * @retval HAL状态
  */
static HAL_StatusTypeDef MPU6050_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data) {
  uint8_t txData[2] = {reg, data};
  return HAL_I2C_Master_Transmit(hi2c, MPU6050_ADDR, txData, 2, 100);
}

/**
  * @brief  从MPU6050寄存器读取数据
  * @param  hi2c: I2C句柄指针
  * @param  reg: 寄存器地址
  * @param  data: 存储读取数据的指针
  * @retval HAL状态
  */
static HAL_StatusTypeDef MPU6050_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data) {
  if (HAL_I2C_Master_Transmit(hi2c, MPU6050_ADDR, &reg, 1, 100) != HAL_OK) {
    return HAL_ERROR;
  }
  
  return HAL_I2C_Master_Receive(hi2c, MPU6050_ADDR, data, 1, 100);
}


/* USER CODE BEGIN 1 */
HAL_StatusTypeDef MPU6050_ZERO_MOTION_INIT(I2C_HandleTypeDef *hi2c, int THRESHOLD, int DURATION) {
	if(MPU6050_WriteReg(hi2c, ZRMOT_THR, THRESHOLD)!= HAL_OK){
		return HAL_ERROR;
	}
	if(MPU6050_WriteReg(hi2c, ZRMOT_DUR, DURATION)!= HAL_OK){
		return HAL_ERROR;
	}
	if(MPU6050_WriteReg(hi2c, CONFIG, 0x04)!= HAL_OK){
		return HAL_ERROR;
	}
	if(MPU6050_WriteReg(hi2c, ACCEL_CONFIG_REG, 0x1C)!= HAL_OK){
		return HAL_ERROR;
	}
	if(MPU6050_WriteReg(hi2c, INT_PIN_CFG, 0x30)!= HAL_OK){
		return HAL_ERROR;
	}
	if(MPU6050_WriteReg(hi2c, INT_ENABLE, 0x40)!= HAL_OK){
		return HAL_ERROR;
	}
	return HAL_OK;
}
/* USER CODE END 1 */

uint8_t CHECK_MPU6050_INTERRUPT(I2C_HandleTypeDef *hi2c){
	uint8_t STATUS;
	int counter = 0;
	int timer = 0;
	while(timer < 600){
		MPU6050_ReadReg(hi2c, INT_STATUS, &STATUS);
		if(STATUS & 0x40) // 0x40 = 01000000b (第6位)
			{
				counter++;
				timer++;
			}
			else
			{
				counter--;
				timer++;
			}
			HAL_Delay(100);
	}
	if(counter >= 0){
		return 1; //处于静止状态
	}
	else{
		return 0; //处于运动状态
	}
}
