
#ifndef __TAP_DETECTION_H
#define __TAP_DETECTION_H

#include "stdint.h"
#include "main.h"

typedef enum {
    TAP_STATE_IDLE,        // 空闲状�?
    TAP_STATE_POTENTIAL,   // 潜在点击状�?(初�?��?��?)
    TAP_STATE_DIRECTION,   // 方向�?认状�?
    TAP_STATE_CONFIRMED,   // 点击�?认状�?
    TAP_STATE_SETTLE       // 稳定状�?(防抖�?)
} TapState;

typedef enum {
    DIR_X_POS,  // X+
    DIR_X_NEG,  // X-
    DIR_Y_POS,  // Y+
    DIR_Y_NEG,  // Y-
    DIR_Z_POS,  // Z+
    DIR_Z_NEG,  // Z-
    DIR_UNKNOWN // 方向�?�?
} AccelDirection;

// 点击检测参数配�?
#define SEND_INTERVAL_MS     500    // 数据发送间�?
#define TAP_DETECT_INTERVAL  5      // 点击检测间�?(5ms/�?)
#define TAP_MAX_DURATION     200    // 点击最大持�?时间(200ms)
#define TAP_ACCEL_THRESHOLD  1.8f   // 点击加速度阈�?
#define TAP_GYRO_THRESHOLD   1.0f   // 点击陀螺仪阈�?
#define TAP_PEAK_RATIO       1.05f   // 峰�?/平均值比�?
#define DIR_CONSIST_COUNT    2      // 方向一致�?�数
#define DIR_CHANGE_THRESHOLD 3      // 方向变化阈�?
#define TAP_SETTLE_TIME      500    // 稳定时间
#define SWING_SMOOTH_THRESHOLD 0.5f // 挥动平滑阈�?
#define REBOUND_ACCEL_THRESHOLD 1.2f // 回弹判定阈�?
#define DIR_CHANGE_MIN_GAP 0.4f     // 方向变化最小幅�?

void DetectTap(ICM20602_Data *data);
float CalcVectorMag(float x, float y, float z);
AccelDirection GetMainDirection(float x, float y, float z, AccelDirection lastDir);
float CalcAccelChangeRate(float current, float previous);
uint8_t GetMotionFlag(void);
void ClearMotionFlag(void);

#endif /* __TAP_DETECTION_H */
