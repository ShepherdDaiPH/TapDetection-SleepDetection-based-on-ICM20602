#ifndef __TAP_DETECTION_H
#define __TAP_DETECTION_H

#include "icm20602.h"

typedef enum {
    TAP_STATE_IDLE,        // 空闲状态
    TAP_STATE_POTENTIAL,   // 潜在点击状态(初步检测)
    TAP_STATE_DIRECTION,   // 方向确认状态
    TAP_STATE_CONFIRMED,   // 点击确认状态
    TAP_STATE_SETTLE       // 稳定状态(防抖动)
} TapState;

typedef enum {
    DIR_X_POS,  // X+
    DIR_X_NEG,  // X-
    DIR_Y_POS,  // Y+
    DIR_Y_NEG,  // Y-
    DIR_Z_POS,  // Z+
    DIR_Z_NEG,  // Z-
    DIR_UNKNOWN // 方向未知
} AccelDirection;

// 点击检测参数配置
#define SEND_INTERVAL_MS     500    // 数据发送间隔
#define TAP_DETECT_INTERVAL  5      // 点击检测间隔(5ms/次)
#define TAP_MAX_DURATION     200    // 点击最大持续时间(200ms)
#define TAP_ACCEL_THRESHOLD  1.8f   // 点击加速度阈值
#define TAP_GYRO_THRESHOLD   1.0f   // 点击陀螺仪阈值
#define TAP_PEAK_RATIO       1.05f   // 峰值/平均值比例
#define DIR_CONSIST_COUNT    2      // 方向一致计数
#define DIR_CHANGE_THRESHOLD 3      // 方向变化阈值
#define TAP_SETTLE_TIME      500    // 稳定时间
#define SWING_SMOOTH_THRESHOLD 0.5f // 挥动平滑阈值
#define REBOUND_ACCEL_THRESHOLD 1.2f // 回弹判定阈值
#define DIR_CHANGE_MIN_GAP 0.4f     // 方向变化最小幅度

extern TapState tapState;
extern uint32_t lastTapTime;       // 上次点击时间
extern uint32_t tapStartTime;      // 点击开始时间
extern float tapMaxAccel;       // 点击过程最大加速度
extern float tapAvgAccel;       // 点击过程平均加速度
extern uint8_t accelSampleCount;   // 加速度采样次数
extern AccelDirection mainDir;
extern uint8_t dirConsistCount;    // 方向一致计数
extern uint8_t dirChangeCount;     // 方向变化计数
extern float lastAccelMag;      // 上一次加速度合大小

void SystemClock_Config(void);
uint8_t DetectTap(ICM20602_Data *data);
float CalcVectorMag(float x, float y, float z);
AccelDirection GetMainDirection(float x, float y, float z, AccelDirection lastDir);
float CalcAccelChangeRate(float current, float previous);

#endif /* __TAP_DETECTION_H */
