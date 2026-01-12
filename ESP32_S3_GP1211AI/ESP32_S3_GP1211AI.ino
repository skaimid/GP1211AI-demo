#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// ==================== ESP32-S3 平台说明 ====================
// 本代码适用于 ESP32-S3 芯片，从 Arduino 版本迁移而来
// 功能：
// 1. WiFi连接和NTP时间同步
// 2. 显示实时时间
// 3. 显示外部天气和气温（带图标）
// 4. 显示AHT20传感器温湿度
// 5. 按钮切换到小时天气预报
// ===========================================================

// 包含配置和资源文件
#include "config.h"
#include "weather_icons.h"

// 包含字库头文件
#include "../Arduino_GP1211AI/ASC1224.h"
#include "../Arduino_GP1211AI/ASC816.h"
#include "../Arduino_GP1211AI/ASC57.h"
#include "../Arduino_GP1211AI/my_img.h"

// ==================== ESP32-S3 引脚定义 ====================
#define VFD_SIG_PIN  GPIO_NUM_37  // 信号控制
#define VFD_CLKG_PIN GPIO_NUM_38  // 栅极时钟
#define VFD_LAT_PIN  GPIO_NUM_39  // 数据锁存
#define VFD_BK_PIN   GPIO_NUM_40  // PWM 亮度控制

// ESP32-S3 HSPI 引脚
#define VFD_CLKA_PIN GPIO_NUM_35  // SCK
#define VFD_SIA_PIN  GPIO_NUM_36  // MOSI

// 电源管理引脚
#define HV_EN_PIN    GPIO_NUM_41  // 高压使能
#define FL_EN_PIN    GPIO_NUM_42  // 灯丝使能

// 按键引脚
#define K_U_PIN      GPIO_NUM_18  // 增加亮度
#define K_D_PIN      GPIO_NUM_19  // 减少亮度
#define K_M_PIN      GPIO_NUM_20  // 菜单/切换布局

// I2C 引脚 (AHT20 温湿度传感器)
#define I2C_SDA_PIN  GPIO_NUM_4  // I2C 数据线
#define I2C_SCL_PIN  GPIO_NUM_5  // I2C 时钟线
#define I2C_FREQ     100000      // I2C 频率 100kHz

// AHT20 配置
#define AHT20_ADDR   0x38        // AHT20 I2C 地址

// ==================== LEDC PWM 配置 ====================
#define LEDC_FREQUENCY      5000        // 5kHz PWM 频率
#define LEDC_RESOLUTION     8           // 8位分辨率 (0-255)

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

// SPI 对象
SPIClass *vspi = NULL;

// AHT20 温湿度数据
float temperature = 0.0;    // 温度 (°C)
float humidity = 0.0;       // 湿度 (%)
unsigned long lastSensorRead = 0;  // 上次读取传感器的时间
#define SENSOR_READ_INTERVAL 2000  // 传感器读取间隔 (ms)

// WiFi 和时间
bool wifiConnected = false;
struct tm timeinfo;
char timeBuffer[32];
char dateBuffer[32];

// 天气数据
struct WeatherData {
    float temp;
    int humidity;
    String description;
    String icon;
    int weatherId;
};

WeatherData currentWeather;
WeatherData hourlyForecast[8];  // 存储8小时预报
unsigned long lastWeatherUpdate = 0;
bool weatherDataValid = false;

// UI 状态
enum DisplayMode {
    MODE_DEFAULT,      // 默认布局
    MODE_HOURLY        // 小时预报布局
};

DisplayMode currentMode = MODE_DEFAULT;
unsigned long modeChangeTime = 0;
#define MODE_AUTO_RETURN_TIME 20000  // 20秒后自动返回

// 按钮防抖
unsigned long lastButtonPress = 0;
#define BUTTON_DEBOUNCE 200

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

// 显示16x16天气图标
void Display_Weather_Icon(unsigned char x, unsigned char y, const unsigned char* icon) {
    for (unsigned char row = 0; row < 2; row++) {
        for (unsigned char col = 0; col < 16; col++) {
            DP_RAM[x + row][y + col] = pgm_read_byte(&icon[row * 16 + col]);
        }
    }
}

// 显示8x8小图标
void Display_Small_Icon(unsigned char x, unsigned char y, const unsigned char* icon) {
    for (unsigned char i = 0; i < 8; i++) {
        DP_RAM[x][y + i] = pgm_read_byte(&icon[i]);
    }
}

// ==================== ESP32 硬件定时器中断 ====================
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
    ledcWrite(VFD_BK_PIN, 0);           // 消隐
    VFD_LAT_STROBE();                     // 锁存数据
    ledcWrite(VFD_BK_PIN, Disp_Brt_Data); // 恢复亮度

    VFD_GRID_SCAN--;

    portEXIT_CRITICAL_ISR(&timerMux);
}

// ==================== WiFi 连接函数 ====================

void Connect_WiFi() {
    Serial.println("\n连接WiFi...");
    Serial.printf("SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\n✓ WiFi连接成功");
        Serial.print("  IP地址: ");
        Serial.println(WiFi.localIP());
    } else {
        wifiConnected = false;
        Serial.println("\n✗ WiFi连接失败");
    }
}

// ==================== NTP 时间同步函数 ====================

void Sync_Time() {
    if (!wifiConnected) return;

    Serial.println("同步时间...");
    configTime(TIMEZONE_OFFSET, 0, NTP_SERVER);

    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (getLocalTime(&timeinfo)) {
        Serial.println("\n✓ 时间同步成功");
        Serial.printf("  当前时间: %04d-%02d-%02d %02d:%02d:%02d\n",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        Serial.println("\n✗ 时间同步失败");
    }
}

// ==================== 天气API函数 ====================

const unsigned char* Get_Weather_Icon(int weatherId) {
    // 根据OpenWeatherMap的天气ID返回对应图标
    if (weatherId >= 200 && weatherId < 300) {
        return icon_thunderstorm;  // 雷暴
    } else if (weatherId >= 300 && weatherId < 600) {
        return icon_rainy;  // 雨
    } else if (weatherId >= 600 && weatherId < 700) {
        return icon_snowy;  // 雪
    } else if (weatherId >= 700 && weatherId < 800) {
        return icon_mist;  // 雾/霾
    } else if (weatherId == 800) {
        return icon_sunny;  // 晴天
    } else {
        return icon_cloudy;  // 多云
    }
}

bool Fetch_Weather() {
    if (!wifiConnected) {
        Serial.println("✗ WiFi未连接，无法获取天气");
        return false;
    }

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
                 String(WEATHER_CITY) + "," + String(WEATHER_COUNTRY) +
                 "&appid=" + String(WEATHER_API_KEY) + "&units=metric";

    Serial.println("获取当前天气...");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            currentWeather.temp = doc["main"]["temp"];
            currentWeather.humidity = doc["main"]["humidity"];
            currentWeather.description = doc["weather"][0]["description"].as<String>();
            currentWeather.weatherId = doc["weather"][0]["id"];

            weatherDataValid = true;
            Serial.println("✓ 天气数据获取成功");
            Serial.printf("  温度: %.1f°C\n", currentWeather.temp);
            Serial.printf("  湿度: %d%%\n", currentWeather.humidity);
            Serial.printf("  状况: %s\n", currentWeather.description.c_str());

            http.end();
            return true;
        }
    }

    http.end();
    Serial.printf("✗ 天气获取失败 (HTTP: %d)\n", httpCode);
    return false;
}

bool Fetch_Hourly_Forecast() {
    if (!wifiConnected) return false;

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" +
                 String(WEATHER_CITY) + "," + String(WEATHER_COUNTRY) +
                 "&appid=" + String(WEATHER_API_KEY) + "&units=metric&cnt=8";

    Serial.println("获取小时预报...");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            JsonArray list = doc["list"];
            for (int i = 0; i < 8 && i < list.size(); i++) {
                hourlyForecast[i].temp = list[i]["main"]["temp"];
                hourlyForecast[i].humidity = list[i]["main"]["humidity"];
                hourlyForecast[i].weatherId = list[i]["weather"][0]["id"];
                hourlyForecast[i].description = list[i]["weather"][0]["description"].as<String>();
            }

            Serial.println("✓ 小时预报获取成功");
            http.end();
            return true;
        }
    }

    http.end();
    Serial.printf("✗ 小时预报获取失败 (HTTP: %d)\n", httpCode);
    return false;
}

// ==================== AHT20 温湿度传感器函数 ====================

bool AHT20_Init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    delay(40);

    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xBE);
    Wire.write(0x08);
    Wire.write(0x00);
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.printf("✗ AHT20 初始化失败 (错误代码: %d)\n", error);
        return false;
    }

    delay(10);
    Serial.println("✓ AHT20 初始化成功");
    return true;
}

bool AHT20_TriggerMeasurement() {
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    uint8_t error = Wire.endTransmission();

    return (error == 0);
}

bool AHT20_ReadData(float *temp, float *humi) {
    if (!AHT20_TriggerMeasurement()) {
        return false;
    }

    delay(80);

    uint8_t data[6];
    Wire.requestFrom(AHT20_ADDR, 6);

    if (Wire.available() != 6) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        data[i] = Wire.read();
    }

    if (data[0] & 0x80) {
        return false;
    }

    uint32_t raw_humidity = ((uint32_t)data[1] << 12) |
                            ((uint32_t)data[2] << 4) |
                            ((uint32_t)data[3] >> 4);
    *humi = (raw_humidity * 100.0) / 1048576.0;

    uint32_t raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) |
                               ((uint32_t)data[4] << 8) |
                               ((uint32_t)data[5]);
    *temp = (raw_temperature * 200.0) / 1048576.0 - 50.0;

    return true;
}

// ==================== UI 显示函数 ====================

void Display_Default_UI() {
    DP_RAM_CLR();

    // 第一行：显示当前时间
    if (getLocalTime(&timeinfo)) {
        sprintf(timeBuffer, "%02d:%02d:%02d",
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        VFD_DISP_ASC816_STR(0, 0, timeBuffer);

        // 显示日期（右侧小字）
        sprintf(dateBuffer, "%02d/%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
        VFD_DISP_ASC57_STR(0, 10, dateBuffer);
    } else {
        VFD_DISP_ASC816_STR(0, 0, "--:--:--");
    }

    // 下部左侧：外部天气和气温（带图标）
    if (weatherDataValid) {
        // 显示天气图标 (行3-4, 列0-1)
        Display_Weather_Icon(3, 0, Get_Weather_Icon(currentWeather.weatherId));

        // 显示温度 (行3)
        char tempStr[16];
        sprintf(tempStr, "%3.0fC", currentWeather.temp);
        VFD_DISP_ASC816_STR(1, 2, tempStr);

        // 显示天气描述 (行5, 小字)
        VFD_DISP_ASC57_STR(5, 0, currentWeather.description.substring(0, 10).c_str());
    } else {
        VFD_DISP_ASC57_STR(3, 0, "Weather");
        VFD_DISP_ASC57_STR(4, 0, "N/A");
    }

    // 下部右侧：传感器温湿度
    char buffer[16];

    // 温度图标和数值
    Display_Small_Icon(3, 64, icon_temp);
    sprintf(buffer, "%5.1fC", temperature);
    VFD_DISP_ASC816_STR(1, 9, buffer);

    // 湿度图标和数值
    Display_Small_Icon(5, 64, icon_humidity);
    sprintf(buffer, "%5.1f%%", humidity);
    VFD_DISP_ASC816_STR(2, 9, buffer);
}

void Display_Hourly_UI() {
    DP_RAM_CLR();

    // 标题
    VFD_DISP_ASC816_STR(0, 0, "HOURLY");

    // 显示4个时段的预报（分两行）
    for (int i = 0; i < 4; i++) {
        // 第一行：前4小时
        int x = 2;
        int y = i * 30;

        char tempStr[8];
        sprintf(tempStr, "%2.0fC", hourlyForecast[i].temp);
        VFD_DISP_ASC57_STR(x, y / 8, tempStr);

        // 第二行：后4小时
        if (i + 4 < 8) {
            sprintf(tempStr, "%2.0fC", hourlyForecast[i + 4].temp);
            VFD_DISP_ASC57_STR(x + 2, y / 8, tempStr);
        }
    }

    // 显示倒计时提示
    unsigned long elapsed = millis() - modeChangeTime;
    unsigned long remaining = (MODE_AUTO_RETURN_TIME - elapsed) / 1000;
    char countStr[8];
    sprintf(countStr, "%lus", remaining);
    VFD_DISP_ASC57_STR(7, 13, countStr);
}

// ==================== 按钮处理函数 ====================

void Handle_Buttons() {
    unsigned long currentMillis = millis();

    // 亮度增加按钮
    if (digitalRead(K_U_PIN) == LOW) {
        delay(10);
        if (digitalRead(K_U_PIN) == LOW) {
            Disp_Brt_Data += 10;
            if (Disp_Brt_Data > 250) Disp_Brt_Data = 250;
            Serial.printf("亮度增加: %d\n", Disp_Brt_Data);
        }
        while(digitalRead(K_U_PIN) == LOW);
    }

    // 亮度减少按钮
    if (digitalRead(K_D_PIN) == LOW) {
        delay(10);
        if (digitalRead(K_D_PIN) == LOW) {
            if (Disp_Brt_Data >= 10) Disp_Brt_Data -= 10;
            Serial.printf("亮度减少: %d\n", Disp_Brt_Data);
        }
        while(digitalRead(K_D_PIN) == LOW);
    }

    // 菜单/模式切换按钮
    if (digitalRead(K_M_PIN) == LOW &&
        (currentMillis - lastButtonPress) > BUTTON_DEBOUNCE) {
        delay(10);
        if (digitalRead(K_M_PIN) == LOW) {
            lastButtonPress = currentMillis;

            if (currentMode == MODE_DEFAULT) {
                // 切换到小时预报模式
                currentMode = MODE_HOURLY;
                modeChangeTime = currentMillis;
                Serial.println("切换到小时预报模式");

                // 获取小时预报数据
                Fetch_Hourly_Forecast();
            } else {
                // 返回默认模式
                currentMode = MODE_DEFAULT;
                Serial.println("返回默认模式");
            }
        }
        while(digitalRead(K_M_PIN) == LOW);
    }

    // 自动返回逻辑
    if (currentMode == MODE_HOURLY) {
        if ((currentMillis - modeChangeTime) > MODE_AUTO_RETURN_TIME) {
            currentMode = MODE_DEFAULT;
            Serial.println("自动返回默认模式");
        }
    }
}

// ==================== Setup & Loop ====================

void setup() {
    Serial.begin(115200);
    Serial.println("\n==========================================");
    Serial.println("ESP32-S3 天气仪表板 VFD Display");
    Serial.println("==========================================\n");

    // 1. 初始化 GPIO 引脚
    pinMode(VFD_LAT_PIN, OUTPUT);
    pinMode(VFD_CLKG_PIN, OUTPUT);
    pinMode(VFD_SIG_PIN, OUTPUT);
    pinMode(HV_EN_PIN, OUTPUT);
    pinMode(FL_EN_PIN, OUTPUT);

    pinMode(K_U_PIN, INPUT_PULLUP);
    pinMode(K_D_PIN, INPUT_PULLUP);
    pinMode(K_M_PIN, INPUT_PULLUP);

    // 2. 配置 LEDC PWM
    ledcAttach(VFD_BK_PIN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcWrite(VFD_BK_PIN, Disp_Brt_Data);

    Serial.println("✓ GPIO 初始化完成");

    // 3. 初始化 SPI
    vspi = new SPIClass(HSPI);
    vspi->begin(VFD_CLKA_PIN, -1, VFD_SIA_PIN, -1);
    vspi->beginTransaction(SPISettings(8000000, LSBFIRST, SPI_MODE0));

    Serial.println("✓ SPI 初始化完成");

    // 4. VFD上电时序
    digitalWrite(FL_EN_PIN, HIGH);
    digitalWrite(HV_EN_PIN, LOW);
    DP_RAM_CLR();

    Serial.println("✓ 灯丝预热中...");
    delay(500);

    // 5. 初始化硬件定时器
    timer = timerBegin(5319);
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, 1, true, 0);

    Serial.println("✓ 硬件定时器启动");

    Disp_Buf_Update();

    digitalWrite(FL_EN_PIN, LOW);
    delay(100);
    digitalWrite(HV_EN_PIN, HIGH);
    delay(100);

    Serial.println("✓ VFD 准备就绪\n");

    // 6. 初始化 AHT20
    if (AHT20_Init()) {
        if (AHT20_ReadData(&temperature, &humidity)) {
            Serial.printf("  温度: %.1f°C\n", temperature);
            Serial.printf("  湿度: %.1f%%\n\n", humidity);
        }
    }

    // 7. 连接WiFi
    Connect_WiFi();

    // 8. 同步时间
    if (wifiConnected) {
        Sync_Time();

        // 9. 获取天气数据
        Fetch_Weather();
        Fetch_Hourly_Forecast();
        lastWeatherUpdate = millis();
    }

    Serial.println("\n按键功能:");
    Serial.println("  K_U (GPIO18): 增加亮度");
    Serial.println("  K_D (GPIO19): 减少亮度");
    Serial.println("  K_M (GPIO20): 切换显示模式\n");

    lastSensorRead = millis();
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. 处理按键
    Handle_Buttons();

    // 2. 定期读取 AHT20（每2秒）
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;
        AHT20_ReadData(&temperature, &humidity);
    }

    // 3. 定期更新天气（每10分钟）
    if (wifiConnected &&
        (currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL)) {
        lastWeatherUpdate = currentMillis;
        Fetch_Weather();
        if (currentMode == MODE_HOURLY) {
            Fetch_Hourly_Forecast();
        }
    }

    // 4. 根据当前模式显示UI
    if (currentMode == MODE_DEFAULT) {
        Display_Default_UI();
    } else {
        Display_Hourly_UI();
    }

    // 5. 更新显示缓冲区
    Disp_Buf_Update();
    delay(100);
}
