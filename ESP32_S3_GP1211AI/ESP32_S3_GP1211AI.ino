#include <SPI.h>

// ==================== ESP32-S3 平台说明 ====================
// 本代码适用于 ESP32-S3 芯片，从 Arduino 版本迁移而来
// 主要改动：
// 1. 使用 ESP32 硬件定时器替代 TimerOne
// 2. 使用 LEDC PWM 控制器替代 analogWrite
// 3. 更新引脚定义为 ESP32-S3 GPIO
// 4. SPI 配置优化
// ===========================================================

// 包含字库头文件
#include "../Arduino_GP1211AI/ASC1224.h"
#include "../Arduino_GP1211AI/ASC816.h"
#include "../Arduino_GP1211AI/ASC57.h"
#include "../Arduino_GP1211AI/my_img.h"
// #include "../Arduino_GP1211AI/CHINESE.h" // 如果需要中文，取消注释

// ==================== ESP32-S3 引脚定义 ====================
// ESP32-S3 GPIO 引脚配置（可根据实际硬件修改）
#define VFD_BK_PIN   GPIO_NUM_12  // PWM 亮度控制
#define VFD_LAT_PIN  GPIO_NUM_13  // 数据锁存
#define VFD_CLKG_PIN GPIO_NUM_14  // 栅极时钟
#define VFD_SIG_PIN  GPIO_NUM_15  // 信号控制

// ESP32-S3 HSPI 引脚（使用默认 HSPI）
#define VFD_SIA_PIN  GPIO_NUM_11  // MOSI
#define VFD_CLKA_PIN GPIO_NUM_12  // SCK
#define VFD_SS_PIN   GPIO_NUM_10  // SS (可选，未使用)

// 电源管理引脚
#define HV_EN_PIN    GPIO_NUM_16  // 高压使能
#define FL_EN_PIN    GPIO_NUM_17  // 灯丝使能

// 按键引脚
#define K_U_PIN      GPIO_NUM_18  // 增加亮度
#define K_D_PIN      GPIO_NUM_19  // 减少亮度
#define K_M_PIN      GPIO_NUM_20  // 菜单

// ==================== LEDC PWM 配置 ====================
#define LEDC_CHANNEL        0           // LEDC 通道
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_FREQUENCY      5000        // 5kHz PWM 频率
#define LEDC_RESOLUTION     LEDC_TIMER_8_BIT  // 8位分辨率 (0-255)

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
    ledcWrite(LEDC_CHANNEL, 0);           // 消隐
    VFD_LAT_STROBE();                     // 锁存数据
    ledcWrite(LEDC_CHANNEL, Disp_Brt_Data); // 恢复亮度

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
    ledcSetup(LEDC_CHANNEL, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(VFD_BK_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0); // 初始亮度为 0

    Serial.println("✓ GPIO 初始化完成");
    Serial.println("✓ LEDC PWM 配置完成");

    // 3. 初始化 SPI（使用 HSPI）
    vspi = new SPIClass(HSPI);
    vspi->begin(VFD_CLKA_PIN, -1, VFD_SIA_PIN, -1); // SCK, MISO(-1), MOSI, SS(-1)
    vspi->beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));

    Serial.println("✓ SPI 初始化完成 (8MHz, LSB First)");

    // 4. 上电时序
    digitalWrite(FL_EN_PIN, HIGH); // 灯丝开启
    digitalWrite(HV_EN_PIN, LOW);  // 高压关闭
    DP_RAM_CLR();

    Serial.println("✓ 灯丝预热中...");
    delay(100);

    // 5. 初始化硬件定时器
    // ESP32-S3 有 4 个定时器组，每组 2 个定时器
    // 使用定时器 0，分频器 80 (1MHz 时钟)，188us = 188 计数
    timer = timerBegin(0, 80, true); // 定时器 0, 分频 80, 向上计数
    timerAttachInterrupt(timer, &onTimer, true); // 绑定中断函数
    timerAlarmWrite(timer, 188, true); // 188us 周期，自动重载
    timerAlarmEnable(timer); // 启动定时器

    Serial.println("✓ 硬件定时器启动 (188us 周期)");

    // 显示初始图像
    Disp_Buf_Update();

    digitalWrite(FL_EN_PIN, LOW);
    delay(100);
    digitalWrite(HV_EN_PIN, HIGH); // 高压开启

    Serial.println("✓ 高压开启，VFD 准备就绪\n");
    Serial.println("按键功能:");
    Serial.println("  K_U (GPIO18): 增加亮度");
    Serial.println("  K_D (GPIO19): 减少亮度");
    Serial.println("  K_M (GPIO20): 菜单\n");

    timerStartTime = millis();
}

void loop() {
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

    // 显示自定义图像
    VFD_DISP_PIC_12864(my_image);

    // 更新显示缓冲区
    Disp_Buf_Update();
    delay(100);
}
