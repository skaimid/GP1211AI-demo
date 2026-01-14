// ==================== UI Display Implementation ====================

#include "ui_display.h"

#define MODE_AUTO_RETURN_TIME 20000  // 20秒自动返回

// 全局UIDisplay对象
UIDisplay uiDisplay;

// ==================== 构造函数 ====================
UIDisplay::UIDisplay()
    : currentMode(MODE_DEFAULT)
    , modeChangeTime(0)
{
}

// ==================== 显示模式控制 ====================
void UIDisplay::setMode(DisplayMode mode) {
    currentMode = mode;
    if (mode == MODE_HOURLY) {
        modeChangeTime = millis();
    }
}

DisplayMode UIDisplay::getMode() const {
    return currentMode;
}

// ==================== 渲染界面 ====================
void UIDisplay::render() {
    if (currentMode == MODE_DEFAULT) {
        renderDefaultMode();
    } else {
        renderHourlyMode();
    }
    vfd.update();
}

void UIDisplay::renderDefaultMode() {
    vfd.clear();

    // 第一行：显示当前时间
    struct tm timeinfo;
    if (wifiMgr.getLocalTime(&timeinfo)) {
        char timeBuffer[16];
        sprintf(timeBuffer, "%02d:%02d:%02d",
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        vfd.drawString8x16(0, 0, timeBuffer);

        // 显示日期（右侧小字）
        char dateBuffer[16];
        sprintf(dateBuffer, "%02d/%02d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
        vfd.drawString5x7(0, 10, dateBuffer);
    } else {
        vfd.drawString8x16(0, 0, "--:--:--");
    }

    // 下部左侧：外部天气和气温（带图标）
    const WeatherData& weather = weatherAPI.getCurrentWeather();
    if (weather.valid) {
        // 显示天气图标 (行3-4, 列0-1)
        vfd.drawIcon16x16(3, 0, weatherAPI.getWeatherIcon(weather.weatherId));

        // 显示温度 (行1)
        char tempStr[16];
        sprintf(tempStr, "%3.0fC", weather.temp);
        vfd.drawString8x16(1, 2, tempStr);

        // 显示天气描述 (行5, 小字)
        String desc = weather.description.substring(0, 10);
        vfd.drawString5x7(5, 0, desc.c_str());
    } else {
        vfd.drawString5x7(3, 0, "Weather");
        vfd.drawString5x7(4, 0, "N/A");
    }

    // 下部右侧：传感器温湿度
    if (aht20.isReady()) {
        char buffer[16];

        // 温度图标和数值
        vfd.drawIcon8x8(3, 64, icon_temp);
        sprintf(buffer, "%5.1fC", aht20.getTemperature());
        vfd.drawString8x16(1, 9, buffer);

        // 湿度图标和数值
        vfd.drawIcon8x8(5, 64, icon_humidity);
        sprintf(buffer, "%5.1f%%", aht20.getHumidity());
        vfd.drawString8x16(2, 9, buffer);
    }
}

void UIDisplay::renderHourlyMode() {
    vfd.clear();

    // 标题
    vfd.drawString8x16(0, 0, "HOURLY");

    // 显示4个时段的预报（分两行）
    for (int i = 0; i < 4; i++) {
        int x = 2;
        int y = i * 30;

        // 第一行：前4小时
        const WeatherData& forecast1 = weatherAPI.getHourlyForecast(i);
        if (forecast1.valid) {
            char tempStr[8];
            sprintf(tempStr, "%2.0fC", forecast1.temp);
            vfd.drawString5x7(x, y / 8, tempStr);
        }

        // 第二行：后4小时
        if (i + 4 < 8) {
            const WeatherData& forecast2 = weatherAPI.getHourlyForecast(i + 4);
            if (forecast2.valid) {
                char tempStr[8];
                sprintf(tempStr, "%2.0fC", forecast2.temp);
                vfd.drawString5x7(x + 2, y / 8, tempStr);
            }
        }
    }

    // 显示倒计时提示
    unsigned long elapsed = millis() - modeChangeTime;
    if (elapsed < MODE_AUTO_RETURN_TIME) {
        unsigned long remaining = (MODE_AUTO_RETURN_TIME - elapsed) / 1000;
        char countStr[8];
        sprintf(countStr, "%lus", remaining);
        vfd.drawString5x7(7, 13, countStr);
    }
}

// ==================== 模式切换 ====================
void UIDisplay::switchMode() {
    if (currentMode == MODE_DEFAULT) {
        // 切换到小时预报模式
        currentMode = MODE_HOURLY;
        modeChangeTime = millis();
        Serial.println("切换到小时预报模式");

        // 获取小时预报数据
        weatherAPI.fetchHourlyForecast();
    } else {
        // 返回默认模式
        currentMode = MODE_DEFAULT;
        Serial.println("返回默认模式");
    }
}

// ==================== 自动返回检查 ====================
bool UIDisplay::checkAutoReturn() {
    if (currentMode == MODE_HOURLY) {
        if ((millis() - modeChangeTime) > MODE_AUTO_RETURN_TIME) {
            currentMode = MODE_DEFAULT;
            Serial.println("自动返回默认模式");
            return true;
        }
    }
    return false;
}
