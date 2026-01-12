# 内存优化说明

## 问题
Arduino Uno只有2048字节RAM，原始代码使用了4793字节（234%），超出限制。

## 解决方案

### 三个版本可选：

#### 1. GP1211AI_Simple.ino（推荐）
**RAM使用**: ~1200字节 (59%)

**优化内容**:
- ✅ 删除了2064字节的SPI缓冲区 DP_BUF[2064]
- ✅ 直接在定时器中断中计算传输数据
- ✅ 只保留5×7字体
- ✅ 简化功能，只显示HelloWorld

**使用方法**:
1. 用Arduino IDE打开 `GP1211AI_Simple.ino`
2. 确保 `ASC57.h` 在同一目录
3. 上传到Arduino

#### 2. GP1211AI_Ultra.ino（最省内存）
**RAM使用**: ~1100字节 (54%)

**优化内容**:
- ✅ 使用PROGMEM将字体数据存到Flash（节省420字节RAM）
- ✅ 删除SPI缓冲区
- ✅ 最小化全局变量

**使用方法**:
1. 用Arduino IDE打开 `GP1211AI_Ultra.ino`
2. 确保 `ASC57_Progmem.h` 在同一目录
3. 上传到Arduino

## RAM使用对比

| 版本 | RAM使用 | 百分比 | 说明 |
|------|---------|--------|------|
| 原始版本 | 4793字节 | 234% | ❌ 超出限制 |
| Simple版本 | ~1200字节 | 59% | ✅ 推荐 |
| Ultra版本 | ~1100字节 | 54% | ✅ 最省内存 |

## 内存优化技巧

### 1. 删除不必要的大数组
```cpp
// ❌ 原始代码
unsigned char DP_BUF[2064];  // 占用2064字节

// ✅ 优化后
// 删除DP_BUF，直接在定时器中计算
```

### 2. 使用PROGMEM存储常量数据
```cpp
// ❌ 数据在RAM中
const unsigned char font[] = { ... };

// ✅ 数据在Flash中
const unsigned char font[] PROGMEM = { ... };
// 读取时: pgm_read_byte(&font[index])
```

### 3. 使用const
```cpp
// ❌ 占用RAM
unsigned char brightness = 100;

// ✅ 可能被优化到Flash
const unsigned char brightness = 100;
```

### 4. 减少字体数量
```cpp
// ❌ 包含多种字体
#include "ASC57.h"
#include "ASC816.h"
#include "ASC1224.h"

// ✅ 只包含需要的字体
#include "ASC57.h"  // 只用5×7字体
```

## 编译后查看内存使用

在Arduino IDE编译后，底部会显示：

```
Sketch uses 1234 bytes (3%) of program storage space.
Maximum is 32256 bytes.

Global variables use 1156 bytes (56%) of dynamic memory,
leaving 892 bytes for local variables. Maximum is 2048 bytes.
```

确保第二行的百分比小于100%！

## 推荐使用

**Arduino Uno/Nano (2KB RAM)**: 使用 `GP1211AI_Simple.ino`

**Arduino Mega (8KB RAM)**: 可以使用原始的 `GP1211AI.ino`

**ESP32 (520KB RAM)**: 可以使用任何版本

## 常见问题

### Q: 编译后还是超过100%怎么办？
A: 使用 `GP1211AI_Ultra.ino`，它使用PROGMEM，最省内存。

### Q: 显示不正常？
A: 检查：
1. SPI接线是否正确
2. 时钟信号是否稳定
3. 电源是否充足

### Q: 想要更多功能？
A: 如果使用Arduino Mega，内存足够，可以用完整版 `GP1211AI.ino`。
