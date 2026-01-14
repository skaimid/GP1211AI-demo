// ==================== Button Handler Implementation ====================

#include "button_handler.h"
#include "vfd_display.h"
#include "ui_display.h"

#define BUTTON_DEBOUNCE 200  // 防抖时间（毫秒）

// 全局ButtonHandler对象
ButtonHandler buttonHandler;

// ==================== 构造函数 ====================
ButtonHandler::ButtonHandler()
    : lastButtonPress(0)
{
}

// ==================== 初始化 ====================
void ButtonHandler::begin() {
    pinMode(K_U_PIN, INPUT_PULLUP);
    pinMode(K_D_PIN, INPUT_PULLUP);
    pinMode(K_M_PIN, INPUT_PULLUP);

    Serial.println("✓ 按钮初始化完成");
    Serial.println("  K_U (GPIO18): 增加亮度");
    Serial.println("  K_D (GPIO19): 减少亮度");
    Serial.println("  K_M (GPIO20): 切换显示模式");
}

// ==================== 更新按键状态 ====================
void ButtonHandler::update() {
    handleBrightnessUp();
    handleBrightnessDown();
    handleModeSwitch();
}

// ==================== 处理亮度增加 ====================
void ButtonHandler::handleBrightnessUp() {
    if (digitalRead(K_U_PIN) == LOW) {
        delay(10);  // 简单防抖
        if (digitalRead(K_U_PIN) == LOW) {
            uint8_t brightness = vfd.getBrightness();
            brightness += 10;
            if (brightness > 250) brightness = 250;
            vfd.setBrightness(brightness);

            Serial.printf("亮度增加: %d\n", brightness);
        }
        while(digitalRead(K_U_PIN) == LOW);  // 等待释放
    }
}

// ==================== 处理亮度减少 ====================
void ButtonHandler::handleBrightnessDown() {
    if (digitalRead(K_D_PIN) == LOW) {
        delay(10);  // 简单防抖
        if (digitalRead(K_D_PIN) == LOW) {
            uint8_t brightness = vfd.getBrightness();
            if (brightness >= 10) {
                brightness -= 10;
                vfd.setBrightness(brightness);
                Serial.printf("亮度减少: %d\n", brightness);
            }
        }
        while(digitalRead(K_D_PIN) == LOW);  // 等待释放
    }
}

// ==================== 处理模式切换 ====================
void ButtonHandler::handleModeSwitch() {
    unsigned long currentMillis = millis();

    if (digitalRead(K_M_PIN) == LOW &&
        (currentMillis - lastButtonPress) > BUTTON_DEBOUNCE) {
        delay(10);  // 简单防抖
        if (digitalRead(K_M_PIN) == LOW) {
            lastButtonPress = currentMillis;

            // 切换显示模式
            uiDisplay.switchMode();
        }
        while(digitalRead(K_M_PIN) == LOW);  // 等待释放
    }
}
