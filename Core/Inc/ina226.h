#ifndef __INA226_H
#define __INA226_H

#include "stm32f1xx_hal.h"

#define INA226_ADDR			      (0x40 << 1)

#define CONFIG_REG             0x00
#define SHUNT_VOLTAGE_REG      0x01
#define BUS_VOLTAGE_REG        0x02
#define PWR_REG                0x03
#define CUR_REG                0x04
#define CALIB_REG              0x05

/* ===================== 枚举类型定义 ===================== */
/* 平均次数 AVG */
typedef enum {
    AVG_1    = 0x0,
    AVG_4    = 0x1,
    AVG_16   = 0x2,
    AVG_64   = 0x3,
    AVG_128  = 0x4,
    AVG_256  = 0x5,
    AVG_512  = 0x6,
    AVG_1024 = 0x7,
} Avg;

/* 总线电压转换时间 VBUSCT */
typedef enum {
    VBUSCT_140us   = 0x0,
    VBUSCT_204us   = 0x1,
    VBUSCT_332us   = 0x2,
    VBUSCT_588us   = 0x3,
    VBUSCT_1_1ms   = 0x4,
    VBUSCT_2_116ms = 0x5,
    VBUSCT_4_156ms = 0x6,
    VBUSCT_8_244ms = 0x7,
} Vbusct;

/* 分流电压转换时间 VSHCT */
typedef enum {
    VSHCT_140us   = 0x0,
    VSHCT_204us   = 0x1,
    VSHCT_332us   = 0x2,
    VSHCT_588us   = 0x3,
    VSHCT_1_1ms   = 0x4,
    VSHCT_2_116ms = 0x5,
    VSHCT_4_156ms = 0x6,
    VSHCT_8_244ms = 0x7,
} Vshct;

/* 工作模式 MODE */
typedef enum {
    MODE_POWER_DOWN0 =    0x0,
    MODE_SHUNT_TRIG  =    0x1,
    MODE_BUS_TRIG    =    0x2,
    MODE_SHUNT_BUS_TRIG = 0x3,
    MODE_POWER_DOWN1 =    0x4,
    MODE_SHUNT_CONT  =    0x5,
    MODE_BUS_CONT    =    0x6,
    MODE_SHUNT_BUS_CONT = 0x7,
} Mode;

typedef struct {
		float shunt_voltage;
		float bus_voltage;
		float current;
		float power_consumption;
} INA226_Data;

HAL_StatusTypeDef INA226_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *data);
HAL_StatusTypeDef INA226_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t data);
HAL_StatusTypeDef INA226_Init(I2C_HandleTypeDef *hi2c, Avg avg, Vbusct vbusct, Vshct vshct, Mode mode);
HAL_StatusTypeDef INA226_ReadData(I2C_HandleTypeDef *hi2c, INA226_Data *data);

#endif /* __INA226_H */
