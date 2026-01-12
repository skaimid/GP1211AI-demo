#include <SPI.h>
#include <TimerOne.h> // 需要在库管理器安装 "TimerOne" 库
#include <avr/pgmspace.h>

// // 包含你的字库头文件
// #include "ASC1224.h"
// #include "ASC816.h"
// #include "ASC57.h"
// #include "amp_graph.h"
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

// 显存定义
unsigned char DP_RAM[8][128];     // 逻辑显存
unsigned char DP_BUF[2064];       // 物理显存 (SPI流)

volatile unsigned char VFD_GRID_SCAN = 0;
volatile unsigned char *DP_BUF_POINT;
volatile unsigned char Disp_Brt_Data = 100; // 亮度


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
                Dp_Ram_Temp =((DP_RAM[i][Dp_Ram_Base_Addr+5]<<1)&0x02);
		 		Dp_Ram_Temp|=((DP_RAM[i][Dp_Ram_Base_Addr+4]<<3)&0x08);
         		Dp_Ram_Temp|=((DP_RAM[i][Dp_Ram_Base_Addr+3]<<5)&0x20);
         		Dp_Ram_Temp|=((DP_RAM[i][Dp_Ram_Base_Addr+5]<<6)&0x80);
	        	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;		
         		Dp_Ram_Temp =   DP_RAM[i][Dp_Ram_Base_Addr+4]     &0x02;
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<2)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])<<3)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<5)&0x80);
				*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>1)&0x02);
         		Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+5]     &0x08;
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<2)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<4)&0x80);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>3)&0x02);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>1)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])<<1)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])<<2)&0x80);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>4)&0x02);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>2)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>1)&0x20);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])<<1)&0x80);    
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
         		Dp_Ram_Temp =(((DP_RAM[i][Dp_Ram_Base_Addr+3])>>5)&0x02);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+5])>>4)&0x08);
         		Dp_Ram_Temp|=(((DP_RAM[i][Dp_Ram_Base_Addr+4])>>2)&0x20);
         		Dp_Ram_Temp|=   DP_RAM[i][Dp_Ram_Base_Addr+3]     &0x80;
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            } else { // 单数列
                 // 请在此处粘贴原文件中 else {} 块的代码
                 // 逻辑完全一致
                Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr+0]   &0x01;
           	 	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<2&0x04);
           	 	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<4&0x10);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<5&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>1&0x01);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<1&0x04);
		    	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<2&0x10);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<4&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>2&0x01);
		    	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>1&0x04);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]<<1&0x10);
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]<<3&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+0]>>4&0x01);
				Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>2&0x04);
            	Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+2]   &0x10;
            	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]<<1&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+1]>>5&0x01);
		    	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>3&0x04);  
	        	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>2&0x10);
            	Dp_Ram_Temp|= DP_RAM[i][Dp_Ram_Base_Addr+1]   &0x40;
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;
            	Dp_Ram_Temp =(DP_RAM[i][Dp_Ram_Base_Addr+2]>>6&0x01);
	        	Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+0]>>5&0x04);
				Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+1]>>3&0x10);   
				Dp_Ram_Temp|=(DP_RAM[i][Dp_Ram_Base_Addr+2]>>1&0x40);
            	*Dp_Buf_Ptr = Dp_Ram_Temp;
	        	++Dp_Buf_Ptr;  			     
            }
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




// --- Setup & Loop ---

void setup() {

// 0. 初始化串口 - 使用波特率 115200
    Serial.begin(115200);

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

// 接收缓冲区
// 一帧图像 = 128列 * 8页 = 1024 字节
#define FRAME_SIZE 1024
byte rxBuffer[FRAME_SIZE];

void loop() {
    // 检查串口是否有足够的数据 (一整帧)
    int available = Serial.available();

    // Serial.write('K');
    
    // 调试输出：显示可用字节数
    if (available > 0 && available % 100 == 0) {
        // 每接收100字节输出一次，避免串口洪水
        Serial.print("缓冲区字节数: ");
        Serial.println(available);
    }
    
    if (available >= FRAME_SIZE) {
        Serial.println("收到完整数据帧！开始读取...");
        
        // 1. 读取 1024 个字节
        size_t bytesRead = Serial.readBytes(rxBuffer, FRAME_SIZE);
        Serial.print("实际读取字节数: ");
        Serial.println(bytesRead);
        
        // 2. 验证数据：打印前几个字节
        Serial.print("前5个字节: ");
        for(int i = 0; i < 5; i++) {
            Serial.print(rxBuffer[i]);
            Serial.print(" ");
        }
        Serial.println();

        // 3. 将接收到的线性数据填入二维数组 DP_RAM
        // 因为 Python 发送时是按页顺序发送的，我们直接内存拷贝即可
        // DP_RAM 在内存中也是连续排列的，所以可以直接拷贝
        memcpy(DP_RAM, rxBuffer, FRAME_SIZE);
        Serial.println("数据已复制到 DP_RAM");

        // 4. 触发 VFD 数据转换 (逻辑层 -> 物理层)
        Disp_Buf_Update();
        Serial.println("数据已转换到物理显存");
        
        // 5. 给 PC 回发一个 'K' 表示接收完毕
        Serial.write('K');
        Serial.println("已发送确认信号 'K'");
        
        // 清除多余的串口缓存（防止数据错位）
        while(Serial.available() > 0) {
            Serial.read(); // 清除所有剩余字节
        }
    }
}
