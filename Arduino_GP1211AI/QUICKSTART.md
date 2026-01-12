# GP1211AI Arduino 快速入门指南

## 1. 准备工作

### 需要的硬件
- Arduino开发板（Uno/Nano/Mega等）
- FUTABA GP1211AI VFD显示模块
- 杜邦线若干
- 5V或12V电源（根据VFD模块要求）

### 需要的软件
- Arduino IDE（建议1.8.x或2.x版本）
- TimerOne库

### 安装TimerOne库
1. 打开Arduino IDE
2. 点击 `工具` -> `管理库`
3. 搜索 `TimerOne`
4. 点击安装

## 2. 快速接线（5步完成）

### 步骤1：电源连接（最重要！）
```
Arduino 5V  -----> VCC  (VFD模块)
Arduino GND -----> GND  (VFD模块)
```
⚠️ **确认电压！** 大多数GP1211AI需要5V，有些需要12V

### 步骤2：SPI数据线（自动刷新用）
```
Arduino D11 -----> SIA  (MOSI)
Arduino D13 -----> CLKA (SCK)
```

### 步骤3：控制信号
```
Arduino D6 -----> LAT  (锁存)
Arduino D7 -----> SIG  (信号)
Arduino D8 -----> CLKG (栅极时钟)
```

### 步骤4：亮度控制
```
Arduino D3 -----> BK   (亮度PWM)
```

### 步骤5：使能控制
```
Arduino A0 -----> HV_EN (高压使能)
Arduino A1 -----> FL_EN (灯丝使能)
```

### 按键（可选）
```
Arduino D2 -----> K_U  (增加亮度，另一端接GND)
Arduino D4 -----> K_D  (减少亮度，另一端接GND)
```

## 3. 上传代码

1. 用Arduino IDE打开 `GP1211AI.ino`
2. 选择开发板：`工具` -> `开发板` -> `Arduino Uno`
3. 选择串口：`工具` -> `端口` -> 选择你的Arduino
4. 点击上传按钮（→）

## 4. 测试运行

上传成功后，你应该看到：

1. **串口输出**（打开串口监视器，115200波特率）：
   ```
   GP1211AI VFD Display - Arduino Port
   VFD Display initialized
   ```

2. **VFD屏幕显示**：
   - 显示logo图案
   - 左上角显示亮度值（如 "001"）

3. **按键测试**：
   - 按K_U：亮度增加，数值变化
   - 按K_D：亮度减少，数值变化

## 5. 常见问题排查

### 问题1：编译错误 "expected initializer before 'ASC57'"
**已修复！** 所有字体文件已更新为Arduino兼容格式。

### 问题2：VFD不亮
**检查清单**：
- [ ] VCC电压是否正确（5V或12V）
- [ ] GND是否连接
- [ ] HV_EN和FL_EN是否接线正确
- [ ] 是否上传了代码

### 问题3：VFD很暗或看不清
**解决方法**：
- 调整代码中的初始亮度值：
  ```cpp
  unsigned char Disp_Brt_Data = 100;  // 改成100或更高
  ```

### 问题4：显示乱码
**可能原因**：
- SPI接线松动
- 时钟信号不稳定
- 尝试降低SPI速度：
  ```cpp
  SPI.setClockDivider(SPI_CLOCK_DIV8);  // 改成DIV8
  ```

### 问题5：按键不响应
**检查**：
- 按键是否连接到GND
- 引脚是否配置正确
- 使用内部上拉，不需要外部电阻

## 6. 显示你的第一行文字

在 `loop()` 函数中添加：

```cpp
void loop() {
  // 清屏
  DP_RAM_CLR();

  // 显示文字
  VFD_DISP_ASC816_STR(1, 0, "Hello World!");

  // 更新显示
  Disp_Buf_Update();

  delay(2000);
}
```

## 7. 进阶使用

### 显示多行文字
```cpp
VFD_DISP_ASC57_STR(0, 0, "Line 1");
VFD_DISP_ASC57_STR(1, 0, "Line 2");
VFD_DISP_ASC57_STR(2, 0, "Line 3");
```

### 混合字体大小
```cpp
VFD_DISP_ASC57_STR(0, 0, "Small");
VFD_DISP_ASC816_STR(2, 0, "Medium");
```

### 显示数字
```cpp
int value = 123;
VFD_DISP_ASC816_STR(1, 0, String(value).c_str());
```

### 控制亮度
```cpp
// 设置亮度为50%
Disp_Brt_Data = 100;
analogWrite(VFD_BK_PIN, map(Disp_Brt_Data, 0, 200, 0, 255));
```

## 8. 性能参数

- **刷新频率**: 265Hz（188μs/周期）
- **亮度范围**: 1-200
- **显示分辨率**: 128×64像素
- **字符支持**: ASCII 0x20-0x7E

## 9. 引脚自定义

如果想使用其他引脚，修改代码开头的宏定义：

```cpp
// 例如：把D3改为D9
#define VFD_BK_PIN    9     // PWM pin for brightness control
```

**注意**：
- VFD_BK必须是PWM引脚（带~标记）
- SPI引脚在Uno上固定为D11/D13

## 10. 获取帮助

- 查看详细文档：`README.md`
- 查看接线图：`WIRING.md`
- 查看示例：`examples/GP1211AI_Demo/`

## 恭喜！🎉

你已经成功将51单片机代码迁移到Arduino！

现在可以开始你的VFD显示项目了。祝玩得开心！
