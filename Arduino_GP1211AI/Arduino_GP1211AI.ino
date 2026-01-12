#include <SPI.h>
#include <TimerOne.h> // 需要在库管理器安装 "TimerOne" 库
// #include <avr/pgmspace.h>

// 包含你的字库头文件
#include "ASC1224.h"
#include "ASC816.h"
#include "ASC57.h"
#include "my_img.h"
// #include "CHINESE.h" // 如果需要中文，取消注释并确保文件格式正确

// --- 引脚定义 ---
#define VFD_BK_PIN   11  // PWM 亮度控制
#define VFD_LAT_PIN  49
#define VFD_CLKG_PIN 48
#define VFD_SIG_PIN  47
#define VFD_SIA_PIN  51  // MOSI
#define VFD_CLKA_PIN 52  // SCK

#define HV_EN_PIN    46
#define FL_EN_PIN    45

#define K_U_PIN      2
#define K_D_PIN      3
#define K_M_PIN      4

// --- 宏定义模拟 ---
// 为了速度，这里可以用直接端口操作优化，但为了可读性先用 digitalWrite
#define VFD_CLKG_STROBE() { digitalWrite(VFD_CLKG_PIN, LOW); digitalWrite(VFD_CLKG_PIN, HIGH); }
#define VFD_LAT_STROBE()  { digitalWrite(VFD_LAT_PIN, HIGH); digitalWrite(VFD_LAT_PIN, LOW); }

// --- 全局变量 ---
unsigned char DP_RAM[8][128];     // 显示缓冲 (RAM)
unsigned char DP_BUF[2064];       // SPI 点阵显示缓冲 (RAM)

volatile unsigned char VFD_GRID_SCAN = 0;
volatile unsigned char *DP_BUF_POINT;
volatile unsigned char Disp_Brt_Data = 50; // 亮度 (0-255 for Arduino PWM)

// 时间
unsigned long timerStartTime = 0;  // 记录开始计时的时间点
char timeBuffer[10];               // 用于存放格式化后的字符串 "00:00:00"


// --- 延时函数封装 ---
// Arduino 自带 delay() 和 delayMicroseconds()，无需重写

// --- VFD 逻辑函数 ---

// 将显存 DP_RAM 转换为 VFD 硬件所需的位流 DP_BUF
// 逻辑保持原版 C 代码不变，仅适配数据类型
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
                
                // ... (中间省略的代码逻辑完全照搬原文件，只需注意数组越界检查) ...
                // 为节省篇幅，这里假设你已经把原 Disp_Buf_Update 函数完整复制过来
                // 请务必将原文件中 Disp_Buf_Update 的内容完整粘贴在此处
                // 唯一的区别是无需 xdata 关键字
                
                // 下面是原代码片段的逻辑补全示例 (建议复制原文件内容):
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
                 // 请在此处粘贴原文件中 else {} 块的代码
                 // 逻辑完全一致
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

// --- 绘图函数 (适配 pgm_read_byte) ---

void VFD_DISP_ASC57(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT) {
    for (unsigned char i = 0; i < 5; i++) {
        // 使用 pgm_read_byte 读取 Flash 中的字库
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
            DP_RAM[i][j] = pgm_read_byte(dat++); // 从 Flash 读取图片数据
        }
    }
}

void DP_RAM_CLR(void) {
    memset(DP_RAM, 0, sizeof(DP_RAM));
}

// --- 核心定时器中断 (刷新屏幕) ---
// 原理：STC 每次中断发送一个 Grid 的数据 (48字节)
// Arduino Mega 的 TimerOne 将模拟此行为
void timerIsr() {
    unsigned char cycle_cnt = 48;

    if (VFD_GRID_SCAN == 0) {
        DP_BUF_POINT = DP_BUF;
        VFD_GRID_SCAN = 43;

        // 帧同步序列
        digitalWrite(VFD_SIG_PIN, HIGH);
        VFD_CLKG_STROBE();
        VFD_CLKG_STROBE();
        digitalWrite(VFD_SIG_PIN, LOW);
        VFD_CLKG_STROBE();
        VFD_CLKG_STROBE();
        VFD_CLKG_STROBE();
    }

    // 通过硬件 SPI 发送 48 字节数据
    // Arduino Mega SPI 默认 4MHz，速度足够
    while ((cycle_cnt--) > 0) {
        SPI.transfer(*DP_BUF_POINT++);
    }

    VFD_CLKG_STROBE();
    
    // 亮度控制 / 锁存序列
    // 原代码逻辑：PWMB_CCR6=0 (关灯/消隐) -> Latch -> PWMB_CCR6=Val (开灯)
    // 我们用 analogWrite 来控制
    analogWrite(VFD_BK_PIN, 0); // 消隐
    VFD_LAT_STROBE();           // 锁存数据
    analogWrite(VFD_BK_PIN, Disp_Brt_Data); // 恢复亮度

    VFD_GRID_SCAN--;
}


void Show_Timer(unsigned char row, unsigned char col) {
    // 1. 获取当前运行时间
    unsigned long currentMillis = millis();
    
    // 2. 计算流逝的总秒数
    // (currentMillis - timerStartTime) 得到毫秒差，除以1000得到秒
    unsigned long totalSeconds = (currentMillis - timerStartTime) / 1000;

    // 3. 计算 时:分:秒
    unsigned long seconds = totalSeconds % 60;
    unsigned long minutes = (totalSeconds / 60) % 60;
    unsigned long hours   = (totalSeconds / 3600); 

    // 4. 格式化字符串
    // %02lu 表示：输出无符号长整型，至少2位，不足补0
    // 例如：1点5分9秒 -> "01:05:09"
    sprintf(timeBuffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);

    // 5. 显示到 VFD
    // 使用之前的 5x7 字符显示函数
    VFD_DISP_ASC57_STR(row, col, timeBuffer);
}


// --- Setup & Loop ---

void setup() {
    // 1. 初始化引脚
    pinMode(VFD_BK_PIN, OUTPUT);
    pinMode(VFD_LAT_PIN, OUTPUT);
    pinMode(VFD_CLKG_PIN, OUTPUT);
    pinMode(VFD_SIG_PIN, OUTPUT);
    pinMode(HV_EN_PIN, OUTPUT);
    pinMode(FL_EN_PIN, OUTPUT);
    
    // 按钮
    pinMode(K_U_PIN, INPUT_PULLUP);
    pinMode(K_D_PIN, INPUT_PULLUP);
    pinMode(K_M_PIN, INPUT_PULLUP);

    // 2. 初始化 SPI
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0)); // VFD通常是LSB First，根据需要调整

    // 3. 上电时序
    digitalWrite(FL_EN_PIN, HIGH); // 灯丝开启
    digitalWrite(HV_EN_PIN, LOW);  // 高压关闭
    DP_RAM_CLR();
    
    delay(100); // 预热

    // 显示 Logo
    // VFD_DISP_PIC_12864(logo_12864);
    Disp_Buf_Update();

    // 4. 初始化定时器 (刷新屏幕)
    // 原 STC 频率 44MHz，定时器设为 188us。
    // Arduino Mega 16MHz，为了保证刷新率和不阻塞主循环，设置 200us 左右
    Timer1.initialize(200); 
    Timer1.attachInterrupt(timerIsr);

    digitalWrite(FL_EN_PIN, LOW); // 注意：原代码逻辑 FL_EN=0 后 Delay 100ms 再 HV_EN=1
    // 这里可能是控制逻辑反转，或者是特定的电源电路逻辑，保持原代码顺序：
    delay(100);
    digitalWrite(HV_EN_PIN, HIGH); // 高压开启
}

void loop() {
    // 按键处理逻辑 (保持原逻辑)
    if (digitalRead(K_U_PIN) == LOW) {
        delay(10);
        if (digitalRead(K_U_PIN) == LOW) {
            Disp_Brt_Data += 10;
            if (Disp_Brt_Data > 250) Disp_Brt_Data = 250; // Arduino PWM max 255
        }
        while(digitalRead(K_U_PIN) == LOW);
    }

    if (digitalRead(K_D_PIN) == LOW) {
        delay(10);
        if (digitalRead(K_D_PIN) == LOW) {
            if (Disp_Brt_Data >= 10) Disp_Brt_Data -= 10;
        }
        while(digitalRead(K_D_PIN) == LOW);
    }

    // // 演示显示数字
    // static int demoValue = 0;
    
    // 显示亮度值
    // 注意：原代码将 Disp_Brt_Data++ 用作演示效果
    // 这里我们简单显示 demoValue
    // VFD_DISP_ASC57(0, 0, (demoValue / 1000) + '0');
    // VFD_DISP_ASC57(0, 1, (demoValue % 1000 / 100) + '0');
    // VFD_DISP_ASC57(0, 2, (demoValue % 1000 % 100 / 10) + '0');
    // VFD_DISP_ASC57(0, 3, (demoValue % 1000 % 100 % 10) + '0');
    // VFD_DISP_ASC57_STR(0, 0, "hello world!!!!");

    // // 在第 2 行 (row 2), 第 0 列 (col 0) 显示计时
    // Show_Timer(2, 0); 

VFD_DISP_PIC_12864(my_image);

    // --- 屏幕刷新 ---
    // 必须调用这个函数，显存里的内容才会真正发送到屏幕
    
    Disp_Buf_Update(); // 将显存写入缓冲区
    delay(100);
    
    // demoValue++;
    // if(demoValue > 9999) demoValue = 0;
}