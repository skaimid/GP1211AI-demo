// ==================== VFD Display Driver Header ====================
// VFD显示驱动模块 - GP1211AI 128x64 VFD显示屏
// 负责底层显示驱动、字符/图形绘制、定时器中断等

#ifndef VFD_DISPLAY_H
#define VFD_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>

// ==================== 引脚定义 ====================
#define VFD_SIG_PIN  GPIO_NUM_37  // 信号控制
#define VFD_CLKG_PIN GPIO_NUM_38  // 栅极时钟
#define VFD_LAT_PIN  GPIO_NUM_39  // 数据锁存
#define VFD_BK_PIN   GPIO_NUM_40  // PWM 亮度控制
#define VFD_CLKA_PIN GPIO_NUM_35  // SCK
#define VFD_SIA_PIN  GPIO_NUM_36  // MOSI
#define HV_EN_PIN    GPIO_NUM_41  // 高压使能
#define FL_EN_PIN    GPIO_NUM_42  // 灯丝使能

// ==================== LEDC PWM 配置 ====================
#define LEDC_FREQUENCY      5000  // 5kHz PWM 频率
#define LEDC_RESOLUTION     8     // 8位分辨率 (0-255)

// ==================== VFD Display 类 ====================
class VFDDisplay {
public:
    VFDDisplay();

    // 初始化和控制
    bool begin();                          // 初始化VFD
    void setBrightness(uint8_t brightness); // 设置亮度 (0-255)
    uint8_t getBrightness();               // 获取当前亮度

    // 显存操作
    void clear();                          // 清屏
    void update();                         // 更新显示（将DP_RAM转换为DP_BUF）
    void setPixel(uint8_t x, uint8_t y, bool on); // 设置单个像素

    // 文本显示
    void drawChar5x7(uint8_t row, uint8_t col, char c);
    void drawString5x7(uint8_t row, uint8_t col, const char* str);
    void drawChar8x16(uint8_t row, uint8_t col, char c);
    void drawString8x16(uint8_t row, uint8_t col, const char* str);

    // 图形显示
    void drawIcon16x16(uint8_t row, uint8_t col, const uint8_t* icon);
    void drawIcon8x8(uint8_t row, uint8_t col, const uint8_t* icon);
    void drawImage128x64(const uint8_t* image);

    // 定时器中断处理（需要在中断中调用）
    void IRAM_ATTR timerISR();

private:
    void initGPIO();
    void initPWM();
    void initSPI();
    void initTimer();
    void powerSequence();

    // 显存缓冲区
    uint8_t DP_RAM[8][128];      // 显示内存
    uint8_t DP_BUF[2064];        // SPI缓冲

    // 定时器相关
    hw_timer_t* timer;
    portMUX_TYPE timerMux;

    // SPI
    SPIClass* vspi;

    // 显示控制
    volatile uint8_t gridScan;
    volatile uint8_t* bufPoint;
    volatile uint8_t brightness;
};

// 全局VFD对象（在cpp中定义）
extern VFDDisplay vfd;

#endif // VFD_DISPLAY_H
