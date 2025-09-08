#include "main.h"
#include "tap_detection.h"
#include "math.h"
#include "string.h"
#include "usart.h"
#include "stdio.h"

static TapState tapState = TAP_STATE_IDLE;
static uint32_t lastTapTime = 0;       // 上次点击时间
static uint32_t tapStartTime = 0;      // 点击开始时间
static float tapMaxAccel = 0.0f;       // 点击过程最大加速度
static float tapAvgAccel = 0.0f;       // 点击过程平均加速度
static uint8_t accelSampleCount = 0;   // 加速度采样次数
static AccelDirection mainDir = DIR_UNKNOWN;
static uint8_t dirConsistCount = 0;    // 方向一致计数
static uint8_t dirChangeCount = 0;     // 方向变化计数
static float lastAccelMag = 0.0f;      // 上一次加速度合大小
static uint8_t motionFlag = 0;

/**
 * @brief 计算三维向量的合大小
 */
float CalcVectorMag(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

/**
 * @brief 判断加速度的主方向（带历史方向参考，过滤微小波动）
 */
AccelDirection GetMainDirection(float x, float y, float z, AccelDirection lastDir) {
    float absX = fabs(x);
    float absY = fabs(y);
    float absZ = fabs(z);
    AccelDirection currDir = DIR_UNKNOWN;

    // 确定当前方向
    if (absX >= absY && absX >= absZ) currDir = x > 0 ? DIR_X_POS : DIR_X_NEG;
    else if (absY >= absX && absY >= absZ) currDir = y > 0 ? DIR_Y_POS : DIR_Y_NEG;
    else currDir = z > 0 ? DIR_Z_POS : DIR_Z_NEG;

    // 过滤微小方向变化（如拍击回弹）
    if (currDir != lastDir && lastDir != DIR_UNKNOWN) {
        float lastAccel = 0.0f;
        float currAccel = 0.0f;
        
        // 提取上次方向和当前方向的加速度绝对值
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
        
        // 差值小于最小幅度 → 视为同一方向
        if (fabs(currAccel - lastAccel) < DIR_CHANGE_MIN_GAP) {
            currDir = lastDir;
        }
    }
    return currDir;
}

/**
 * @brief 计算加速度变化率
 */
float CalcAccelChangeRate(float current, float previous) {
    return fabs(current - previous) / TAP_DETECT_INTERVAL;
}

/**
 * @brief 点击检测核心函数
 */
void DetectTap(ICM20602_Data *data) {
    uint32_t currTime = HAL_GetTick();
    float accelMag = CalcVectorMag(data->accelX, data->accelY, data->accelZ);
    float gyroMag = CalcVectorMag(data->gyroX, data->gyroY, data->gyroZ);
    AccelDirection currDir = GetMainDirection(data->accelX, data->accelY, data->accelZ, mainDir);
    float accelChangeRate = CalcAccelChangeRate(accelMag, lastAccelMag);
    lastAccelMag = accelMag;

    switch (tapState) {
        case TAP_STATE_IDLE:
            // 检测到潜在点击信号
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
            // 检查持续时间是否过长
            if (currTime - tapStartTime > TAP_MAX_DURATION) {
                sprintf(uartBuffer, "Swing (Too long: %u ms)\r\n", currTime - tapStartTime);
                HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
                tapState = TAP_STATE_IDLE;
								motionFlag = 1;
                break;
            }

            // 检查方向变化
            if (currDir != mainDir) {
                dirChangeCount++;
                mainDir = currDir;
                if (dirChangeCount >= DIR_CHANGE_THRESHOLD) {
                    sprintf(uartBuffer, "Swing (Dir changes: %d)\r\n", dirChangeCount);
                    HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
                    tapState = TAP_STATE_IDLE;
										motionFlag = 1;
                    break;
                }
            }

            // 检查信号是否持续
            if (accelMag > TAP_ACCEL_THRESHOLD * 0.5f && gyroMag > TAP_GYRO_THRESHOLD * 0.5f) {
                accelSampleCount++;
                tapAvgAccel = (tapAvgAccel * (accelSampleCount - 1) + accelMag) / accelSampleCount;
                if (accelMag > tapMaxAccel) {
                    tapMaxAccel = accelMag;
                }

                // 检查方向一致性
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
            // 判断是否为有效点击
            if (tapMaxAccel > tapAvgAccel * TAP_PEAK_RATIO && 
                tapMaxAccel > TAP_ACCEL_THRESHOLD * 1.2f &&
                accelChangeRate > SWING_SMOOTH_THRESHOLD){
								motionFlag = 1;
                lastTapTime = currTime;
                tapState = TAP_STATE_CONFIRMED;
                sprintf(uartBuffer, "Tap (Peak: %.2fg, Dur: %u ms)\r\n", 
                        tapMaxAccel, currTime - tapStartTime);
                HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
            } else {
                sprintf(uartBuffer, "Swing (Smooth: %.2f, Ratio: %.2f)\r\n", accelChangeRate, tapMaxAccel / tapAvgAccel);
                HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
                tapState = TAP_STATE_IDLE;
								motionFlag = 1;
            }
            break;

        case TAP_STATE_CONFIRMED:
            tapState = TAP_STATE_SETTLE;
            break;

        case TAP_STATE_SETTLE:
            // 等待回弹完全结束
            if (currTime - lastTapTime > TAP_SETTLE_TIME && accelMag < REBOUND_ACCEL_THRESHOLD) {
                tapState = TAP_STATE_IDLE;
            }
            break;

        default:
            tapState = TAP_STATE_IDLE;
            break;
    }
}

uint8_t GetMotionFlag(void){
	return motionFlag;
}

void ClearMotionFlag(void){
	motionFlag = 0;
}