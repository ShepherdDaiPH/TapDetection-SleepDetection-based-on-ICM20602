#include "ina226.h"

static const uint16_t defaultcalibration = 0x0A00;
static const uint16_t defaultconfig = 0x4127;
static const uint16_t low_power_config = 0x44FF;

/**
  * @brief  д�Ĵ������Ĵ�����ַ 8 λ������ 16 λ�����
  */
HAL_StatusTypeDef INA226_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t data) {
    uint8_t txData[3];
    txData[0] = reg;                // �Ĵ�����ַ
    txData[1] = (data >> 8) & 0xFF; // ���ֽ� (MSB)
    txData[2] = data & 0xFF;        // ���ֽ� (LSB)

    return HAL_I2C_Master_Transmit(hi2c, INA226_ADDR, txData, 3, 100);
}

/**
  * @brief  ���Ĵ������Ĵ�����ַ 8 λ���������� 16 λ�����
  */
HAL_StatusTypeDef INA226_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *data) {
    if (HAL_I2C_Master_Transmit(hi2c, INA226_ADDR, &reg, 1, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    uint8_t rxData[2];
    if (HAL_I2C_Master_Receive(hi2c, INA226_ADDR, rxData, 2, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    *data = (rxData[0] << 8) | rxData[1]; // ��������
    return HAL_OK;
}

/**
  * @brief  INA226��ʼ��
  */
HAL_StatusTypeDef INA226_Init(I2C_HandleTypeDef *hi2c, Avg avg, Vbusct vbusct, Vshct vshct, Mode mode) {
	uint16_t config = (0 << 15)          // RST=0
									| (avg << 9)        // AVG
									| (vbusct << 6)     // VBUSCT
									| (vshct << 3)      // VSHCT
									| (mode << 0);      // MODE
	
	if(INA226_WriteReg(hi2c, CONFIG_REG, defaultconfig) != HAL_OK) return HAL_ERROR;	
	if(INA226_WriteReg(hi2c, CALIB_REG, defaultcalibration) != HAL_OK) return HAL_ERROR;
	
	return HAL_OK;
}

/**
 * @brief ��ȡINA226����
 */
HAL_StatusTypeDef INA226_ReadData(I2C_HandleTypeDef *hi2c, INA226_Data *data){
	uint16_t raw_current   = 0;
	if(INA226_ReadReg(hi2c, CUR_REG, &raw_current) != HAL_OK) return HAL_ERROR;
	int16_t current_detected = (int16_t)raw_current;
	float current_real = current_detected * CURRENT_LSB;
	
	uint16_t raw_shunt_voltage  = 0;
	if(INA226_ReadReg(hi2c, SHUNT_VOLTAGE_REG, &raw_shunt_voltage) != HAL_OK) return HAL_ERROR;	
	int16_t shunt_voltage_detected = (int16_t)raw_shunt_voltage;
	float real_shunt_voltage = shunt_voltage_detected * SHUNT_VOLTAGE_LSB;
	
	uint16_t bus_voltage_detected  = 0;
	if(INA226_ReadReg(hi2c, BUS_VOLTAGE_REG, &bus_voltage_detected) != HAL_OK) return HAL_ERROR;	
	float real_bus_voltage = bus_voltage_detected * BUS_VOLTAGE_LSB;	

	uint16_t power_consumption_detected  = 0;
	if(INA226_ReadReg(hi2c, PWR_REG, &power_consumption_detected) != HAL_OK) return HAL_ERROR;	
	float real_power_consumption = power_consumption_detected * POWER_LSB;
	
	data->current           = current_real;
	data->shunt_voltage     = real_shunt_voltage;
	data->bus_voltage       = real_bus_voltage;
	data->power_consumption = real_power_consumption;
	
	return HAL_OK;
}

/**
 * @brief 	����INA226Ϊ�͹���ģʽ
 */
HAL_StatusTypeDef INA226_EnterLowPowerMode(I2C_HandleTypeDef *hi2c) {
	return INA226_WriteReg(hi2c, CONFIG_REG, low_power_config);
}
