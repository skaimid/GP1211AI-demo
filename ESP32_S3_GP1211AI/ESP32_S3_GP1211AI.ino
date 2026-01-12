// ==================== ESP32-S3 天气仪表板主程序 ====================
// 基于GP1211AI VFD显示屏的智能天气仪表板
//
// 功能特性：
// ✅ WiFi连接和NTP时间同步
// ✅ 实时时钟显示
// ✅ OpenWeatherMap天气API集成
// ✅ AHT20温湿度传感器
// ✅ 双模式显示（默认/小时预报）
// ✅ 按钮控制（亮度/模式切换）
//
// 代码架构：模块化设计，易于维护和扩展
// ===========================================================

#include "config.h"
#include "vfd_display.h"
#include "aht20_sensor.h"
#include "wifi_manager.h"
#include "weather_api.h"
#include "ui_display.h"
#include "button_handler.h"

// ==================== 全局变量 ====================
unsigned long lastSensorRead = 0;
#define SENSOR_READ_INTERVAL 2000  // 传感器读取间隔（毫秒）

// ==================== Setup ====================
void setup() {
    Serial.begin(115200);
    Serial.println("\n==========================================");
    Serial.println("ESP32-S3 天气仪表板 VFD Display");
    Serial.println("模块化架构 v2.0");
    Serial.println("==========================================\n");

    // 1. 初始化VFD显示驱动
    if (!vfd.begin()) {
        Serial.println("✗ VFD初始化失败");
        while(1) delay(1000);
    }

    // 2. 初始化AHT20传感器
    if (!aht20.begin()) {
        Serial.println("⚠ AHT20传感器初始化失败，温湿度功能不可用");
    }

    // 3. 初始化按钮
    buttonHandler.begin();

    // 4. 连接WiFi
    if (wifiMgr.connect(WIFI_SSID, WIFI_PASSWORD)) {
        // 5. 同步时间
        wifiMgr.syncTime(NTP_SERVER, TIMEZONE_OFFSET);

        // 6. 配置天气API
        weatherAPI.configure(WEATHER_API_KEY, WEATHER_CITY, WEATHER_COUNTRY);

        // 7. 获取天气数据
        weatherAPI.fetchCurrentWeather();
        weatherAPI.fetchHourlyForecast();
    } else {
        Serial.println("⚠ WiFi连接失败，天气功能不可用");
    }

    Serial.println("\n==========================================");
    Serial.println("系统初始化完成！");
    Serial.println("==========================================\n");

    lastSensorRead = millis();
}

// ==================== Loop ====================
void loop() {
    unsigned long currentMillis = millis();

    // 1. 处理按键输入
    buttonHandler.update();

    // 2. 定期读取AHT20传感器（每2秒）
    if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
        lastSensorRead = currentMillis;
        if (aht20.isReady()) {
            aht20.read();
        }
    }

    // 3. 检查天气数据是否需要更新（每10分钟）
    if (wifiMgr.isConnected() && weatherAPI.needsUpdate()) {
        weatherAPI.fetchCurrentWeather();
        if (uiDisplay.getMode() == MODE_HOURLY) {
            weatherAPI.fetchHourlyForecast();
        }
    }

    // 4. 检查自动返回默认模式
    uiDisplay.checkAutoReturn();

    // 5. 渲染UI
    uiDisplay.render();

    // 6. 延迟
    delay(100);
}
