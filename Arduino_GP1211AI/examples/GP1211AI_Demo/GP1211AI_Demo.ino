//////////////////////////////////////////////////////////////////////////////////////
// GP1211AI VFD Display - 示例代码
// 演示各种显示功能
//////////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>
#include <TimerOne.h>

#include "ASC57.h"
#include "ASC816.h"
#include "ASC1224.h"
#include "amp_graph.h"

// 引脚定义（与主程序相同）
#define VFD_BK_PIN    3
#define VFD_LAT_PIN   6
#define VFD_SIG_PIN   7
#define VFD_CLKG_PIN  8
#define VFD_SIA_PIN   11
#define VFD_CLKA_PIN  13
#define HV_EN_PIN     A0
#define FL_EN_PIN     A1

// 宏定义
#define VFD_CLKG_STROBE()  (digitalWrite(VFD_CLKG_PIN, LOW), delayMicroseconds(1), \
                            digitalWrite(VFD_CLKG_PIN, HIGH), delayMicroseconds(1))
#define VFD_LAT_STROBE()   (digitalWrite(VFD_LAT_PIN, HIGH), delayMicroseconds(1), \
                            digitalWrite(VFD_LAT_PIN, LOW), delayMicroseconds(1))

// 全局变量
unsigned char DP_RAM[8][128];
unsigned char DP_BUF[2064];
volatile unsigned char VFD_GRID_SCAN = 0;
volatile unsigned char *DP_BUF_POINT;
unsigned char Disp_Brt_Data = 50;

// 函数声明
void setup_display();
void Disp_Buf_Update(void);
void DP_RAM_CLR(void);
void VFD_DISP_ASC57(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT);
void VFD_DISP_ASC57_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR);
void VFD_DISP_ASC816(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT);
void VFD_DISP_ASC816_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR);
void VFD_DISP_ASC1224(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT);
void VFD_DISP_ASC1224_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR);
void Timer_0_Svr();

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== GP1211AI Demo ==="));

  setup_display();

  Serial.println(F("Display initialized!"));
  delay(1000);
}

void loop() {
  // 示例1: 显示不同大小的文字
  Serial.println(F("Demo 1: Different text sizes"));

  DP_RAM_CLR();
  VFD_DISP_ASC57_STR(0, 0, "5x7 Font");
  Disp_Buf_Update();
  delay(2000);

  DP_RAM_CLR();
  VFD_DISP_ASC816_STR(1, 0, "8x16 Font");
  Disp_Buf_Update();
  delay(2000);

  DP_RAM_CLR();
  VFD_DISP_ASC1224_STR(0, 0, "BIG");
  Disp_Buf_Update();
  delay(2000);

  // 示例2: 显示多行文本
  Serial.println(F("Demo 2: Multiple lines"));

  DP_RAM_CLR();
  VFD_DISP_ASC57_STR(0, 0, "Line 1: Hello");
  VFD_DISP_ASC57_STR(1, 0, "Line 2: World");
  VFD_DISP_ASC57_STR(2, 0, "Line 3: Arduino");
  VFD_DISP_ASC57_STR(3, 0, "Line 4: VFD!");
  Disp_Buf_Update();
  delay(3000);

  // 示例3: 混合字体大小
  Serial.println(F("Demo 3: Mixed fonts"));

  DP_RAM_CLR();
  VFD_DISP_ASC57_STR(0, 0, "Small");
  VFD_DISP_ASC816_STR(2, 0, "Medium");
  Disp_Buf_Update();
  delay(3000);

  // 示例4: 显示数字
  Serial.println(F("Demo 4: Numbers"));

  DP_RAM_CLR();
  VFD_DISP_ASC1224_STR(0, 0, "123");
  VFD_DISP_ASC57_STR(4, 0, "Count: 12345");
  Disp_Buf_Update();
  delay(3000);

  // 示例5: 亮度调节演示
  Serial.println(F("Demo 5: Brightness fade"));

  for (int i = 1; i <= 200; i += 10) {
    DP_RAM_CLR();
    VFD_DISP_ASC816_STR(1, 0, "Brightness:");
    VFD_DISP_ASC816_STR(3, 0, String(i).c_str());
    Disp_Buf_Update();

    analogWrite(VFD_BK_PIN, map(i, 0, 200, 0, 255));
    Disp_Brt_Data = i;
    delay(200);
  }

  delay(1000);

  for (int i = 200; i >= 1; i -= 10) {
    DP_RAM_CLR();
    VFD_DISP_ASC816_STR(1, 0, "Brightness:");
    VFD_DISP_ASC816_STR(3, 0, String(i).c_str());
    Disp_Buf_Update();

    analogWrite(VFD_BK_PIN, map(i, 0, 200, 0, 255));
    Disp_Brt_Data = i;
    delay(200);
  }

  Disp_Brt_Data = 100;
  analogWrite(VFD_BK_PIN, map(Disp_Brt_Data, 0, 200, 0, 255));

  // 示例6: 滚动文本效果
  Serial.println(F("Demo 6: Scrolling text"));

  const char *scrollText = "Arduino VFD Display Demo...";
  int scrollPos = 0;

  for (int i = 0; i < strlen(scrollText) * 6 + 20; i++) {
    DP_RAM_CLR();

    // 计算要显示的子串
    int startPos = scrollPos;
    int charIndex = 0;

    for (int col = 0; col < 20; col++) {
      int charPos = startPos + col;
      if (charPos >= 0 && charPos < strlen(scrollText)) {
        VFD_DISP_ASC57(2, col, scrollText[charPos]);
      }
    }

    Disp_Buf_Update();
    delay(150);
    scrollPos--;
  }

  delay(2000);

  // 示例7: 显示logo（如果可用）
  Serial.println(F("Demo 7: Logo"));

  DP_RAM_CLR();
  VFD_DISP_PIC_12864(logo_12864);
  Disp_Buf_Update();
  delay(3000);

  // 示例8: 简单动画
  Serial.println(F("Demo 8: Simple animation"));

  for (int frame = 0; frame < 20; frame++) {
    DP_RAM_CLR();

    // 绘制一个移动的条
    int barPos = frame % 128;
    for (int i = 0; i < 8; i++) {
      DP_RAM[i][barPos] = 0xFF;
    }

    Disp_Buf_Update();
    delay(100);
  }

  delay(1000);

  Serial.println(F("Demo cycle complete! Restarting..."));
  delay(2000);
}

// === 显示缓冲区更新 ===
void Disp_Buf_Update() {
  unsigned char i;
  unsigned char VFD_GRID;
  unsigned char VFD_GRID_TMP;
  unsigned char Dp_Ram_Temp;
  unsigned char Dp_Ram_Base_Addr;
  unsigned char *Dp_Buf_Ptr;

  Dp_Buf_Ptr = DP_BUF;

  for (VFD_GRID = 0; VFD_GRID < 43; VFD_GRID++) {
    VFD_GRID_TMP = 42 - VFD_GRID;
    Dp_Ram_Base_Addr = (VFD_GRID_TMP >> 1) * 6;

    for (i = 0; i < 8; i++) {
      if (VFD_GRID_TMP & 0x01) {
        // 双点列（代码与主程序相同，为简洁省略）
        Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 1) & 0x02);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] << 3) & 0x08);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] << 5) & 0x20);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 6) & 0x80);
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr + 4] & 0x02;
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] << 2) & 0x08);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 3) & 0x20);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] << 5) & 0x80);
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 3] >> 1) & 0x02);
        Dp_Ram_Temp |= DP_RAM[i][Dp_Ram_Base_Addr + 5] & 0x08;
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] << 2) & 0x20);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] << 4) & 0x80);
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 5] >> 3) & 0x02);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] >> 1) & 0x08);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] << 1) & 0x20);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] << 2) & 0x80);
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 4] >> 4) & 0x02);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 3] >> 2) & 0x08);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] >> 1) & 0x20);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] << 1) & 0x80);
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = ((DP_RAM[i][Dp_Ram_Base_Addr + 3] >> 5) & 0x02);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 5] >> 4) & 0x08);
        Dp_Ram_Temp |= ((DP_RAM[i][Dp_Ram_Base_Addr + 4] >> 2) & 0x20);
        Dp_Ram_Temp |= DP_RAM[i][Dp_Ram_Base_Addr + 3] & 0x80;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;
      } else {
        // 单点列
        Dp_Ram_Temp = DP_RAM[i][Dp_Ram_Base_Addr + 0] & 0x01;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 1] << 2) & 0x04;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 2] << 4) & 0x10;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 0] << 5) & 0x40;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = (DP_RAM[i][Dp_Ram_Base_Addr + 1] >> 1) & 0x01;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 2] << 1) & 0x04;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 0] << 2) & 0x10;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 1] << 4) & 0x40;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = (DP_RAM[i][Dp_Ram_Base_Addr + 2] >> 2) & 0x01;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 0] >> 1) & 0x04;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 1] << 1) & 0x10;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 2] << 3) & 0x40;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = (DP_RAM[i][Dp_Ram_Base_Addr + 0] >> 4) & 0x01;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 1] >> 2) & 0x04;
        Dp_Ram_Temp |= DP_RAM[i][Dp_Ram_Base_Addr + 2] & 0x10;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 0] << 1) & 0x40;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = (DP_RAM[i][Dp_Ram_Base_Addr + 1] >> 5) & 0x01;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 2] >> 3) & 0x04;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 0] >> 2) & 0x10;
        Dp_Ram_Temp |= DP_RAM[i][Dp_Ram_Base_Addr + 1] & 0x40;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;

        Dp_Ram_Temp = (DP_RAM[i][Dp_Ram_Base_Addr + 2] >> 6) & 0x01;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 0] >> 5) & 0x04;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 1] >> 3) & 0x10;
        Dp_Ram_Temp |= (DP_RAM[i][Dp_Ram_Base_Addr + 2] >> 1) & 0x40;
        *Dp_Buf_Ptr = Dp_Ram_Temp;
        ++Dp_Buf_Ptr;
      }
    }
  }
}

// === 清屏 ===
void DP_RAM_CLR(void) {
  unsigned char i, j;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 128; j++) {
      DP_RAM[i][j] = 0x00;
    }
  }
}

// === 字符显示函数 ===
void VFD_DISP_ASC57(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT) {
  unsigned char i;
  for (i = 0; i < 5; i++) {
    DP_RAM[VFD_X][VFD_Y * 8 + i] = ASC57[((DAT - 0x20) * 5) + i];
  }
}

void VFD_DISP_ASC57_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR) {
  while (*STR) {
    VFD_DISP_ASC57(VFD_X, VFD_Y++, *STR++);
  }
}

void VFD_DISP_ASC816(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT) {
  unsigned char i;
  for (i = 0; i < 8; i++) {
    DP_RAM[VFD_X * 2][VFD_Y * 8 + i] = ASC816[((DAT - 0x20) << 4) + i];
    DP_RAM[VFD_X * 2 + 1][VFD_Y * 8 + i] = ASC816[((DAT - 0x20) << 4) + 8 + i];
  }
}

void VFD_DISP_ASC816_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR) {
  while (*STR) {
    VFD_DISP_ASC816(VFD_X, VFD_Y++, *STR++);
  }
}

void VFD_DISP_ASC1224(unsigned char VFD_X, unsigned char VFD_Y, unsigned char DAT) {
  unsigned char i;
  for (i = 0; i < 12; i++) {
    DP_RAM[VFD_X][VFD_Y + i] = ASC1224[((DAT - 0x20) * 36) + i];
    DP_RAM[VFD_X + 1][VFD_Y + i] = ASC1224[((DAT - 0x20) * 36) + 12 + i];
    DP_RAM[VFD_X + 2][VFD_Y + i] = ASC1224[((DAT - 0x20) * 36) + 24 + i];
  }
}

void VFD_DISP_ASC1224_STR(unsigned char VFD_X, unsigned char VFD_Y, const char *STR) {
  while (*STR) {
    VFD_DISP_ASC1224(VFD_X, VFD_Y, *STR++);
    VFD_Y = VFD_Y + 12;
  }
}

void VFD_DISP_PIC_12864(const unsigned char *dat) {
  unsigned char i, j;
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 128; j++) {
      DP_RAM[i][j] = *dat++;
    }
  }
}

// === 初始化函数 ===
void setup_display() {
  pinMode(VFD_BK_PIN, OUTPUT);
  pinMode(VFD_LAT_PIN, OUTPUT);
  pinMode(VFD_SIG_PIN, OUTPUT);
  pinMode(VFD_CLKG_PIN, OUTPUT);
  pinMode(HV_EN_PIN, OUTPUT);
  pinMode(FL_EN_PIN, OUTPUT);

  digitalWrite(VFD_CLKG_PIN, HIGH);
  digitalWrite(VFD_LAT_PIN, LOW);
  digitalWrite(VFD_SIG_PIN, LOW);
  digitalWrite(VFD_BK_PIN, LOW);

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);

  Timer1.initialize(188);
  Timer1.attachInterrupt(Timer_0_Svr);

  digitalWrite(FL_EN_PIN, HIGH);
  digitalWrite(HV_EN_PIN, LOW);

  delay(100);

  DP_RAM_CLR();

  analogWrite(VFD_BK_PIN, map(Disp_Brt_Data, 0, 200, 0, 255));

  digitalWrite(FL_EN_PIN, LOW);
  delay(100);
  digitalWrite(HV_EN_PIN, HIGH);
}

// === 定时器中断 ===
void Timer_0_Svr() {
  unsigned char cycle_cnt = 48;

  if (VFD_GRID_SCAN == 0) {
    DP_BUF_POINT = DP_BUF;
    VFD_GRID_SCAN = 43;

    digitalWrite(VFD_SIG_PIN, HIGH);
    VFD_CLKG_STROBE();
    VFD_CLKG_STROBE();
    digitalWrite(VFD_SIG_PIN, LOW);
    VFD_CLKG_STROBE();
    VFD_CLKG_STROBE();
    VFD_CLKG_STROBE();
  }

  while ((cycle_cnt--) > 0) {
    SPI.transfer(*DP_BUF_POINT++);
  }

  VFD_CLKG_STROBE();
  analogWrite(VFD_BK_PIN, 0);
  VFD_LAT_STROBE();
  analogWrite(VFD_BK_PIN, map(Disp_Brt_Data, 0, 200, 0, 255));
  VFD_GRID_SCAN--;
}
