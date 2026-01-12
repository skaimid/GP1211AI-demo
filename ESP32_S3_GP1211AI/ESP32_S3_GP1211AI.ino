#include <SPI.h>
#include <Wire.h>

// ==================== ESP32-S3 平台说明 ====================
// 本代码适用于 ESP32-S3 芯片，从 Arduino 版本迁移而来
// 主要改动：
// 1. 使用 ESP32 硬件定时器替代 TimerOne
// 2. 使用 LEDC PWM 控制器替代 analogWrite
// 3. 更新引脚定义为 ESP32-S3 GPIO
// 4. SPI 配置优化
// 5. 添加 AHT20 温湿度传感器支持 (I2C)
// ===========================================================

// 包含字库头文件
#include "../Arduino_GP1211AI/ASC1224.h"
#include "../Arduino_GP1211AI/ASC816.h"
#include "../Arduino_GP1211AI/ASC57.h"
#include "../Arduino_GP1211AI/my_img.h"
// #include "../Arduino_GP1211AI/CHINESE.h" // 如果需要中文，取消注释

// ==================== ESP32-S3 引脚定义 ====================
// ESP32-S3 GPIO 引脚配置（可根据实际硬件修改）
#define VFD_SIG_PIN  GPIO_NUM_37  // 信号控制
#define VFD_CLKG_PIN GPIO_NUM_38  // 栅极时钟
#define VFD_LAT_PIN  GPIO_NUM_39  // 数据锁存
#define VFD_BK_PIN   GPIO_NUM_40  // PWM 亮度控制


// ESP32-S3 HSPI 引脚（使用默认 HSPI）
#define VFD_CLKA_PIN GPIO_NUM_35  // SCK
#define VFD_SIA_PIN  GPIO_NUM_36  // MOSI

// #define VFD_SS_PIN   GPIO_NUM_10  // SS (可选，未使用)

// 电源管理引脚
#define HV_EN_PIN    GPIO_NUM_41  // 高压使能
#define FL_EN_PIN    GPIO_NUM_42  // 灯丝使能

// 按键引脚
#define K_U_PIN      GPIO_NUM_18  // 增加亮度
#define K_D_PIN      GPIO_NUM_19  // 减少亮度
#define K_M_PIN      GPIO_NUM_20  // 菜单

// I2C 引脚 (AHT20 温湿度传感器)
#define I2C_SDA_PIN  GPIO_NUM_4  // I2C 数据线
#define I2C_SCL_PIN  GPIO_NUM_5  // I2C 时钟线
#define I2C_FREQ     100000       // I2C 频率 100kHz

// AHT20 配置
#define AHT20_ADDR   0x38         // AHT20 I2C 地址

// ==================== LEDC PWM 配置 ====================
#define LEDC_FREQUENCY      5000        // 5kHz PWM 频率
#define LEDC_RESOLUTION     8           // 8位分辨率 (0-255)

// ==================== 硬件定时器配置 ====================
hw_timer_t *timer = NULL;               // 定时器句柄
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ==================== 宏定义 ====================
#define VFD_CLKG_STROBE() { \
    gpio_set_level(VFD_CLKG_PIN, 0); \
    gpio_set_level(VFD_CLKG_PIN, 1); \
}

#define VFD_LAT_STROBE() { \
    gpio_set_level(VFD_LAT_PIN, 1); \
    gpio_set_level(VFD_LAT_PIN, 0); \
}

// ==================== 全局变量 ====================
unsigned char DP_RAM[8][128];           // 显示缓冲 (1KB)
unsigned char DP_BUF[2064];             // SPI 点阵显示缓冲 (~2KB)

volatile unsigned char VFD_GRID_SCAN = 0;
volatile unsigned char *DP_BUF_POINT;
volatile unsigned char Disp_Brt_Data = 50; // 亮度 (0-255)

// 时间相关
unsigned long timerStartTime = 0;
char timeBuffer[10];

// SPI 对象
SPIClass *vspi = NULL;

// AHT20 温湿度数据
float temperature = 0.0;    // 温度 (°C)
float humidity = 0.0;       // 湿度 (%)
unsigned long lastSensorRead = 0;  // 上次读取传感器的时间
#define SENSOR_READ_INTERVAL 2000  // 传感器读取间隔 (ms)

// ==================== VFD 逻辑函数 ====================

// 将显存 DP_RAM 转换为 VFD 硬件所需的位流 DP_BUF
void Disp_Buf_Update(void) {
    unsigned char i = 0;
    unsigned char VFD_GRID;
    unsigned char VFD_GRID_TMP = 0;
    unsigned char Dp_Ram_Temp = 0;
    unsigned char Dp_Ram_Base_Addr = 0;
    unsigned char *Dp_Buf_Ptr;

    Dp_Buf_Ptr = DP_BUF;

    for (VFD_GRID = 0; VFD_GRID < 43; VFD_GRID++) {
        VFD_GRID_TMP = 42 - VFD_GRID;
        Dp_Ram_Base_Addr = (VFD_GRID_TMP >> 1) * 6;

        for (i = 0; i < 8; i++) {
            if ((VFD_GRID_TMP & 0x01)) { // 双数列
                Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 1) & 0x02);
                Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] << 3) & 0x08);
                Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] << 5) & 0x20);
                Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 6) & 0x80);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr + 4] & 0x02;
                Dp_Ram_Temp |= (((DP_RAM[i][Dp_Ram_Base_Addr + 3]) << 2) & 0x08);
                Dp_Ram_Temp |= (((DP_RAM[i][Dp_Ram_Base_Addr + 5]) << 3) & 0x20);
                Dp_Ram_Temp |= (((DP_RAM[i][Dp_Ram_Base_Addr + 4]) << 5) & 0x80);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>1)&0x02);
                Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+5]     &0x08;
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<2)&0x20);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<4)&0x80);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>3)&0x02);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>1)&0x08);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<1)&0x20);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])<<2)&0x80);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>4)&0x02);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>2)&0x08);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>1)&0x20);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<1)&0x80);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>5)&0x02);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>4)&0x08);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>2)&0x20);
                Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+3]     &0x80;
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;
            } else { // 单数列
                Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr+0]   &0x01;
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<2&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<4&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<5&0x40);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>1&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<1&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<2&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<4&0x40);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>2&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>1&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<1&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<3&0x40);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+0]>>4&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>2&0x04);
                Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+2]   &0x10;
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<1&0x40);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>5&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>3&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>2&0x10);
                Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+1]   &0x40;
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>6&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>5&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>3&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>1&0x40);
                *Dp_Buf_Ptr = Dp_Ram_Temp; Dp_Buf_Ptr++;
            }
        }
    }
}

// ==================== 绘图函数 ====================

void VFD_DISP_ASC57(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT) {
    for (unsigned char i = 0; i < 5; i++) {
        DP_RAM[VFD_X][VFD_Y * 8 + i] = pgm_read_byte(&ASC57[((DAT - 0x20) * 5) + i]);
    }
}

void VFD_DISP_ASC57_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR) {
    while (*STR) {
        VFD_DISP_ASC57(VFD_X, VFD_Y++, *STR++);
    }
}

void VFD_DISP_ASC816(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT) {
    for (unsigned char i = 0; i < 8; i++) {
        DP_RAM[VFD_X * 2][VFD_Y * 8 + i] = pgm_read_byte(&ASC816[((DAT - 0x20) << 4) + i]);
        DP_RAM[VFD_X * 2 + 1][VFD_Y * 8 + i] = pgm_read_byte(&ASC816[((DAT - 0x20) << 4) + 8 + i]);
    }
}

void VFD_DISP_ASC816_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR) {
    while (*STR) {
        VFD_DISP_ASC816(VFD_X, VFD_Y++, *STR++);
    }
}

void VFD_DISP_PIC_12864(const unsigned char *dat) {
    unsigned char i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 128; j++) {
            DP_RAM[i][j] = pgm_read_byte(dat++);
        }
    }
}

void DP_RAM_CLR(void) {
    memset(DP_RAM, 0, sizeof(DP_RAM));
}

// ==================== ESP32 硬件定时器中断 ====================
// 定时器中断服务程序（ISR）- 使用 IRAM_ATTR 确保代码在 RAM 中执行
void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timerMux);

    unsigned char cycle_cnt = 48;

    if (VFD_GRID_SCAN == 0) {
        DP_BUF_POINT = DP_BUF;
        VFD_GRID_SCAN = 43;

        // 帧同步序列
        gpio_set_level(VFD_SIG_PIN, 1);
        VFD_CLKG_STROBE();
        VFD_CLKG_STROBE();
        gpio_set_level(VFD_SIG_PIN, 0);
        VFD_CLKG_STROBE();
        VFD_CLKG_STROBE();
        VFD_CLKG_STROBE();
    }

    // 通过硬件 SPI 发送 48 字节数据
    while ((cycle_cnt--) > 0) {
        vspi->transfer(*DP_BUF_POINT++);
    }

    VFD_CLKG_STROBE();

    // 亮度控制 / 锁存序列
    // 使用 LEDC PWM 控制亮度
    ledcWrite(VFD_BK_PIN, 0);           // 消隐
    VFD_LAT_STROBE();                     // 锁存数据
    ledcWrite(VFD_BK_PIN, Disp_Brt_Data); // 恢复亮度

    VFD_GRID_SCAN--;

    portEXIT_CRITICAL_ISR(&timerMux);
}

// ==================== 时间显示函数 ====================

void Show_Timer(unsigned char row, unsigned char col) {
    unsigned long currentMillis = millis();
    unsigned long totalSeconds = (currentMillis - timerStartTime) / 1000;

    unsigned long seconds = totalSeconds % 60;
    unsigned long minutes = (totalSeconds / 60) % 60;
    unsigned long hours   = (totalSeconds / 3600);

    sprintf(timeBuffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    VFD_DISP_ASC57_STR(row, col, timeBuffer);
}

// ==================== AHT20 温湿度传感器函数 ====================

// ==================== 硬件测试诊断函数 ====================

/**
 * 硬件引脚测试 - 用于快速定位问题
 * 在串口监视器输入 't' 触发测试
 */
void RunHardwareTest() {
    Serial.println("\n========== VFD 硬件诊断测试 ==========\n");

    // 测试1: GPIO引脚状态
    Serial.println("[测试1] GPIO引脚状态:");
    Serial.printf("  HV_EN_PIN (GPIO%d): %s\n", HV_EN_PIN, digitalRead(HV_EN_PIN) ? "HIGH" : "LOW");
    Serial.printf("  FL_EN_PIN (GPIO%d): %s\n", FL_EN_PIN, digitalRead(FL_EN_PIN) ? "HIGH" : "LOW");
    Serial.printf("  当前亮度值: %d\n", Disp_Brt_Data);

    // 测试2: 强制开启所有电源（用于测试）
    Serial.println("\n[测试2] 强制开启VFD电源:");
    Serial.println("  → 灯丝电源开启...");
    digitalWrite(FL_EN_PIN, HIGH);
    delay(500);

    Serial.println("  → 高压电源开启...");
    digitalWrite(HV_EN_PIN, HIGH);
    delay(100);

    Serial.println("  → 设置最大亮度...");
    Disp_Brt_Data = 200;
    ledcWrite(VFD_BK_PIN, Disp_Brt_Data);
    delay(500);

    // 测试3: 填充全白屏幕
    Serial.println("\n[测试3] 显示全白测试图案...");
    DP_RAM_CLR();
    memset(DP_RAM, 0xFF, sizeof(DP_RAM));  // 全白
    Disp_Buf_Update();
    delay(2000);

    // 恢复
    Serial.println("\n[恢复] 恢复正常设置...");
    Disp_Brt_Data = 50;
    ledcWrite(VFD_BK_PIN, Disp_Brt_Data);
    DP_RAM_CLR();
    Disp_Buf_Update();

    Serial.println("\n========== 测试完成 ==========");
    Serial.println("如果看到屏幕闪烁或显示：");
    Serial.println("  ✓ 硬件连接正常");
    Serial.println("  ✓ 检查引脚定义是否正确");
    Serial.println("\n如果屏幕完全黑屏：");
    Serial.println("  ✗ 检查HV_EN/FL_EN引脚连接");
    Serial.println("  ✗ 检查电源电压（需要高压模块）");
    Serial.println("  ✗ 检查SPI接线（CLK/MOSI）\n");
}

/**
 * 初始化 AHT20 传感器
 * @return true: 初始化成功, false: 失败
 */
bool AHT20_Init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    delay(40);  // 等待传感器上电稳定

    // 发送初始化命令
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xBE);  // 初始化命令
    Wire.write(0x08);  // 参数1
    Wire.write(0x00);  // 参数2
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.printf("✗ AHT20 初始化失败 (错误代码: %d)\n", error);
        return false;
    }

    delay(10);
    Serial.println("✓ AHT20 初始化成功");
    return true;
}

/**
 * 触发 AHT20 测量
 * @return true: 触发成功, false: 失败
 */
bool AHT20_TriggerMeasurement() {
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xAC);  // 触发测量命令
    Wire.write(0x33);  // 参数1
    Wire.write(0x00);  // 参数2
    uint8_t error = Wire.endTransmission();

    return (error == 0);
}

/**
 * 读取 AHT20 温湿度数据
 * @param temp: 温度输出 (°C)
 * @param humi: 湿度输出 (%)
 * @return true: 读取成功, false: 失败
 */
bool AHT20_ReadData(float *temp, float *humi) {
    // 触发测量
    if (!AHT20_TriggerMeasurement()) {
        Serial.println("✗ AHT20 触发测量失败");
        return false;
    }

    // 等待测量完成 (典型值 80ms)
    delay(80);

    // 读取 6 字节数据
    uint8_t data[6];
    Wire.requestFrom(AHT20_ADDR, 6);

    if (Wire.available() != 6) {
        Serial.println("✗ AHT20 数据读取失败");
        return false;
    }

    for (int i = 0; i < 6; i++) {
        data[i] = Wire.read();
    }

    // 检查状态位 (bit[7] = 忙标志, 应该为 0)
    if (data[0] & 0x80) {
        Serial.println("✗ AHT20 忙碌中");
        return false;
    }

    // 计算湿度 (20位数据)
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) |
                            ((uint32_t)data[2] << 4) |
                            ((uint32_t)data[3] >> 4);
    *humi = (raw_humidity * 100.0) / 1048576.0;  // 2^20 = 1048576

    // 计算温度 (20位数据)
    uint32_t raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) |
                               ((uint32_t)data[4] << 8) |
                               ((uint32_t)data[5]);
    *temp = (raw_temperature * 200.0) / 1048576.0 - 50.0;

    return true;
}

/**
 * 在 VFD 屏幕上显示温湿度
 * @param row: 起始行 (0-7)
 * @param col: 起始列 (0-15)
 */
void Display_TempHumi(unsigned char row, unsigned char col) {
    char buffer[16];

    // 显示温度
    sprintf(buffer, "T:%5.1fC", temperature);
    VFD_DISP_ASC57_STR(row, col, buffer);

    // 显示湿度（下一行）
    sprintf(buffer, "H:%5.1f%%", humidity);
    VFD_DISP_ASC57_STR(row + 1, col, buffer);
}

// ==================== Setup & Loop ====================

void setup() {
    Serial.begin(115200);
    Serial.println("\n==========================================");
    Serial.println("ESP32-S3 GP1211AI VFD Display Driver");
    Serial.println("==========================================\n");

    // 1. 初始化 GPIO 引脚
    pinMode(VFD_LAT_PIN, OUTPUT);
    pinMode(VFD_CLKG_PIN, OUTPUT);
    pinMode(VFD_SIG_PIN, OUTPUT);
    pinMode(HV_EN_PIN, OUTPUT);
    pinMode(FL_EN_PIN, OUTPUT);

    // 按钮（使用内部上拉）
    pinMode(K_U_PIN, INPUT_PULLUP);
    pinMode(K_D_PIN, INPUT_PULLUP);
    pinMode(K_M_PIN, INPUT_PULLUP);

    // 2. 配置 LEDC PWM（替代 analogWrite）
    ledcAttach(VFD_BK_PIN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcWrite(VFD_BK_PIN, Disp_Brt_Data); // 使用初始亮度值 (50)

    Serial.println("✓ GPIO 初始化完成");
    Serial.println("✓ LEDC PWM 配置完成");

    // 3. 初始化 SPI（使用 HSPI）
    vspi = new SPIClass(HSPI);
    vspi->begin(VFD_CLKA_PIN, -1, VFD_SIA_PIN, -1); // SCK, MISO(-1), MOSI, SS(-1)
    vspi->beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));

    Serial.println("✓ SPI 初始化完成 (8MHz, LSB First)");

    // 4. 上电时序
    // VFD 正确的上电顺序：灯丝预热 → 启动数据刷新 → 开启高压
    digitalWrite(FL_EN_PIN, HIGH); // 灯丝开启
    digitalWrite(HV_EN_PIN, LOW);  // 高压关闭
    DP_RAM_CLR();

    Serial.println("✓ 灯丝预热中...");
    delay(500);  // 增加预热时间到500ms

    // 5. 初始化硬件定时器（必须在开启高压之前启动，确保数据就绪）
    // 新版 ESP32 Arduino Core v3.0+ API
    // 188us 周期 = 5319 Hz (1 / 0.000188)
    timer = timerBegin(5319);  // 频率 5319 Hz
    timerAttachInterrupt(timer, &onTimer); // 绑定中断函数
    timerAlarm(timer, 1, true, 0);  // 每次 tick 触发，自动重载，无限次

    Serial.println("✓ 硬件定时器启动 (188us 周期)");

    // 显示初始图像
    Disp_Buf_Update();

    // 注意：原代码逻辑 FL_EN=0 后 Delay 100ms 再 HV_EN=1
    digitalWrite(FL_EN_PIN, LOW); // 灯丝开启
    delay(100);

    // 灯丝保持开启，开启高压
    digitalWrite(HV_EN_PIN, HIGH); // 高压开启
    delay(100);

    Serial.println("✓ 高压开启，VFD 准备就绪\n");

    // 6. 初始化 AHT20 温湿度传感器
    if (AHT20_Init()) {
        // 首次读取传感器数据
        if (AHT20_ReadData(&temperature, &humidity)) {
            Serial.printf("  温度: %.1f°C\n", temperature);
            Serial.printf("  湿度: %.1f%%\n\n", humidity);
        }
    } else {
        Serial.println(" AHT20 传感器未检测到，温湿度功能不可用\n");
    }

    Serial.println("按键功能:");
    Serial.println("  K_U (GPIO18): 增加亮度");
    Serial.println("  K_D (GPIO19): 减少亮度");
    Serial.println("  K_M (GPIO20): 菜单");
    Serial.println("\n调试命令:");
    Serial.println("  在串口监视器输入 't' 运行硬件诊断测试\n");

    timerStartTime = millis();
    lastSensorRead = millis();
}

void loop() {
    // 串口命令处理（用于调试）
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == 't' || cmd == 'T') {
            RunHardwareTest();
        }
    }

    // 按键处理逻辑
    if (digitalRead(K_U_PIN) == LOW) {
        delay(10);
        if (digitalRead(K_U_PIN) == LOW) {
            Disp_Brt_Data += 10;
            if (Disp_Brt_Data > 250) Disp_Brt_Data = 250;
            Serial.printf("亮度增加: %d\n", Disp_Brt_Data);
        }
        while(digitalRead(K_U_PIN) == LOW);
    }

    if (digitalRead(K_D_PIN) == LOW) {
        delay(10);
        if (digitalRead(K_D_PIN) == LOW) {
            if (Disp_Brt_Data >= 10) Disp_Brt_Data -= 10;
            Serial.printf("亮度减少: %d\n", Disp_Brt_Data);
        }
        while(digitalRead(K_D_PIN) == LOW);
    }

    // 定期读取 AHT20 温湿度数据（每 2 秒）
    unsigned long currentMillis = millis();
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;

        if (AHT20_ReadData(&temperature, &humidity)) {
            Serial.printf("温度: %.1f°C, 湿度: %.1f%%\n", temperature, humidity);
        } else {
            Serial.println(" AHT20 读取失败");
        }
    }

    // 清屏并显示信息
    DP_RAM_CLR();

    // 显示标题
    VFD_DISP_ASC816_STR(0, 0, "ESP32-S3 VFD");

    // 显示温湿度
    Display_TempHumi(3, 0);

    // 显示运行时间
    Show_Timer(6, 0);

    // 更新显示缓冲区
    Disp_Buf_Update();
    delay(100);
}
