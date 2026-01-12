// ==================== Weather API Module Header ====================
// 天气API模块
// 负责与OpenWeatherMap API通信，获取天气数据

#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "weather_icons.h"

// ==================== 天气更新间隔 ====================
#define WEATHER_UPDATE_INTERVAL 600000  // 10分钟

// ==================== 天气数据结构 ====================
struct WeatherData {
    float temp;                  // 温度 (°C)
    int humidity;                // 湿度 (%)
    String description;          // 天气描述
    int weatherId;               // 天气ID
    bool valid;                  // 数据有效性

    WeatherData() : temp(0), humidity(0), weatherId(0), valid(false) {}
};

// ==================== Weather API类 ====================
class WeatherAPI {
public:
    WeatherAPI();

    // 配置API
    void configure(const char* apiKey, const char* city, const char* country);

    // 获取天气数据
    bool fetchCurrentWeather();             // 获取当前天气
    bool fetchHourlyForecast();             // 获取小时预报

    // 获取数据
    const WeatherData& getCurrentWeather() const;
    const WeatherData& getHourlyForecast(int index) const;  // index: 0-7

    // 自动更新检查
    bool needsUpdate() const;               // 检查是否需要更新

    // 工具方法
    const uint8_t* getWeatherIcon(int weatherId) const;  // 根据ID获取图标

private:
    String apiKey;
    String city;
    String country;

    WeatherData currentWeather;
    WeatherData hourlyForecast[8];

    unsigned long lastUpdate;
};

// 全局WeatherAPI对象（在cpp中定义）
extern WeatherAPI weatherAPI;

#endif // WEATHER_API_H
