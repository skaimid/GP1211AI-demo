# ESP32-S3 天气仪表板项目架构文档

## 项目概述

这是一个基于ESP32-S3和GP1211AI VFD显示屏的智能天气仪表板项目，采用模块化架构设计，易于维护和扩展。

## 代码架构

### 模块化设计原则

项目采用面向对象的模块化设计，将不同功能封装到独立的类中，每个模块负责特定的功能领域。

```
ESP32_S3_GP1211AI/
├── ESP32_S3_GP1211AI.ino    # 主程序（简洁的setup和loop）
├── config.h                  # 配置文件（WiFi、API等）
├── weather_icons.h           # 天气图标数据
│
├── vfd_display.h/cpp         # VFD显示驱动模块
├── aht20_sensor.h/cpp        # AHT20温湿度传感器模块
├── wifi_manager.h/cpp        # WiFi和NTP时间管理模块
├── weather_api.h/cpp         # 天气API模块
├── ui_display.h/cpp          # UI显示和布局模块
└── button_handler.h/cpp      # 按钮输入处理模块
```

## 模块详解

### 1. VFD Display Module (`vfd_display.h/cpp`)

**职责**：VFD显示屏底层驱动

**核心功能**：
- 硬件初始化（GPIO、SPI、PWM、定时器）
- 显存管理和刷新
- 字符绘制（5×7、8×16字体）
- 图标绘制（8×8、16×16）
- 亮度控制

**关键接口**：
```cpp
class VFDDisplay {
    bool begin();                           // 初始化
    void clear();                           // 清屏
    void update();                          // 更新显示
    void setBrightness(uint8_t value);      // 设置亮度
    void drawString8x16(uint8_t row, uint8_t col, const char* str);
    void drawIcon16x16(uint8_t row, uint8_t col, const uint8_t* icon);
};
```

**技术细节**：
- 使用硬件定时器（188μs周期）实现扫描刷新
- SPI传输速度：8MHz
- LEDC PWM控制亮度（8位分辨率）
- 显示缓冲区：1KB显存 + 2KB SPI缓冲

---

### 2. AHT20 Sensor Module (`aht20_sensor.h/cpp`)

**职责**：AHT20温湿度传感器驱动

**核心功能**：
- I2C通信初始化
- 触发测量和数据读取
- 温湿度计算
- 传感器状态检查

**关键接口**：
```cpp
class AHT20Sensor {
    bool begin();                // 初始化
    bool read();                 // 读取数据
    float getTemperature();      // 获取温度
    float getHumidity();         // 获取湿度
    bool isReady();              // 检查就绪状态
};
```

**技术细节**：
- I2C地址：0x38
- I2C频率：100kHz
- 测量周期：80ms
- 精度：温度±0.3°C，湿度±3%RH

---

### 3. WiFi Manager Module (`wifi_manager.h/cpp`)

**职责**：WiFi连接和NTP时间管理

**核心功能**：
- WiFi连接管理
- NTP时间同步
- 时间格式化
- 网络状态监控

**关键接口**：
```cpp
class WiFiManager {
    bool connect(const char* ssid, const char* password);
    bool syncTime(const char* ntpServer, long offset);
    bool isConnected();
    String getTimeString();      // "HH:MM:SS"
    String getDateString();      // "MM/DD"
};
```

**技术细节**：
- 连接超时：20次尝试（10秒）
- NTP服务器：pool.ntp.org
- 时区支持：UTC偏移量配置
- 自动重连机制

---

### 4. Weather API Module (`weather_api.h/cpp`)

**职责**：天气数据获取和管理

**核心功能**：
- OpenWeatherMap API集成
- 当前天气获取
- 小时预报获取
- 天气图标映射
- 自动更新管理

**关键接口**：
```cpp
class WeatherAPI {
    void configure(const char* apiKey, const char* city, const char* country);
    bool fetchCurrentWeather();
    bool fetchHourlyForecast();
    const WeatherData& getCurrentWeather();
    const uint8_t* getWeatherIcon(int weatherId);
    bool needsUpdate();          // 检查是否需要更新
};
```

**数据结构**：
```cpp
struct WeatherData {
    float temp;           // 温度 (°C)
    int humidity;         // 湿度 (%)
    String description;   // 天气描述
    int weatherId;        // 天气ID
    bool valid;           // 数据有效性
};
```

**技术细节**：
- 更新间隔：10分钟
- HTTP客户端：ESP32 HTTPClient
- JSON解析：ArduinoJson库
- 天气ID映射：200-899（雷暴到多云）

---

### 5. UI Display Module (`ui_display.h/cpp`)

**职责**：用户界面显示和布局管理

**核心功能**：
- 显示模式管理（默认/小时预报）
- UI渲染逻辑
- 自动返回机制
- 数据整合显示

**关键接口**：
```cpp
class UIDisplay {
    void render();               // 渲染当前模式
    void renderDefaultMode();    // 渲染默认模式
    void renderHourlyMode();     // 渲染小时预报模式
    void switchMode();           // 切换模式
    bool checkAutoReturn();      // 检查自动返回
};
```

**显示模式**：

#### 默认模式（MODE_DEFAULT）
```
┌────────────────────────────────────┐
│ 16:45:23          05/15            │  ← 时间和日期
├────────────────────────────────────┤
│                                    │
│  [☀️]  25°C      [🌡] 22.5°C      │  ← 外部天气 | 室内温度
│                                    │
│  Sunny           [💧] 65.2%       │  ← 天气描述 | 室内湿度
│                                    │
└────────────────────────────────────┘
```

#### 小时预报模式（MODE_HOURLY）
```
┌────────────────────────────────────┐
│ HOURLY                        15s  │  ← 倒计时
├────────────────────────────────────┤
│ 24°C  25°C  26°C  27°C            │  ← 前4小时
│                                    │
│ 26°C  25°C  24°C  23°C            │  ← 后4小时
│                                    │
└────────────────────────────────────┘
```

**技术细节**：
- 自动返回时间：20秒
- 布局协调：整合多个数据源
- 实时刷新：每100ms更新

---

### 6. Button Handler Module (`button_handler.h/cpp`)

**职责**：按钮输入处理

**核心功能**：
- 按键扫描和防抖
- 亮度调节
- 模式切换
- 功能响应

**关键接口**：
```cpp
class ButtonHandler {
    void begin();                // 初始化
    void update();               // 更新按键状态
};
```

**按钮功能**：
- **K_U (GPIO18)**：亮度增加（+10，最大250）
- **K_D (GPIO19)**：亮度减少（-10，最小0）
- **K_M (GPIO20)**：模式切换（默认↔小时预报）

**技术细节**：
- 防抖时间：200ms
- 上拉输入模式
- 阻塞等待释放

---

## 主程序（ESP32_S3_GP1211AI.ino）

### Setup流程

```cpp
void setup() {
    1. 初始化VFD显示驱动
    2. 初始化AHT20传感器
    3. 初始化按钮
    4. 连接WiFi
    5. 同步NTP时间
    6. 配置天气API
    7. 获取天气数据
}
```

### Loop流程

```cpp
void loop() {
    1. 处理按键输入
    2. 定期读取AHT20（每2秒）
    3. 检查天气更新（每10分钟）
    4. 检查自动返回
    5. 渲染UI
    6. 延迟100ms
}
```

**代码行数对比**：
- **原版**：787行（单文件）
- **重构后**：主文件仅102行，总代码约1200行（分布在7个模块）

---

## 配置文件（config.h）

```cpp
// WiFi配置
#define WIFI_SSID     "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// 时区配置
#define TIMEZONE_OFFSET 8 * 3600  // UTC+8
#define NTP_SERVER "pool.ntp.org"

// 天气API配置
#define WEATHER_API_KEY "your_api_key"
#define WEATHER_CITY    "Beijing"
#define WEATHER_COUNTRY "CN"
```

---

## 依赖库

### Arduino库
- **SPI.h**：VFD显示屏通信
- **Wire.h**：AHT20传感器I2C通信
- **WiFi.h**：网络连接
- **HTTPClient.h**：天气API请求
- **ArduinoJson.h**：JSON数据解析（v6.x）
- **time.h**：NTP时间管理

### 外部资源
- **ASC57.h/ASC816.h**：字体库（5×7、8×16）
- **weather_icons.h**：天气图标数据

---

## 数据流图

```
┌─────────────┐
│   Setup     │ 初始化所有模块
└──────┬──────┘
       │
       v
┌─────────────────────────────────────────┐
│            Main Loop                     │
└─────────────────────────────────────────┘
       │
       ├──> ButtonHandler.update()
       │    └─> VFDDisplay.setBrightness()
       │    └─> UIDisplay.switchMode()
       │
       ├──> AHT20Sensor.read()
       │    └─> 温度/湿度数据
       │
       ├──> WeatherAPI.needsUpdate()
       │    └─> WeatherAPI.fetch()
       │        └─> WeatherData
       │
       ├──> UIDisplay.checkAutoReturn()
       │    └─> 模式自动切换
       │
       └──> UIDisplay.render()
            ├─> WiFiManager.getTime()
            ├─> WeatherAPI.getWeather()
            ├─> AHT20Sensor.getData()
            └─> VFDDisplay.draw*()
                └─> VFDDisplay.update()
```

---

## 模块依赖关系

```
ESP32_S3_GP1211AI.ino (主程序)
    │
    ├─> config.h (配置)
    │
    ├─> VFDDisplay (显示驱动)
    │   └─> weather_icons.h
    │
    ├─> AHT20Sensor (传感器)
    │
    ├─> WiFiManager (网络管理)
    │
    ├─> WeatherAPI (天气API)
    │   └─> weather_icons.h
    │
    ├─> UIDisplay (UI渲染)
    │   ├─> VFDDisplay
    │   ├─> AHT20Sensor
    │   ├─> WiFiManager
    │   ├─> WeatherAPI
    │   └─> weather_icons.h
    │
    └─> ButtonHandler (按钮处理)
        ├─> VFDDisplay
        └─> UIDisplay
```

---

## 模块化设计优势

### 1. 可维护性
- **单一职责**：每个模块只负责一个功能领域
- **清晰结构**：代码组织清晰，易于理解
- **独立测试**：可单独测试每个模块

### 2. 可扩展性
- **新增传感器**：只需创建新的传感器模块
- **更换显示屏**：只需修改VFDDisplay模块
- **添加功能**：不影响现有模块

### 3. 可复用性
- **模块独立**：可在其他项目中复用
- **接口标准**：统一的API设计
- **配置分离**：config.h集中管理配置

### 4. 团队协作
- **并行开发**：不同成员开发不同模块
- **版本控制**：更小的文件，更少的冲突
- **代码审查**：更容易review单个模块

---

## 编译配置

### Arduino IDE
```
开发板：ESP32S3 Dev Module
Flash Size：16MB
PSRAM：Enabled
Upload Speed：921600
USB CDC On Boot：Enabled
```

### PlatformIO
```ini
[env:esp32-s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson@^6.21.0
```

---

## 性能指标

| 指标 | 数值 |
|------|------|
| VFD刷新率 | ~265Hz |
| SPI速度 | 8MHz |
| 传感器采样 | 2秒/次 |
| 天气更新 | 10分钟/次 |
| UI刷新 | 100ms/次 |
| RAM使用 | ~15KB |
| Flash使用 | ~1.2MB |

---

## 开发指南

### 添加新模块

1. **创建头文件** (`new_module.h`)
```cpp
#ifndef NEW_MODULE_H
#define NEW_MODULE_H

class NewModule {
public:
    NewModule();
    bool begin();
    void update();
};

extern NewModule newModule;

#endif
```

2. **创建实现文件** (`new_module.cpp`)
```cpp
#include "new_module.h"

NewModule newModule;

NewModule::NewModule() {}

bool NewModule::begin() {
    // 初始化代码
    return true;
}

void NewModule::update() {
    // 更新逻辑
}
```

3. **在主程序中使用**
```cpp
#include "new_module.h"

void setup() {
    newModule.begin();
}

void loop() {
    newModule.update();
}
```

### 修改UI布局

编辑 `ui_display.cpp` 中的渲染函数：
- `renderDefaultMode()`：默认布局
- `renderHourlyMode()`：小时预报布局

### 添加新的天气图标

在 `weather_icons.h` 中定义16×16点阵：
```cpp
const unsigned char PROGMEM icon_custom[] = {
    0x00, 0x00, ... // 32字节数据
};
```

在 `weather_api.cpp` 中添加映射：
```cpp
const uint8_t* WeatherAPI::getWeatherIcon(int weatherId) const {
    if (weatherId == xxx) {
        return icon_custom;
    }
    // ...
}
```

---

## 调试技巧

### 串口监视器输出

项目包含详细的串口日志：
```
==========================================
ESP32-S3 天气仪表板 VFD Display
模块化架构 v2.0
==========================================

初始化VFD显示驱动...
✓ VFD GPIO初始化完成
✓ VFD PWM初始化完成
✓ VFD SPI初始化完成
✓ VFD灯丝预热中...
✓ VFD高压开启，准备就绪
✓ VFD定时器启动 (188μs)
✓ VFD显示驱动初始化完成

初始化AHT20传感器...
✓ AHT20初始化成功
  温度: 22.5°C, 湿度: 65.2%

✓ 按钮初始化完成
  K_U (GPIO18): 增加亮度
  K_D (GPIO19): 减少亮度
  K_M (GPIO20): 切换显示模式

连接WiFi...
SSID: YourWiFi
..........
✓ WiFi连接成功
  IP地址: 192.168.1.100

同步时间...
✓ 时间同步成功
  当前时间: 2026-01-12 16:45:23

天气API配置完成
  城市: Beijing, CN

获取当前天气...
✓ 天气数据获取成功
  温度: 25.0°C
  湿度: 60%
  状况: clear sky

==========================================
系统初始化完成！
==========================================
```

### 常见问题排查

| 问题 | 模块 | 解决方法 |
|------|------|----------|
| 显示屏不亮 | VFDDisplay | 检查引脚连接和电源 |
| 传感器读取失败 | AHT20Sensor | 检查I2C连接（SDA/SCL） |
| WiFi连接失败 | WiFiManager | 检查SSID和密码 |
| 天气获取失败 | WeatherAPI | 检查API密钥和网络 |
| 按钮无响应 | ButtonHandler | 检查上拉电阻和引脚 |

---

## 版本历史

### v2.0 (2026-01-12) - 模块化重构
- ✨ 完全模块化架构
- ✨ 代码拆分为6个独立模块
- ✨ 主程序简化为102行
- ✨ 添加详细的模块文档
- 📦 改进代码组织结构
- 🐛 修复潜在的内存问题

### v1.0 (2026-01-12) - 初始版本
- ✨ 实现所有核心功能
- ✨ WiFi连接和NTP时间同步
- ✨ 天气API集成
- ✨ 双模式UI显示
- ✨ 按钮控制

---

## 许可证

本项目基于MIT许可证开源。

## 作者

ESP32-S3移植、天气仪表板功能开发和模块化重构

---

## 结语

通过模块化设计，本项目实现了：
- **主程序从787行减少到102行（-87%）**
- **代码可读性大幅提升**
- **维护和扩展更加容易**
- **团队协作更加高效**

这是一个展示如何将单文件项目重构为模块化架构的优秀案例。
