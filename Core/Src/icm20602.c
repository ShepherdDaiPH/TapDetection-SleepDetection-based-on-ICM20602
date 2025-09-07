#include "icm20602.h"

// ��Ҫ��������
static SystemState POWER_MODE = NORMAL_MODE;
static uint8_t low_power_mode_1    = 0x28;
static uint8_t low_power_mode_2    = 0x07;
static uint8_t normal_power_mode = 0x00;
static float init_sum_x = 0.0f;
static float init_sum_y = 0.0f;
static float init_sum_z = 0.0f;
static SensorState ICM20602_SensorState;

// ���ٶȼ����̶�Ӧ�������̷�Χ��g��
#define ACCEL_FS_2G     2.0f
#define ACCEL_FS_4G     4.0f
#define ACCEL_FS_8G     8.0f
#define ACCEL_FS_16G    16.0f

// ���������̶�Ӧ�������̷�Χ����/s��
#define GYRO_FS_250DPS  250.0f
#define GYRO_FS_500DPS  500.0f
#define GYRO_FS_1000DPS 1000.0f
#define GYRO_FS_2000DPS 2000.0f

// 16λADC�����̷�Χ
#define ADC_FULL_SCALE  32768.0f

// �Ƕ�ת����ϵ������/180��
#define DEG_TO_RAD      0.017453292519943295f

AccelRange current_accel_range;
GyroRange current_gyro_range;

HAL_StatusTypeDef ICM20602_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data);

/**
  * @brief  ��ʼ�� ICM20602
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
	
	if (ICM20602_WriteReg(hi2c, INT_ENABLE, 0x00) != HAL_OK) {
		return HAL_ERROR;
	}
  
  return HAL_OK;
}

/**
  * @brief  ��ȡ ICM20602 ����
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
  * @brief  д�Ĵ���
  */
HAL_StatusTypeDef ICM20602_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data) {
  uint8_t txData[2] = {reg, data};
  return HAL_I2C_Master_Transmit(hi2c, ICM20602_ADDR, txData, 2, 100);
}

/**
  * @brief  ���Ĵ���
  */
HAL_StatusTypeDef ICM20602_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data) {
  if (HAL_I2C_Master_Transmit(hi2c, ICM20602_ADDR, &reg, 1, 100) != HAL_OK) {
    return HAL_ERROR;
  }
  return HAL_I2C_Master_Receive(hi2c, ICM20602_ADDR, data, 1, 100);
}

/**
  * @brief  ����͹���ģʽ
  */
HAL_StatusTypeDef ICM20602_ENTER_LOW_POWER_MODE(I2C_HandleTypeDef *hi2c, uint8_t THRESHOLD) 
{   
    // Step1: ȷ�����ٶȼ��ڹ����������ǹر�
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x08) != HAL_OK) return HAL_ERROR; // TEMP_DIS=1, CYCLE=0
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_2_REG, 0x07) != HAL_OK) return HAL_ERROR; // Gyro off, Accel on

    // Step2: ���ü��ٶȼ� (�͹����˲�)
    if(ICM20602_WriteReg(hi2c, ACCEL_CONFIG2, (1<<3) | 0x01) != HAL_OK) return HAL_ERROR;

    // Step3: WOM �ж�ʹ��
    if(ICM20602_WriteReg(hi2c, INT_ENABLE, 0xE0) != HAL_OK) return HAL_ERROR;

    // Step4: ������ֵ
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_X_THR, THRESHOLD) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Y_THR, THRESHOLD) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Z_THR, THRESHOLD) != HAL_OK) return HAL_ERROR;

    // Step5: �����ж�ģʽ
    if(ICM20602_WriteReg(hi2c, ACCEL_INTEL_CTRL, (1<<7) | (1<<6)) != HAL_OK) return HAL_ERROR;

    // Step6: ���ò�����
    if(ICM20602_WriteReg(hi2c, SMPLRT_DIV, 19) != HAL_OK) return HAL_ERROR;

    // Step7: ������͹���ѭ��ģʽ
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x28) != HAL_OK) return HAL_ERROR; // CYCLE=1

    POWER_MODE = LOW_POWER_MODE;

    return HAL_OK;
}


/**
  * @brief  �˳��͹���ģʽ
  */
HAL_StatusTypeDef ICM20602_EXIT_LOW_POWER_MODE(I2C_HandleTypeDef *hi2c)
{
    // Step1: �˳�ѭ��ģʽ (CYCLE=0, TEMP_DIS=0/1 ��������)
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_1_REG, 0x00) != HAL_OK) return HAL_ERROR; 
    // �ϵ��Ĭ��ֵ 0x00: ʱ��=�ڲ�8MHz, CYCLE=0, SLEEP=0
    // �������Ҫ�ر��¶ȣ������� 0x08 (TEMP_DIS=1)

    // Step2: �ر� WOM �ж�
    if(ICM20602_WriteReg(hi2c, INT_ENABLE, 0x00) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_INTEL_CTRL, 0x00) != HAL_OK) return HAL_ERROR;

    // Step3: �ָ����ٶȼ����� (����ģʽ�˲�)
    if(ICM20602_WriteReg(hi2c, ACCEL_CONFIG2, 0x07) != HAL_OK) return HAL_ERROR;  
    // Ĭ�� 0x07: ACCEL_FCHOICE_B=0, A_DLPF_CFG=111 (BW=~5Hz)
    
    // Step4: ��� WOM ��ֵ
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_X_THR, 0x00) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Y_THR, 0x00) != HAL_OK) return HAL_ERROR;
    if(ICM20602_WriteReg(hi2c, ACCEL_WOM_Z_THR, 0x00) != HAL_OK) return HAL_ERROR;

    // Step5: �ָ������� (������Ӧ����Ҫ�������������ֵ)
    if(ICM20602_WriteReg(hi2c, SMPLRT_DIV, 0x07) != HAL_OK) return HAL_ERROR; 
    // ��Ӧ������ = 1kHz / (1+7) = 125Hz

    // Step6: �ָ������������� (gyro + accel ȫ��)
    if(ICM20602_WriteReg(hi2c, PWR_MGMT_2_REG, 0x00) != HAL_OK) return HAL_ERROR; 
    // 0x00 ��ʾ accel+gyro ȫ��

    POWER_MODE = NORMAL_MODE;

    return HAL_OK;
}
