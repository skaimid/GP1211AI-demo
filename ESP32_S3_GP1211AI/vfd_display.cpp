// ==================== VFD Display Driver Implementation ====================

#include "vfd_display.h"
#include "../Arduino_GP1211AI/ASC57.h"
#include "../Arduino_GP1211AI/ASC816.h"

// 宏定义
#define VFD_CLKG_STROBE() { \
    gpio_set_level(VFD_CLKG_PIN, 0); \
    gpio_set_level(VFD_CLKG_PIN, 1); \
}

#define VFD_LAT_STROBE() { \
    gpio_set_level(VFD_LAT_PIN, 1); \
    gpio_set_level(VFD_LAT_PIN, 0); \
}

// 全局VFD对象
VFDDisplay vfd;

// 定时器中断回调（必须是静态函数）
void IRAM_ATTR vfd_timer_isr() {
    vfd.timerISR();
}

// ==================== 构造函数 ====================
VFDDisplay::VFDDisplay()
    : timer(nullptr)
    , vspi(nullptr)
    , gridScan(0)
    , bufPoint(nullptr)
    , brightness(50)
    , timerMux(portMUX_INITIALIZER_UNLOCKED)
{
    memset(DP_RAM, 0, sizeof(DP_RAM));
    memset(DP_BUF, 0, sizeof(DP_BUF));
}

// ==================== 初始化 ====================
bool VFDDisplay::begin() {
    Serial.println("初始化VFD显示驱动...");

    initGPIO();
    initPWM();
    initSPI();
    powerSequence();
    initTimer();

    Serial.println("✓ VFD显示驱动初始化完成");
    return true;
}

void VFDDisplay::initGPIO() {
    pinMode(VFD_LAT_PIN, OUTPUT);
    pinMode(VFD_CLKG_PIN, OUTPUT);
    pinMode(VFD_SIG_PIN, OUTPUT);
    pinMode(HV_EN_PIN, OUTPUT);
    pinMode(FL_EN_PIN, OUTPUT);

    Serial.println("✓ VFD GPIO初始化完成");
}

void VFDDisplay::initPWM() {
    ledcAttach(VFD_BK_PIN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcWrite(VFD_BK_PIN, brightness);

    Serial.println("✓ VFD PWM初始化完成");
}

void VFDDisplay::initSPI() {
    vspi = new SPIClass(HSPI);
    vspi->begin(VFD_CLKA_PIN, -1, VFD_SIA_PIN, -1);
    vspi->beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));

    Serial.println("✓ VFD SPI初始化完成 (8MHz)");
}

void VFDDisplay::powerSequence() {
    digitalWrite(FL_EN_PIN, HIGH);
    digitalWrite(HV_EN_PIN, LOW);
    clear();

    Serial.println("✓ VFD灯丝预热中...");
    delay(500);

    update();

    digitalWrite(FL_EN_PIN, LOW);
    delay(100);
    digitalWrite(HV_EN_PIN, HIGH);
    delay(100);

    Serial.println("✓ VFD高压开启，准备就绪");
}

void VFDDisplay::initTimer() {
    timer = timerBegin(5319);  // 188μs周期
    timerAttachInterrupt(timer, &vfd_timer_isr);
    timerAlarm(timer, 1, true, 0);

    Serial.println("✓ VFD定时器启动 (188μs)");
}

// ==================== 亮度控制 ====================
void VFDDisplay::setBrightness(uint8_t value) {
    brightness = constrain(value, 0, 255);
}

uint8_t VFDDisplay::getBrightness() {
    return brightness;
}

// ==================== 显存操作 ====================
void VFDDisplay::clear() {
    memset(DP_RAM, 0, sizeof(DP_RAM));
}

void VFDDisplay::setPixel(uint8_t x, uint8_t y, bool on) {
    if (x >= 8 || y >= 128) return;

    uint8_t byte_pos = y / 8;
    uint8_t bit_pos = y % 8;

    if (on) {
        DP_RAM[x][byte_pos] |= (1 << bit_pos);
    } else {
        DP_RAM[x][byte_pos] &= ~(1 << bit_pos);
    }
}

void VFDDisplay::update() {
    uint8_t i = 0;
    uint8_t VFD_GRID;
    uint8_t VFD_GRID_TMP = 0;
    uint8_t Dp_Ram_Temp = 0;
    uint8_t Dp_Ram_Base_Addr = 0;
    uint8_t* Dp_Buf_Ptr = DP_BUF;

    for (VFD_GRID = 0; VFD_GRID < 43; VFD_GRID++) {
        VFD_GRID_TMP = 42 - VFD_GRID;
        Dp_Ram_Base_Addr = (VFD_GRID_TMP >> 1) * 6;

        for (i = 0; i < 8; i++) {
            if ((VFD_GRID_TMP & 0x01)) { // 双数列
                Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 1) & 0x02);
                Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] << 3) & 0x08);
                Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] << 5) & 0x20);
                Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 6) & 0x80);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr + 4] & 0x02;
                Dp_Ram_Temp |= (((DP_RAM[i][Dp_Ram_Base_Addr + 3]) << 2) & 0x08);
                Dp_Ram_Temp |= (((DP_RAM[i][Dp_Ram_Base_Addr + 5]) << 3) & 0x20);
                Dp_Ram_Temp |= (((DP_RAM[i][Dp_Ram_Base_Addr + 4]) << 5) & 0x80);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>1)&0x02);
                Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+5]     &0x08;
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<2)&0x20);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<4)&0x80);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>3)&0x02);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>1)&0x08);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<1)&0x20);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])<<2)&0x80);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>4)&0x02);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>2)&0x08);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>1)&0x20);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<1)&0x80);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>5)&0x02);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>4)&0x08);
                Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>2)&0x20);
                Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+3]     &0x80;
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;
            } else { // 单数列
                Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr+0]   &0x01;
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<2&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<4&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<5&0x40);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>1&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<1&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<2&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<4&0x40);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>2&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>1&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<1&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<3&0x40);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+0]>>4&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>2&0x04);
                Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+2]   &0x10;
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<1&0x40);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>5&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>3&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>2&0x10);
                Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+1]   &0x40;
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;

                Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>6&0x01);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>5&0x04);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>3&0x10);
                Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>1&0x40);
                *Dp_Buf_Ptr++ = Dp_Ram_Temp;
            }
        }
    }
}

// ==================== 文本显示 ====================
void VFDDisplay::drawChar5x7(uint8_t row, uint8_t col, char c) {
    for (uint8_t i = 0; i < 5; i++) {
        DP_RAM[row][col * 8 + i] = pgm_read_byte(&ASC57[((c - 0x20) * 5) + i]);
    }
}

void VFDDisplay::drawString5x7(uint8_t row, uint8_t col, const char* str) {
    while (*str) {
        drawChar5x7(row, col++, *str++);
    }
}

void VFDDisplay::drawChar8x16(uint8_t row, uint8_t col, char c) {
    for (uint8_t i = 0; i < 8; i++) {
        DP_RAM[row * 2][col * 8 + i] = pgm_read_byte(&ASC816[((c - 0x20) << 4) + i]);
        DP_RAM[row * 2 + 1][col * 8 + i] = pgm_read_byte(&ASC816[((c - 0x20) << 4) + 8 + i]);
    }
}

void VFDDisplay::drawString8x16(uint8_t row, uint8_t col, const char* str) {
    while (*str) {
        drawChar8x16(row, col++, *str++);
    }
}

// ==================== 图形显示 ====================
void VFDDisplay::drawIcon16x16(uint8_t row, uint8_t col, const uint8_t* icon) {
    for (uint8_t r = 0; r < 2; r++) {
        for (uint8_t c = 0; c < 16; c++) {
            DP_RAM[row + r][col + c] = pgm_read_byte(&icon[r * 16 + c]);
        }
    }
}

void VFDDisplay::drawIcon8x8(uint8_t row, uint8_t col, const uint8_t* icon) {
    for (uint8_t i = 0; i < 8; i++) {
        DP_RAM[row][col + i] = pgm_read_byte(&icon[i]);
    }
}

void VFDDisplay::drawImage128x64(const uint8_t* image) {
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 128; j++) {
            DP_RAM[i][j] = pgm_read_byte(image++);
        }
    }
}

// ==================== 定时器中断 ====================
void IRAM_ATTR VFDDisplay::timerISR() {
    portENTER_CRITICAL_ISR(&timerMux);

    uint8_t cycle_cnt = 48;

    if (gridScan == 0) {
        bufPoint = DP_BUF;
        gridScan = 43;

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
        vspi->transfer(*bufPoint++);
    }

    VFD_CLKG_STROBE();

    // 亮度控制 / 锁存序列
    ledcWrite(VFD_BK_PIN, 0);           // 消隐
    VFD_LAT_STROBE();                   // 锁存数据
    ledcWrite(VFD_BK_PIN, brightness);  // 恢复亮度

    gridScan--;

    portEXIT_CRITICAL_ISR(&timerMux);
}
