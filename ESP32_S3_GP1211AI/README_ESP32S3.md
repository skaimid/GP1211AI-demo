# GP1211AI VFD 显示驱动 - ESP32-S3 版本

这是 FUTABA GP1211AI VFD 显示屏驱动的 **ESP32-S3 移植版本**，从 Arduino 版本迁移而来。

## 项目信息

- **原始平台**: STC8H8K 单片机 (Ver3.1)
- **第一次移植**: Arduino Uno/Mega (2025-12-25)
- **ESP32-S3 移植**: 2026-01-12
- **显示屏**: FUTABA GP1211AI VFD (128×64 像素)
- **刷新率**: ~265Hz (188μs 周期)

## ESP32-S3 版本特性

### ✨ 新特性

1. **高性能**: 240MHz 双核处理器，远超 Arduino 16MHz
2. **大内存**: 512KB SRAM，无内存限制
3. **精确定时**: 硬件定时器，1μs 精度
4. **高级 PWM**: 16 位 LEDC，亮度控制更平滑
5. **灵活 GPIO**: 45 个 GPIO，引脚可自由配置
6. **扩展性强**: 支持 WiFi/蓝牙，可实现远程控制
7. **调试友好**: USB CDC 串口，无需额外转换器
8. **🌡️ 温湿度监测**: 集成 AHT20 传感器，实时显示环境数据

### 🔧 主要改进

| 功能 | Arduino 版本 | ESP32-S3 版本 |
|------|-------------|--------------|
| 定时器 | TimerOne 库 | 硬件定时器 API |
| PWM 控制 | analogWrite() | LEDC (16位) |
| SPI 速度 | 4MHz | 8MHz (可达 40MHz) |
| 串口调试 | 需要 USB-TTL | USB CDC 内置 |
| 定时器精度 | ±4μs | ±1μs |
| 内存限制 | 2KB RAM | 512KB SRAM |

### ⚠️ 重要注意事项

**电压兼容性**:
- ESP32-S3 是 **3.3V 逻辑**
- 如果 VFD 模块是 5V 逻辑，**必须使用电平转换器**
- 推荐使用 TXS0108E 或 74HC245 芯片
- 详见 [WIRING_ESP32S3.md](WIRING_ESP32S3.md)

## 快速开始

### 1. 硬件准备

- ESP32-S3 开发板（DevKitC-1 或兼容板）
- FUTABA GP1211AI VFD 显示模块
- AHT20 温湿度传感器模块 **[新增]**
- 电平转换器（如 TXS0108E）**[如果 VFD 是 5V]**
- 杜邦线若干
- 5V 电源（如需要）

### 2. 软件准备

#### Arduino IDE
```
1. 安装 ESP32 支持包
   - 文件 → 首选项 → 附加开发板管理器网址
   - 添加: https://espressif.github.io/arduino-esp32/package_esp32_index.json

2. 安装开发板
   - 工具 → 开发板管理器
   - 搜索 "ESP32" 并安装

3. 选择开发板
   - 工具 → 开发板 → ESP32 Arduino → ESP32S3 Dev Module
```

#### PlatformIO (可选)
```bash
pio init --board esp32-s3-devkitc-1
# 复制代码到 src/ 目录
pio run -t upload
```

### 3. 接线

详细接线说明请查看 [WIRING_ESP32S3.md](WIRING_ESP32S3.md)

**快速参考表**:

| ESP32-S3 | 设备 | 功能 |
|----------|------|------|
| **VFD 显示屏** |
| GPIO12 | VFD BK | PWM 亮度 |
| GPIO13 | VFD LAT | 锁存 |
| GPIO14 | VFD CLKG | 栅极时钟 |
| GPIO15 | VFD SIG | 信号 |
| GPIO11 | VFD SIA | SPI MOSI |
| GPIO12 | VFD CLKA | SPI SCK |
| GPIO16 | VFD HV_EN | 高压使能 |
| GPIO17 | VFD FL_EN | 灯丝使能 |
| **AHT20 传感器** |
| GPIO21 | AHT20 SDA | I2C 数据 |
| GPIO22 | AHT20 SCL | I2C 时钟 |
| 3.3V | AHT20 VCC | 传感器电源 |
| **电源** |
| 3.3V | - | 电源正极 |
| GND | - | 公共地 |

⚠️ **如果 VFD 是 5V 逻辑，VFD 信号线需通过电平转换器连接！**

⚠️ **AHT20 是 3.3V 设备，直接连接 ESP32-S3，无需电平转换！**

### 4. 编译上传

```
1. 打开 ESP32_S3_GP1211AI.ino
2. 选择开发板: ESP32S3 Dev Module
3. 选择端口
4. 点击上传
5. 打开串口监视器 (115200 波特率)
```

### 5. 验证运行

串口输出应该显示:

```
==========================================
ESP32-S3 GP1211AI VFD Display Driver
==========================================

✓ GPIO 初始化完成
✓ LEDC PWM 配置完成
✓ SPI 初始化完成 (8MHz, LSB First)
✓ 灯丝预热中...
✓ 硬件定时器启动 (188us 周期)
✓ 高压开启，VFD 准备就绪

按键功能:
  K_U (GPIO18): 增加亮度
  K_D (GPIO19): 减少亮度
  K_M (GPIO20): 菜单
```

## 代码结构

```
ESP32_S3_GP1211AI/
├── ESP32_S3_GP1211AI.ino    # 主程序
├── README_ESP32S3.md         # 本文档
├── WIRING_ESP32S3.md         # 详细接线指南
└── MIGRATION_ESP32S3.md      # 迁移总结

共享资源（来自 Arduino_GP1211AI）:
├── ASC57.h                   # 5×7 字体
├── ASC816.h                  # 8×16 字体
├── ASC1224.h                 # 12×24 字体
└── my_img.h                  # 图像数据
```

## 主要函数

### 显示函数

```cpp
// 清屏
void DP_RAM_CLR(void);

// 显示字符串（5×7 字体）
void VFD_DISP_ASC57_STR(unsigned char row, unsigned char col, const char *str);

// 显示字符串（8×16 字体）
void VFD_DISP_ASC816_STR(unsigned char row, unsigned char col, const char *str);

// 显示全屏图像（128×64）
void VFD_DISP_PIC_12864(const unsigned char *imageData);

// 更新显示缓冲区（必须调用才会刷新屏幕）
void Disp_Buf_Update(void);

// 显示时间
void Show_Timer(unsigned char row, unsigned char col);
```

### AHT20 温湿度传感器函数

```cpp
// 初始化 AHT20 传感器
bool AHT20_Init();

// 读取温湿度数据
// @param temp: 温度输出 (°C)
// @param humi: 湿度输出 (%)
// @return: true=成功, false=失败
bool AHT20_ReadData(float *temp, float *humi);

// 在 VFD 屏幕上显示温湿度
void Display_TempHumi(unsigned char row, unsigned char col);
```

### 使用示例

```cpp
void loop() {
    // 读取温湿度（每 2 秒）
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 2000) {
        lastRead = millis();
        AHT20_ReadData(&temperature, &humidity);
    }

    // 清屏
    DP_RAM_CLR();

    // 显示标题
    VFD_DISP_ASC816_STR(0, 0, "ESP32-S3 VFD");

    // 显示温湿度
    Display_TempHumi(3, 0);  // 第3行开始显示

    // 显示运行时间
    Show_Timer(6, 0);

    // 刷新屏幕（重要！）
    Disp_Buf_Update();

    delay(100);
}
```

## 性能参数

| 参数 | 数值 |
|------|------|
| CPU 频率 | 240MHz |
| 刷新周期 | 188μs |
| 刷新率 | ~265Hz |
| SPI 速度 | 8MHz (可调) |
| 定时器精度 | 1μs |
| PWM 分辨率 | 8位 (0-255) |
| 内存占用 | ~3KB |

## 高级功能（可扩展）

ESP32-S3 强大的硬件让以下功能成为可能：

### WiFi 远程控制
```cpp
#include <WiFi.h>

void setup() {
    WiFi.begin("SSID", "PASSWORD");
    // 通过 WiFi 接收显示内容
}
```

### 蓝牙控制
```cpp
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;
void setup() {
    SerialBT.begin("VFD_Display");
    // 通过蓝牙接收显示内容
}
```

### Web 服务器
```cpp
#include <WebServer.h>

WebServer server(80);
// 创建 Web 界面控制 VFD
```

### 双核任务
```cpp
// 核心 0: VFD 刷新（高优先级）
// 核心 1: 网络通信、用户输入
xTaskCreatePinnedToCore(vfdTask, "VFD", 4096, NULL, 1, NULL, 0);
```

## 故障排除

### 编译错误

**问题**: `hw_timer_t` 未定义
- **解决**: 安装 ESP32 开发板支持包 >= 2.0.0

**问题**: `IRAM_ATTR` 未定义
- **解决**: 确认选择了 ESP32S3 Dev Module

### 硬件问题

**问题**: VFD 不亮
- 检查电平转换器（如使用）
- 检查电源电压 (3.3V vs 5V)
- 查看串口输出错误信息

**问题**: 显示混乱
- 降低 SPI 速度（改为 4MHz）
- 检查信号线连接
- 缩短连接线长度

**问题**: 亮度无法调节
- 检查 GPIO12 (BK) 连接
- 确认 LEDC 配置正确
- 查看 `Disp_Brt_Data` 变量值

### 性能问题

**问题**: 刷新率低
- 检查定时器配置（应该是 188μs）
- 减少 `loop()` 中的延时

**问题**: 闪烁
- 增加 `Disp_Buf_Update()` 调用频率
- 检查电源稳定性

## 与 Arduino 版本对比

| 特性 | Arduino | ESP32-S3 | 说明 |
|------|---------|----------|------|
| CPU | 16MHz | 240MHz | ESP32 快 15 倍 |
| RAM | 2KB | 512KB | 内存不再是限制 |
| 定时器库 | TimerOne | 硬件定时器 | ESP32 更精确 |
| PWM | analogWrite | LEDC | ESP32 16 位分辨率 |
| 扩展性 | 有限 | WiFi/BT | ESP32 支持无线 |
| 调试 | 需要 USB-TTL | USB CDC | ESP32 内置 |
| 成本 | ~$3 | ~$4 | 价格相近 |

**结论**: ESP32-S3 在各方面都优于 Arduino，且价格相近，强烈推荐使用。

## 许可和致谢

- **原始代码**: DONGFENG (dongfeng.dream@126.com)
- **Arduino 移植**: Claude Code (2025-12-25)
- **ESP32-S3 移植**: Claude Code (2026-01-12)

本项目仅供学习参考使用。

## 相关文档

- [接线指南](WIRING_ESP32S3.md) - 详细的硬件连接说明
- [迁移总结](MIGRATION_ESP32S3.md) - 从 Arduino 到 ESP32-S3 的迁移记录
- [Arduino 版本](../Arduino_GP1211AI/README.md) - 原始 Arduino 版本

## 支持和反馈

如有问题或建议，请提交 Issue 或 Pull Request。

---

**享受 ESP32-S3 带来的强大性能！** 🚀
