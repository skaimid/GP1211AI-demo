# ESP32-S3 天气仪表板 VFD 显示

基于 GP1211AI VFD 显示屏的智能天气仪表板，集成 WiFi、NTP 时间同步、天气 API 和 AHT20 温湿度传感器。

## 功能特性

✅ **WiFi 连接**：自动连接到配置的 WiFi 网络
✅ **NTP 时间同步**：自动同步网络时间，显示实时时钟
✅ **外部天气显示**：通过 OpenWeatherMap API 获取实时天气和温度
✅ **天气图标**：6 种天气状态的 16×16 点阵图标（晴、云、雨、雪、雷暴、雾）
✅ **室内温湿度**：通过 AHT20 传感器实时监测室内环境
✅ **双模式显示**：
  - **默认模式**：时间 + 外部天气 + 室内温湿度
  - **小时预报模式**：未来 8 小时天气预报
✅ **智能切换**：按钮切换模式，20 秒后自动返回默认模式

## 硬件要求

### 主要组件
- **ESP32-S3** 开发板
- **GP1211AI VFD** 显示屏（128×64 像素）
- **AHT20** 温湿度传感器（I2C）
- 电平转换器（TXS0108E 或 74HC245）
- 3 个按键（K_U、K_D、K_M）

### 引脚连接

| 功能 | ESP32-S3 GPIO | 说明 |
|------|---------------|------|
| **VFD 控制** |
| VFD_SIG | GPIO 37 | 信号控制 |
| VFD_CLKG | GPIO 38 | 栅极时钟 |
| VFD_LAT | GPIO 39 | 数据锁存 |
| VFD_BK | GPIO 40 | PWM 亮度控制 |
| VFD_CLKA | GPIO 35 | SPI 时钟（SCK） |
| VFD_SIA | GPIO 36 | SPI 数据（MOSI） |
| HV_EN | GPIO 41 | 高压使能 |
| FL_EN | GPIO 42 | 灯丝使能 |
| **I2C 传感器** |
| SDA | GPIO 4 | AHT20 数据线 |
| SCL | GPIO 5 | AHT20 时钟线 |
| **按键** |
| K_U | GPIO 18 | 增加亮度 |
| K_D | GPIO 19 | 减少亮度 |
| K_M | GPIO 20 | 模式切换 |

## 软件配置

### 1. 安装依赖库

在 Arduino IDE 中安装以下库：

```
- ArduinoJson (v6.x)
- WiFi (ESP32 内置)
- HTTPClient (ESP32 内置)
- Wire (I2C, 内置)
- SPI (内置)
```

### 2. 配置 WiFi 和 API

编辑 `config.h` 文件：

```cpp
// WiFi 配置
#define WIFI_SSID     "你的WiFi名称"      // 修改为你的WiFi SSID
#define WIFI_PASSWORD "你的WiFi密码"      // 修改为你的WiFi密码

// 时区配置（中国标准时间 UTC+8）
#define TIMEZONE_OFFSET 8 * 3600
#define NTP_SERVER "pool.ntp.org"

// 天气API配置
#define WEATHER_API_KEY "你的API密钥"     // 从 OpenWeatherMap 获取
#define WEATHER_CITY    "Beijing"         // 你的城市名称
#define WEATHER_COUNTRY "CN"              // 国家代码
```

### 3. 获取 OpenWeatherMap API 密钥

1. 访问 [https://openweathermap.org/api](https://openweathermap.org/api)
2. 注册免费账户
3. 在 "API Keys" 页面生成 API 密钥
4. 将密钥复制到 `config.h` 中的 `WEATHER_API_KEY`

**注意**：免费账户有以下限制：
- 每分钟 60 次调用
- 每天 1,000,000 次调用
- 本项目配置为每 10 分钟更新一次，完全满足免费额度

## 编译和上传

### 使用 Arduino IDE

1. 打开 `ESP32_S3_GP1211AI.ino`
2. 选择开发板：**工具 → 开发板 → ESP32 Arduino → ESP32S3 Dev Module**
3. 配置参数：
   - **Flash Size**: 16MB (或根据你的板子选择)
   - **PSRAM**: Enabled (如果有 PSRAM)
   - **Upload Speed**: 921600
   - **USB CDC On Boot**: Enabled
4. 选择正确的端口
5. 点击 "上传"

### 使用 PlatformIO

如果使用 PlatformIO，创建 `platformio.ini`：

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson@^6.21.0
```

然后运行：
```bash
pio run --target upload
pio device monitor
```

## 使用说明

### 显示布局

#### 默认模式
```
┌────────────────────────────────────┐
│ 16:45:23          05/15            │  ← 时间和日期
├────────────────────────────────────┤
│                                    │
│  [图标]  25°C      [T] 22.5°C     │  ← 外部天气 | 室内温度
│                                    │
│  Sunny            [H] 65.2%       │  ← 天气描述 | 室内湿度
│                                    │
└────────────────────────────────────┘
```

#### 小时预报模式（按 K_M 切换）
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

### 按键功能

| 按键 | 功能 |
|------|------|
| **K_U** (GPIO18) | 增加显示亮度 (+10，最大 250) |
| **K_D** (GPIO19) | 减少显示亮度 (-10，最小 0) |
| **K_M** (GPIO20) | 切换显示模式（默认 ↔ 小时预报） |

### 自动功能

- **时间同步**：启动时自动通过 NTP 同步
- **天气更新**：每 10 分钟自动更新天气数据
- **传感器读取**：每 2 秒读取 AHT20 温湿度
- **自动返回**：小时预报模式 20 秒后自动返回默认模式

## 调试和故障排除

### 串口监视器

打开串口监视器（115200 波特率）查看详细日志：

```
==========================================
ESP32-S3 天气仪表板 VFD Display
==========================================

✓ GPIO 初始化完成
✓ SPI 初始化完成
✓ 灯丝预热中...
✓ 硬件定时器启动
✓ VFD 准备就绪

✓ AHT20 初始化成功
  温度: 22.5°C
  湿度: 65.2%

连接WiFi...
SSID: YourWiFi
..........
✓ WiFi连接成功
  IP地址: 192.168.1.100

同步时间...
✓ 时间同步成功
  当前时间: 2026-01-12 16:45:23

获取当前天气...
✓ 天气数据获取成功
  温度: 25.0°C
  湿度: 60%
  状况: clear sky

获取小时预报...
✓ 小时预报获取成功

按键功能:
  K_U (GPIO18): 增加亮度
  K_D (GPIO19): 减少亮度
  K_M (GPIO20): 切换显示模式
```

### 常见问题

#### WiFi 连接失败
- 检查 `config.h` 中的 SSID 和密码是否正确
- 确认 WiFi 是 2.4GHz（ESP32 不支持 5GHz）
- 检查信号强度

#### 时间不正确
- 确认 WiFi 已连接
- 检查 `TIMEZONE_OFFSET` 是否正确（中国为 8 * 3600）
- 尝试更换 NTP 服务器（如 `time.google.com`）

#### 天气获取失败
- 确认 WiFi 已连接
- 检查 API 密钥是否正确
- 验证城市名称拼写（如 "Beijing", "Shanghai"）
- 检查 API 调用额度（登录 OpenWeatherMap 查看）

#### AHT20 初始化失败
- 检查 I2C 接线（SDA=GPIO4, SCL=GPIO5）
- 确认传感器供电（3.3V）
- 使用 I2C 扫描器确认地址（应为 0x38）

#### 显示屏不亮
- 检查 VFD 电源连接（高压模块）
- 确认引脚连接正确
- 查看串口日志中的初始化信息
- 尝试增加亮度（K_U 按钮）

## 天气图标说明

系统根据天气 ID 自动选择对应图标：

| 天气状态 | ID 范围 | 图标 |
|---------|---------|------|
| 雷暴 | 200-299 | ⚡ 闪电图标 |
| 雨天 | 300-599 | 🌧 雨滴图标 |
| 雪天 | 600-699 | ❄️ 雪花图标 |
| 雾/霾 | 700-799 | 🌫 横线图标 |
| 晴天 | 800 | ☀️ 太阳图标 |
| 多云 | 801-899 | ☁️ 云朵图标 |

## 性能指标

- **显示刷新率**：~265Hz（流畅无闪烁）
- **天气更新间隔**：10 分钟
- **传感器采样**：2 秒
- **时间精度**：NTP 同步（±100ms）
- **温度精度**：±0.3°C（AHT20）
- **湿度精度**：±3%RH（AHT20）

## API 使用说明

### OpenWeatherMap API

#### 当前天气
```
GET http://api.openweathermap.org/data/2.5/weather
参数：
  - q: 城市名,国家代码（如 "Beijing,CN"）
  - appid: API密钥
  - units: metric（摄氏度）
```

#### 5天/3小时预报
```
GET http://api.openweathermap.org/data/2.5/forecast
参数：
  - q: 城市名,国家代码
  - appid: API密钥
  - units: metric
  - cnt: 8（获取8条数据）
```

## 扩展开发

### 添加新的天气图标

编辑 `weather_icons.h`，添加 16×16 点阵数据：

```cpp
const unsigned char PROGMEM icon_custom[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ... 32字节数据（16×16 / 8）
};
```

### 更改显示布局

修改 `Display_Default_UI()` 或 `Display_Hourly_UI()` 函数。

VFD 坐标系统：
- 显示区域：128×64 像素
- 行（X）：0-7（每行 8 像素高）
- 列（Y）：0-127（像素宽度）

字体选择：
- `VFD_DISP_ASC57()`: 5×7 小字体
- `VFD_DISP_ASC816()`: 8×16 大字体
- `VFD_DISP_ASC1224()`: 12×24 超大字体（需引入）

### 添加其他传感器

使用 I2C 或 SPI 总线添加更多传感器：
- **BME280**：温湿度气压传感器
- **BH1750**：光照传感器
- **CCS811**：空气质量传感器（eCO2/TVOC）

## 许可证

本项目基于原始 GP1211AI 驱动代码扩展，遵循相同的开源协议。

## 作者

ESP32-S3 移植及天气仪表板功能开发

## 更新日志

### v2.0.0 (2026-01-12)
- ✨ 添加 WiFi 连接功能
- ✨ 集成 NTP 时间同步
- ✨ 接入 OpenWeatherMap API
- ✨ 创建天气图标库
- ✨ 设计双模式 UI 布局
- ✨ 实现按钮模式切换
- ✨ 支持自动返回默认模式

### v1.0.0 (初始版本)
- 基础 VFD 驱动
- AHT20 温湿度传感器支持
- 简单信息显示

## 支持和反馈

遇到问题或有改进建议？
- 查看串口监视器日志
- 参考上述故障排除章节
- 检查硬件连接

---

**享受你的智能天气仪表板！** 🌤️📟
