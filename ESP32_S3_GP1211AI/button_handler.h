// ==================== Button Handler Module Header ====================
// 按钮处理模块
// 负责按键输入、防抖和功能响应

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// ==================== 按键引脚定义 ====================
#define K_U_PIN      GPIO_NUM_18  // 增加亮度
#define K_D_PIN      GPIO_NUM_19  // 减少亮度
#define K_M_PIN      GPIO_NUM_20  // 菜单/切换模式

// ==================== Button Handler类 ====================
class ButtonHandler {
public:
    ButtonHandler();

    // 初始化
    void begin();

    // 按键处理
    void update();                      // 更新按键状态（在loop中调用）

private:
    void handleBrightnessUp();          // 处理亮度增加
    void handleBrightnessDown();        // 处理亮度减少
    void handleModeSwitch();            // 处理模式切换

    unsigned long lastButtonPress;      // 上次按键时间（防抖）
};

// 全局ButtonHandler对象（在cpp中定义）
extern ButtonHandler buttonHandler;

#endif // BUTTON_HANDLER_H
