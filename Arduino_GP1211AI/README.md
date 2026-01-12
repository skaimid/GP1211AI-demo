# GP1211AI VFD Display - Arduino Port

## 项目简介

这是从STC8H8K单片机移植到Arduino平台的FUTABA GP1211AI VFD（真空荧光显示屏）驱动程序。

- **原版硬件**: STC8H8K Ver3.1
- **原版软件**: Ver3.0 BY DONGFENG
- **移植日期**: 2025-12-25
- **VFD规格**: FUTABA GP1211AI 128×64像素

## 功能特性

- ✅ 支持多种字体大小：5×7、8×16、12×24点阵ASCII字符
- ✅ 支持图片显示：16×16、32×32、128×64点阵
- ✅ SPI高速数据传输
- ✅ PWM亮度控制（0-200级）
- ✅ 按键调节亮度
- ✅ 定时器自动刷新（188μs周期）

## 硬件连接

### Arduino引脚定义

| 功能 | 引脚 | 说明 |
|------|------|------|
| VFD_BK | D3 | PWM亮度控制 |
| VFD_LAT | D6 | 锁存信号 |
| VFD_SIG | D7 | 信号控制 |
| VFD_CLKG | D8 | 栅极时钟 |
| VFD_SIA | D11 | SPI MOSI |
| VFD_CLKA | D13 | SPI SCK |
| HV_EN | A0 | 高压使能 |
| FL_EN | A1 | 灯丝使能 |
| K_U | D2 | 增加亮度按键 |
| K_D | D4 | 减少亮度按键 |
| K_M | D5 | 菜单按键（保留） |

### 接线说明

```
Arduino          GP1211AI VFD Module
-------         --------------------
D3 (PWM)    -->  BK (Brightness)
D6           -->  LAT (Latch)
D7           -->  SIG (Signal)
D8           -->  CLKG (Grid Clock)
D11          -->  SIA (Data)
D13          -->  CLKA (Data Clock)
A0           -->  HV_EN (High Voltage Enable)
A1           -->  FL_EN (Filament Enable)
GND          -->  GND
5V/3.3V      -->  VCC (根据模块要求)
```

## 依赖库

需要安装以下Arduino库：

1. **TimerOne** - 定时器中断库
   - 在Arduino IDE中：工具 -> 管理库 -> 搜索"TimerOne" -> 安装

2. **SPI** - Arduino内置库，无需安装

## 编译和上传

1. 使用Arduino IDE打开 `GP1211AI.ino`
2. 选择开发板（如Arduino Uno/Nano/Mega）
3. 选择正确的串口
4. 点击上传

## API函数说明

### 字符显示函数

```cpp
// 显示单个5x7 ASCII字符
VFD_DISP_ASC57(page, column, character);

// 显示5x7 ASCII字符串
VFD_DISP_ASC57_STR(page, column, "Hello");

// 显示单个8x16 ASCII字符
VFD_DISP_ASC816(page, column, character);

// 显示8x16 ASCII字符串
VFD_DISP_ASC816_STR(page, column, "World");

// 显示单个12x24 ASCII字符
VFD_DISP_ASC1224(page, column, character);

// 显示12x24 ASCII字符串
VFD_DISP_ASC1224_STR(page, column, "Big Text");
```

### 图片显示函数

```cpp
// 显示16x16图片
VFD_DISP_PIC_1616(page, column, image_data);

// 显示32x32图片
VFD_DISP_PIC_3232(page, column, image_data);

// 显示全屏128x64图片
VFD_DISP_PIC_12864(image_data);
```

### 其他函数

```cpp
// 清屏
DP_RAM_CLR();

// 更新显示缓冲区到屏幕
Disp_Buf_Update();
```

## 使用示例

```cpp
void setup() {
  // 初始化在主文件中已完成
}

void loop() {
  // 显示5x7文本
  VFD_DISP_ASC57_STR(0, 0, "Hello World!");
  Disp_Buf_Update();
  delay(2000);

  // 清屏
  DP_RAM_CLR();

  // 显示8x16文本
  VFD_DISP_ASC816_STR(1, 0, "Arduino VFD");
  Disp_Buf_Update();
  delay(2000);
}
```

## 按键功能

- **K_U (按键+)**: 增加亮度
- **K_D (按键-)**: 减少亮度
- 亮度范围：1-200（默认值：1）

## 性能参数

- **刷新频率**: 约265Hz（188μs/周期）
- **SPI速度**: 4MHz（Arduino 16MHz系统时钟）
- **亮度调节**: PWM 0-200级
- **显示缓冲**: 2064字节（43栅极 × 48字节）

## 注意事项

1. **电压要求**: 确认VFD模块的输入电压要求（通常需要5V或12V）
2. **电流要求**: VFD显示需要较大电流，确保电源充足
3. **引脚兼容**: 根据实际Arduino型号调整引脚定义
4. **Timer1冲突**: 如果使用Timer1的其他库，可能会产生冲突
5. **SPI使用**: 使用硬件SPI，确保不与其他SPI设备冲突

## 移植改动说明

相比原版STC8H代码的改动：

1. **寄存器操作** → Arduino digitalWrite/analogWrite
2. **特殊功能寄存器** → 标准库函数（SPI、Timer）
3. **中断服务程序** → Timer1库attachInterrupt
4. **位操作** → 保持不变（数据操作层）
5. **引脚定义** → 可配置的宏定义
6. **PWM控制** → analogWrite映射

## 兼容性

- ✅ Arduino Uno
- ✅ Arduino Nano
- ✅ Arduino Mega
- ✅ Arduino Leonardo（可能需要调整引脚）
- ⚠️ Arduino Due（3.3V逻辑，需注意电平转换）

## 许可说明

本移植基于原版BY DONGFENG的代码，仅供学习参考使用。

## 联系方式

- 原作者: dongfeng.dream@126.com
- 移植: 2025-12-25

---

**祝使用愉快！**
