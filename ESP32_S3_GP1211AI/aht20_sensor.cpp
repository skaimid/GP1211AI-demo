// ==================== AHT20 Sensor Implementation ====================

#include "aht20_sensor.h"

// 全局AHT20对象
AHT20Sensor aht20;

// ==================== 构造函数 ====================
AHT20Sensor::AHT20Sensor()
    : temperature(0.0f)
    , humidity(0.0f)
    , initialized(false)
{
}

// ==================== 初始化 ====================
bool AHT20Sensor::begin() {
    Serial.println("初始化AHT20传感器...");

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    delay(40);  // 等待传感器上电稳定

    // 发送初始化命令
    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xBE);  // 初始化命令
    Wire.write(0x08);  // 参数1
    Wire.write(0x00);  // 参数2
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.printf("✗ AHT20初始化失败 (错误代码: %d)\n", error);
        initialized = false;
        return false;
    }

    delay(10);
    initialized = true;

    // 首次读取数据
    if (read()) {
        Serial.printf("✓ AHT20初始化成功\n");
        Serial.printf("  温度: %.1f°C, 湿度: %.1f%%\n", temperature, humidity);
        return true;
    }

    Serial.println("✗ AHT20数据读取失败");
    return false;
}

// ==================== 触发测量 ====================
bool AHT20Sensor::triggerMeasurement() {
    if (!initialized) return false;

    Wire.beginTransmission(AHT20_ADDR);
    Wire.write(0xAC);  // 触发测量命令
    Wire.write(0x33);  // 参数1
    Wire.write(0x00);  // 参数2
    uint8_t error = Wire.endTransmission();

    return (error == 0);
}

// ==================== 读取原始数据 ====================
bool AHT20Sensor::readRawData(uint8_t* data) {
    if (!initialized) return false;

    Wire.requestFrom(AHT20_ADDR, 6);

    if (Wire.available() != 6) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        data[i] = Wire.read();
    }

    // 检查状态位 (bit[7] = 忙标志, 应该为 0)
    if (data[0] & 0x80) {
        return false;
    }

    return true;
}

// ==================== 数据读取 ====================
bool AHT20Sensor::read() {
    if (!initialized) return false;

    // 触发测量
    if (!triggerMeasurement()) {
        return false;
    }

    // 等待测量完成 (典型值 80ms)
    delay(80);

    // 读取原始数据
    uint8_t data[6];
    if (!readRawData(data)) {
        return false;
    }

    // 计算湿度 (20位数据)
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) |
                            ((uint32_t)data[2] << 4) |
                            ((uint32_t)data[3] >> 4);
    humidity = (raw_humidity * 100.0f) / 1048576.0f;  // 2^20 = 1048576

    // 计算温度 (20位数据)
    uint32_t raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) |
                               ((uint32_t)data[4] << 8) |
                               ((uint32_t)data[5]);
    temperature = (raw_temperature * 200.0f) / 1048576.0f - 50.0f;

    return true;
}

// ==================== 获取数据 ====================
float AHT20Sensor::getTemperature() const {
    return temperature;
}

float AHT20Sensor::getHumidity() const {
    return humidity;
}

// ==================== 状态检查 ====================
bool AHT20Sensor::isReady() const {
    return initialized;
}
