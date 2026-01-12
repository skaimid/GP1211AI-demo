// ==================== AHT20 Sensor Module Header ====================
// AHT20温湿度传感器模块
// 负责I2C通信、数据读取和温湿度计算

#ifndef AHT20_SENSOR_H
#define AHT20_SENSOR_H

#include <Arduino.h>
#include <Wire.h>

// ==================== I2C配置 ====================
#define I2C_SDA_PIN  GPIO_NUM_4   // I2C数据线
#define I2C_SCL_PIN  GPIO_NUM_5   // I2C时钟线
#define I2C_FREQ     100000       // 100kHz
#define AHT20_ADDR   0x38         // AHT20 I2C地址

// ==================== AHT20 Sensor类 ====================
class AHT20Sensor {
public:
    AHT20Sensor();

    // 初始化
    bool begin();

    // 数据读取
    bool read();                    // 触发测量并读取数据
    float getTemperature() const;   // 获取温度 (°C)
    float getHumidity() const;      // 获取湿度 (%)

    // 状态检查
    bool isReady() const;           // 检查传感器是否就绪

private:
    bool triggerMeasurement();      // 触发测量
    bool readRawData(uint8_t* data); // 读取原始数据

    float temperature;              // 温度值
    float humidity;                 // 湿度值
    bool initialized;               // 初始化标志
};

// 全局AHT20对象（在cpp中定义）
extern AHT20Sensor aht20;

#endif // AHT20_SENSOR_H
