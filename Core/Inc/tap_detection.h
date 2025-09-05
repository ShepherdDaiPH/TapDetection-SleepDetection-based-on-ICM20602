/* tap_detection.h */
#ifndef TAP_DETECTION_H
#define TAP_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "icm20602.h"

// =============== 宏定义（与原代码一致）===============
#define SEND_INTERVAL_MS     500    // 数据发送间隔(ms)
#define TAP_DETECT_INTERVAL  5      // 拍击检测间隔(ms)
#define TAP_MAX_DURATION     200    // 拍击最大持续时间(ms)
#define TAP_ACCEL_THRESHOLD  1.8f   // 拍击加速度阈值(g)
#define TAP_GYRO_THRESHOLD   1.0f   // 拍击陀螺仪阈值(rad/s)
#define TAP_PEAK_RATIO       1.05f  // 峰值/平均值比例
#define DIR_CONSIST_COUNT    2      // 方向一致计数阈值
#define DIR_CHANGE_THRESHOLD 3      // 方向变化阈值
#define TAP_SETTLE_TIME      500    // 稳定时间(ms)
#define SWING_SMOOTH_THRESHOLD 0.5f // 挥动平滑阈值
#define REBOUND_ACCEL_THRESHOLD 1.2f // 回弹判定阈值(g)
#define DIR_CHANGE_MIN_GAP 0.4f     // 方向变化最小幅度(g)

// 状态枚举（保持原名）
typedef enum {
    TAP_STATE_IDLE,
    TAP_STATE_POTENTIAL,
    TAP_STATE_DIRECTION,
    TAP_STATE_CONFIRMED,
    TAP_STATE_SETTLE
} TapState;

typedef enum {
    DIR_X_POS,
    DIR_X_NEG,
    DIR_Y_POS,
    DIR_Y_NEG,
    DIR_Z_POS,
    DIR_Z_NEG,
    DIR_UNKNOWN
} AccelDirection;

// 全局变量声明（外部使用）
extern char uartBuffer[300];
extern TapState tapState;
extern uint32_t lastTapTime;
extern uint32_t tapStartTime;
extern float tapMaxAccel;
extern float tapAvgAccel;
extern uint8_t accelSampleCount;
extern AccelDirection mainDir;
extern uint8_t dirConsistCount;
extern uint8_t dirChangeCount;
extern float lastAccelMag;
extern uint8_t motionFlag;

// 函数声明
void DetectTap(ICM20602_Data *data);
float CalcVectorMag(float x, float y, float z);
AccelDirection GetMainDirection(float x, float y, float z, AccelDirection lastDir);
float CalcAccelChangeRate(float current, float previous);

#endif /* TAP_DETECTION_H */
