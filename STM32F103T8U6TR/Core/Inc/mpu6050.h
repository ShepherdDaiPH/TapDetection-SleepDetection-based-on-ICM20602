/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mpu6050.h
  * @brief          : MPU6050 driver header file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MPU6050_H
#define __MPU6050_H

#include "stm32f1xx_hal.h"

// MPU6050 设备地址，7位地址。若AD0接高电平，改为0xD2（8位地址）
#define MPU6050_ADDR         0xD0  // 8位地址（0x68 << 1）

//数学变量定义
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define GRAVITY_G 1.0f
#define ALPHA_FILTER 0.8f
#define RAD_TO_DEG 57.29577951308232f


// 寄存器地址定义
#define SMPLRT_DIV_REG       0x19  // 采样率分频器寄存器
#define PWR_MGMT_1_REG       0x6B  // 电源管理1寄存器
#define WHO_AM_I_REG         0x75  // 设备ID寄存器
#define ACCEL_CONFIG_REG     0x1C  // 加速度计配置寄存器
#define GYRO_CONFIG_REG      0x1B  // 陀螺仪配置寄存器
#define ACCEL_XOUT_H_REG     0x3B  // 加速度计X轴高位寄存器
#define TEMP_OUT_H_REG       0x41  // 温度高位寄存器
#define GYRO_XOUT_H_REG      0x43  // 陀螺仪X轴高位寄存器
#define ZRMOT_THR            0x21  // 静止中断阈值寄存器
#define ZRMOT_DUR            0x22  // 静止中断时间寄存器
#define CONFIG               0x1A  // CONFIG寄存器 配置 数字低通滤波器（DLPF） 和 外部帧同步（FSYNC）
#define INT_PIN_CFG          0x37  // 终端引脚寄存器
#define INT_ENABLE           0x38  // 使能特定中断源
#define INT_STATUS           0x3A  // 中断状态寄存器

// 加速度计量程枚举
typedef enum {
    ACCEL_RANGE_2G    = 0x00,  // ±2g
    ACCEL_RANGE_4G    = 0x08,  // ±4g
    ACCEL_RANGE_8G    = 0x10,  // ±8g
    ACCEL_RANGE_16G   = 0x18   // ±16g
} AccelRange;

// 陀螺仪量程枚举
typedef enum {
    GYRO_RANGE_250DPS = 0x00,  // ±250°/s
    GYRO_RANGE_500DPS = 0x08,  // ±500°/s
    GYRO_RANGE_1000DPS = 0x10, // ±1000°/s
    GYRO_RANGE_2000DPS = 0x18  // ±2000°/s
} GyroRange;

// 存储MPU6050数据的结构体
typedef struct {
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;
    float temp;
} MPU6050_Data;


// 函数声明
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c, AccelRange accelRange, GyroRange gyroRange);
HAL_StatusTypeDef MPU6050_ReadData(I2C_HandleTypeDef *hi2c, MPU6050_Data *data);
HAL_StatusTypeDef MPU6050_ZERO_MOTION_INIT(I2C_HandleTypeDef *hi2c, int THR_ZERO_MOTION, int THR_DUR);
uint8_t CHECK_MPU6050_INTERRUPT(I2C_HandleTypeDef *hi2c);

#endif /* __MPU6050_H */
