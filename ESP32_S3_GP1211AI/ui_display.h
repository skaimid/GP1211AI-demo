// ==================== UI Display Module Header ====================
// UI显示模块
// 负责界面布局、显示逻辑和模式切换

#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include <Arduino.h>
#include "vfd_display.h"
#include "aht20_sensor.h"
#include "wifi_manager.h"
#include "weather_api.h"
#include "weather_icons.h"

// ==================== 显示模式 ====================
enum DisplayMode {
    MODE_DEFAULT,      // 默认模式：时间 + 天气 + 温湿度
    MODE_HOURLY        // 小时预报模式
};

// ==================== UI Display类 ====================
class UIDisplay {
public:
    UIDisplay();

    // 显示模式控制
    void setMode(DisplayMode mode);
    DisplayMode getMode() const;

    // 渲染界面
    void render();                      // 根据当前模式渲染
    void renderDefaultMode();           // 渲染默认模式
    void renderHourlyMode();            // 渲染小时预报模式

    // 模式切换逻辑
    void switchMode();                  // 切换模式
    bool checkAutoReturn();             // 检查自动返回（20秒）

private:
    DisplayMode currentMode;
    unsigned long modeChangeTime;
};

// 全局UIDisplay对象（在cpp中定义）
extern UIDisplay uiDisplay;

#endif // UI_DISPLAY_H
