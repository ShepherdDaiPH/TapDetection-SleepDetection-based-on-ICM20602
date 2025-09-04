/* tap_detection.c */
#include "tap_detection.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"

// =============== 保持原样：全局变量定义 ===============
TapState tapState = TAP_STATE_IDLE;
TickType_t lastTapTime = 0;
TickType_t tapStartTime = 0;
float tapMaxAccel = 0.0f;
float tapAvgAccel = 0.0f;
uint8_t accelSampleCount = 0;
AccelDirection mainDir = DIR_UNKNOWN;
uint8_t dirConsistCount = 0;
uint8_t dirChangeCount = 0;
float lastAccelMag = 0.0f;
uint8_t tapFlag = 0;
uint8_t motionFlag = 0;

// =============== 函数实现（完全保持原逻辑）===============
float CalcVectorMag(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

AccelDirection GetMainDirection(float x, float y, float z, AccelDirection lastDir) {
    float absX = fabs(x);
    float absY = fabs(y);
    float absZ = fabs(z);
    AccelDirection currDir = DIR_UNKNOWN;

    if (absX >= absY && absX >= absZ) currDir = x > 0 ? DIR_X_POS : DIR_X_NEG;
    else if (absY >= absX && absY >= absZ) currDir = y > 0 ? DIR_Y_POS : DIR_Y_NEG;
    else currDir = z > 0 ? DIR_Z_POS : DIR_Z_NEG;

    if (currDir != lastDir && lastDir != DIR_UNKNOWN) {
        float lastAccel = 0.0f;
        float currAccel = 0.0f;

        switch (lastDir) {
            case DIR_X_POS: case DIR_X_NEG: lastAccel = fabs(x); break;
            case DIR_Y_POS: case DIR_Y_NEG: lastAccel = fabs(y); break;
            case DIR_Z_POS: case DIR_Z_NEG: lastAccel = fabs(z); break;
            default: break;
        }
        switch (currDir) {
            case DIR_X_POS: case DIR_X_NEG: currAccel = fabs(x); break;
            case DIR_Y_POS: case DIR_Y_NEG: currAccel = fabs(y); break;
            case DIR_Z_POS: case DIR_Z_NEG: currAccel = fabs(z); break;
            default: break;
        }

        if (fabs(currAccel - lastAccel) < DIR_CHANGE_MIN_GAP) {
            currDir = lastDir;
        }
    }
    return currDir;
}

float CalcAccelChangeRate(float current, float previous) {
    return fabs(current - previous) / TAP_DETECT_INTERVAL;
}

void DetectTap(ICM20602_Data *data) {
    TickType_t currTime = xTaskGetTickCount();
    float accelMag = CalcVectorMag(data->accelX, data->accelY, data->accelZ);
    float gyroMag = CalcVectorMag(data->gyroX, data->gyroY, data->gyroZ);
    AccelDirection currDir = GetMainDirection(data->accelX, data->accelY, data->accelZ, mainDir);
    float accelChangeRate = CalcAccelChangeRate(accelMag, lastAccelMag);
    lastAccelMag = accelMag;

    switch (tapState) {
        case TAP_STATE_IDLE:
            if (accelMag > TAP_ACCEL_THRESHOLD && gyroMag > TAP_GYRO_THRESHOLD) {
                tapStartTime = currTime;
                tapMaxAccel = accelMag;
                tapAvgAccel = accelMag;
                accelSampleCount = 1;
                mainDir = currDir;
                dirConsistCount = 1;
                dirChangeCount = 0;
                tapState = TAP_STATE_POTENTIAL;
            }
            break;

        case TAP_STATE_POTENTIAL:
            if (currTime - tapStartTime > TAP_MAX_DURATION) {
                sprintf(uartBuffer, "Swing (Too long: %lu ms)\r\n", currTime - tapStartTime);
                HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
                tapState = TAP_STATE_IDLE;
                break;
            }

            if (currDir != mainDir) {
                dirChangeCount++;
                mainDir = currDir;
                if (dirChangeCount >= DIR_CHANGE_THRESHOLD) {
                    sprintf(uartBuffer, "Swing (Dir changes: %d)\r\n", dirChangeCount);
                    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
                    tapState = TAP_STATE_IDLE;
                    break;
                }
            }

            if (accelMag > TAP_ACCEL_THRESHOLD * 0.5f && gyroMag > TAP_GYRO_THRESHOLD * 0.5f) {
                accelSampleCount++;
                tapAvgAccel = (tapAvgAccel * (accelSampleCount - 1) + accelMag) / accelSampleCount;
                if (accelMag > tapMaxAccel) {
                    tapMaxAccel = accelMag;
                }

                if (currDir == mainDir) {
                    dirConsistCount++;
                    if (dirConsistCount >= DIR_CONSIST_COUNT) {
                        tapState = TAP_STATE_DIRECTION;
                    }
                } else {
                    dirConsistCount = 0;
                }
            } else {
                tapState = TAP_STATE_IDLE;
            }
            break;

        case TAP_STATE_DIRECTION:
            if (tapMaxAccel > tapAvgAccel * TAP_PEAK_RATIO && 
                tapMaxAccel > TAP_ACCEL_THRESHOLD * 1.2f && 
                accelChangeRate > SWING_SMOOTH_THRESHOLD) {
                tapFlag = 1;
                motionFlag = 1;
                lastTapTime = currTime;
                tapState = TAP_STATE_CONFIRMED;
                sprintf(uartBuffer, "Tap (Peak: %.2fg, Dur: %lu ms)\r\n", 
                        tapMaxAccel, currTime - tapStartTime);
                HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
            } else {
                sprintf(uartBuffer, "Swing (Smooth: %.2f, Ratio: %.2f)\r\n", 
                        accelChangeRate, tapMaxAccel / tapAvgAccel);
                HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
                motionFlag = 1;
                tapState = TAP_STATE_IDLE;
            }
            break;

        case TAP_STATE_CONFIRMED:
            tapState = TAP_STATE_SETTLE;
            break;

        case TAP_STATE_SETTLE:
            if (currTime - lastTapTime > TAP_SETTLE_TIME && accelMag < REBOUND_ACCEL_THRESHOLD) {
                tapState = TAP_STATE_IDLE;
            }
            break;

        default:
            tapState = TAP_STATE_IDLE;
            break;
    }
}
